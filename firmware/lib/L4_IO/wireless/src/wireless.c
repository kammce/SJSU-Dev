/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "wireless.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include "mesh.h"
#include "nrf24L01Plus.h"
#include "sys_config.h"   /* WIRELESS_CHANNEL_NUM */
#include "lpc_sys.h"
#include "eint.h"



static QueueHandle_t g_rx_queue = NULL;     ///< Queue handle for RX queue
static QueueHandle_t g_ack_queue = NULL;    ///< Queue handle for RX Ack packet
static SemaphoreHandle_t g_nrf_activity_sem = NULL; ///< If FreeRTOS is running, we will not poll for nordic activity

/** @{ Functions used for nordic wireless mesh network
 * These are call-back functions for mesh_service() so you shouldn't use these directly.
 */
static int nrf_driver_init(void* p, int len);     ///< Initializes nordic wireless chip
static int nrf_driver_send(void* p, int len);     ///< Sends the data over nordic
static int nrf_driver_receive(void* p, int len);  ///< Gets the data from nordic, returns true if packet was fetched
static int nrf_driver_app_recv(void *p, int len); ///< Application callback function when mesh_service() gets data for us
static int nrf_driver_get_timer(void *p, int len);///< Get system timer value.
/** @} */

/// Retrieves a packet from the queue handle with the given timeout.
static char wireless_get_queued_pkt(QueueHandle_t qhandle, mesh_packet_t *pkt, const uint32_t timeout_ms)
{
    char ok = 0;

    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        ok = xQueueReceive(qhandle, pkt, OS_MS(timeout_ms));
    }
    else {
        uint64_t timeout_of_char = sys_get_uptime_ms() + timeout_ms;
        while (! (ok = xQueueReceive(qhandle, pkt, 0))) {
            if (sys_get_uptime_ms() > timeout_of_char) {
                break;
            }
        }
    }

    return ok;
}

/// ISR callback function upon NRF IRQ rising edge interrupt
static void nrf_irq_callback(void)
{
    long yieldRequired = 0;
    xSemaphoreGiveFromISR(g_nrf_activity_sem, &yieldRequired);
    portEND_SWITCHING_ISR(yieldRequired);
}



bool wireless_init(void)
{
    mesh_driver_t driver;
    driver.app_recv   = nrf_driver_app_recv;
    driver.radio_init = nrf_driver_init;
    driver.radio_recv = nrf_driver_receive;
    driver.radio_send = nrf_driver_send;
    driver.get_timer  = nrf_driver_get_timer;

    /* Don't send broadcast message because node address may be changed by user after init() */
    return mesh_init(WIRELESS_NODE_ADDR, true, WIRELESS_NODE_NAME, driver, false);
}

char wireless_get_rx_pkt(mesh_packet_t *pkt, const uint32_t timeout_ms)
{
    return wireless_get_queued_pkt(g_rx_queue, pkt, timeout_ms);
}

char wireless_get_ack_pkt(mesh_packet_t *pkt, const uint32_t timeout_ms)
{
    return wireless_get_queued_pkt(g_ack_queue, pkt, timeout_ms);
}

int wireless_flush_rx(void)
{
    int cnt = 0;
    mesh_packet_t pkt;
    while(wireless_get_rx_pkt(&pkt, 0) || wireless_get_ack_pkt(&pkt, 0)) {
        cnt++;
    }
    return cnt;
}

void wireless_service(void)
{
    /*
     * If FreeRTOS is running, then a task should be calling us, so we can block on
     * the nordic activity semaphore.
     *
     * There are three cases of block time :
     *  1 - If nordic interrupt signal is still pending, then we haven't read
     *      all Nordic FIFO, so we don't block on semaphore at all.
     *  2 - There are pending packets that need either ACK or retry, so we
     *      block just for one tick to carry out mesh logic.
     *  3 - No RX and no TX, so block until either a packet is sent, or until
     *      we receive a packet; both cases will give the semaphore.
     */
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        if (!nordic_intr_signal()) {
            const TickType_t blockTime = mesh_get_pnd_pkt_count() ? 1 : portMAX_DELAY;
            xSemaphoreTake(g_nrf_activity_sem, blockTime);
        }
        mesh_service();
    }
    /* A timer ISR is calling us, so we can't use FreeRTOS API, hence we poll */
    else {
        if (nordic_intr_signal() || mesh_get_pnd_pkt_count() > 0) {
            mesh_service();
        }
    }
}



static int nrf_driver_init(void* p, int len)
{
    if (NULL == g_rx_queue) {
        g_rx_queue = xQueueCreate(WIRELESS_RX_QUEUE_SIZE, MESH_PAYLOAD);
    }
    if (NULL == g_ack_queue) {
        g_ack_queue = xQueueCreate(1, MESH_PAYLOAD);
    }
    if (NULL == g_nrf_activity_sem) {
        g_nrf_activity_sem = xSemaphoreCreateBinary();
    }

    // Optional: Provide names of the FreeRTOS objects for the Trace Facility
    vTraceSetSemaphoreName(g_nrf_activity_sem, "NRF Act Sem");
    vTraceSetQueueName(g_rx_queue,  "NRF RX-Q");
    vTraceSetQueueName(g_ack_queue, "NRF ACK-Q");

    nordic_init(MESH_PAYLOAD, WIRELESS_CHANNEL_NUM, WIRELESS_AIR_DATARATE_KBPS);
    nordic_standby1_to_rx();

    /* Hook up the interrupt callback for nordic pin */
    eint3_enable_port0(BIO_NORDIC_IRQ_P0PIN, eint_falling_edge, nrf_irq_callback);

    return (NULL != g_rx_queue && NULL != g_ack_queue && NULL != g_nrf_activity_sem);
}

static int nrf_driver_send(void* p, int len)
{
    /**
     * Air time is 1 byte preamble, 5 byte address, 2 byte CRC and 9 bits at the end.
     * We add 25 just to make sure we satisfy air time requirement.
     * The slots is the number of slots we allocate for someone to send their data,
     * and we pick one of them to avoid data collision when nodes repeat a packet.
     */
	static const uint32_t s_pkt_air_time_us =
	        25 + (((8 * (MESH_PAYLOAD + 1 + 5 + 3)) * 1000) / WIRELESS_AIR_DATARATE_KBPS);
	const uint32_t slots = MESH_MAX_NODES;
    int packetWasSent = 1;

    /**
     * If we are not the source of this packet, that means we are repeating the packet.
     * If we are repeating the packet to discover the route (mac.dst == MESH_ZERO_ADDR)
     * then we need to randomly pick one air-time slot otherwise if all nodes send at
     * the same time, then their data will collide and packet won't go through.
     */
    const mesh_packet_t *pkt = (mesh_packet_t*)p;
    if (mesh_get_node_address() != pkt->nwk.src) {
        if (MESH_ZERO_ADDR == pkt->mac.dst) {
            const uint32_t timeSlotDelayUs = ((rand() % slots) + 1) * s_pkt_air_time_us;
            delay_us(timeSlotDelayUs); /**< Maximize mesh nodes to repeat the packet and not collide */
        }
    }

	// Bring from RX mode to TX mode
	nordic_rx_to_Stanby1();
	nordic_standby1_to_tx_mode1();

	// Send the packet :
	nordic_mode1_send_single_packet(p, len);
	nordic_clear_packet_sent_flag();

	// Switch back to receive mode
	nordic_standby1_to_rx();

	/* If FreeRTOS is running, we are probably blocked indefinitely on the activity semaphore.
	 * So we will give the semaphore here, to give the mesh network task to unblock and
	 * carry out retry logic.  We use FromISR() API such that mesh_send() will not be
	 * restricted to be called from a FreeRTOS task alone.
	 */
	if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
	    xSemaphoreGiveFromISR(g_nrf_activity_sem, NULL);
	}

	return packetWasSent;
}

static int nrf_driver_receive(void* p, int len)
{
	int packetWasReceived = 0;

	if(nordic_is_packet_available())
	{
		nordic_read_rx_fifo(p, len);

		// Only clear the interrupt if no more packet available
		// because nordic has 3 level Rx FIFO.
		if (!nordic_is_packet_available()) {
		    nordic_clear_packet_available_flag();
		}
		packetWasReceived = 1;
	}

	return packetWasReceived;
}

static int nrf_driver_app_recv(void *p, int len)
{
    /* Only mesh_pkt_ack_rsp is an ACK packet, others are to REQUEST for ack or nack */
    const mesh_packet_t *pkt = (mesh_packet_t*) p;
    const QueueHandle_t qhandle = (mesh_pkt_ack_rsp == pkt->info.pkt_type) ? g_ack_queue : g_rx_queue;

    int ok = xQueueSend(qhandle, p, 0);

    /* If queue was full, discard oldest data, and push again */
    if (!ok) {
        mesh_packet_t discarded_pkt;
        xQueueReceive(qhandle, &discarded_pkt, 0);
        ok = xQueueSend(qhandle, p, 0);
    }

    return ok;
}

static int nrf_driver_get_timer(void *p, int len)
{
    const int ok = (sizeof(uint32_t) == len) && (NULL != p);
    const uint32_t timerValueMs = (sys_get_uptime_ms() & UINT32_MAX);

    if (ok) {
        uint32_t *timer = (uint32_t*)p;
        *timer = timerValueMs;
    }

    return ok;
}

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
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "can.h"
#include "LPC17xx.h"
#include "sys_config.h"
#include "lpc_sys.h"    // sys_get_uptime_ms()



/**
 * If non-zero, test code is enabled, and each message sent is self-recepted.
 * You need to either connect a CAN transceiver, or connect RD/TD wires of
 * the board with a 1K resistor for the tests to work.
 *
 * Note that FullCAN and CAN filter is not tested together, but they both work individually.
 */
#define CAN_TESTING          0

/// CAN index: enum to struct index conversion
#define CAN_INDEX(can)       (can)
#define CAN_STRUCT_PTR(can)  (&(g_can_structs[CAN_INDEX(can)]))
#define CAN_VALID(x)         (can1 == x || can2 == x)

// Used by CAN_CT_ASSERT().  Obtained from http://www.pixelbeat.org/programming/gcc/static_assert.html
#define CAN_ASSERT_CONCAT_(a, b) a##b
#define CAN_ASSERT_CONCAT(a, b) CAN_ASSERT_CONCAT_(a, b)
#define CAN_CT_ASSERT(e) enum { CAN_ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

// Make some compile-time (CT) checks :
// Check the sizes of the structs because the size needs to match the HW registers
CAN_CT_ASSERT( 2 == sizeof(can_std_id_t));
CAN_CT_ASSERT( 4 == sizeof(can_ext_id_t));
CAN_CT_ASSERT( 8 == sizeof(can_data_t));
CAN_CT_ASSERT(16 == sizeof(can_msg_t));
CAN_CT_ASSERT(12 == sizeof(can_fullcan_msg_t));



/// Interrupt masks of the CANxIER and CANxICR registers
typedef enum {
    intr_rx   = (1 << 0),   ///< Receive
    intr_tx1  = (1 << 1),   ///< Transmit 1
    intr_warn = (1 << 2),   ///< Warning (if error BUS status changes)
    intr_ovrn = (1 << 3),   ///< Data overrun
    intr_wkup = (1 << 4),   ///< Wake-up
    intr_epi  = (1 << 5),   ///< Change from error active to error passive or vice versa
    intr_ali  = (1 << 6),   ///< Arbitration lost
    intr_berr = (1 << 7),   ///< Bus error (happens during each error/retry of a message)
    intr_idi  = (1 << 8),   ///< ID ready (a message was transmitted or aborted)
    intr_tx2  = (1 << 9),   ///< Transmit 2
    intr_tx3  = (1 << 10),  ///< Transmit 3
    intr_all_tx = (intr_tx1 | intr_tx2 | intr_tx3), ///< Mask of the 3 transmit buffers
} can_intr_t;

/// Bit mask of SR register indicating which hardware buffer is available
enum {
    tx1_avail = (1 << 2),   ///< Transmit buffer 1 is available
    tx2_avail = (1 << 10),  ///< Transmit buffer 2 is available
    tx3_avail = (1 << 18),  ///< Transmit buffer 3 is available
    tx_all_avail = (tx1_avail | tx2_avail | tx3_avail),
};

/**
 * Data values of the AFMR register
 * @note Since AFMR is common to both controllers, when bypass mode is enabled,
 *       then ALL messages from ALL CAN controllers will be accepted
 *
 *  Bit1: Bypass    Bit0: ACC Off
 *      0               1               No messages accepted
 *      1               X               All messages accepted
 *      0               0               HW Filter or FullCAN
 */
enum {
    afmr_enabled   = 0x00, ///< Hardware acceptance filtering
    afmr_disabled  = 0x01, ///< No messages will be accepted
    afmr_bypass    = 0x02, ///< Bypass mode, all messages will be accepted.  Both 0x02 or 0x03 will work.
    afmr_fullcan   = 0x04, ///< Hardware will receive and store messages per FullCAN mode.
};

/// CAN MOD register values
enum {
    can_mod_normal = 0x00, ///< CAN MOD register value to enable the BUS
    can_mod_reset  = 0x01, ///< CAN MOD register value to reset the BUS
    can_mod_normal_tpm = (can_mod_normal | (1 << 3)), ///< CAN bus enabled with TPM mode bits set
    can_mod_selftest   = (1 << 2) | can_mod_normal,   ///< Used to enable global self-test
};

/// Mask of the PCONP register
enum {
    can1_pconp_mask = (1 << 13),    ///< CAN1 power on bitmask
    can2_pconp_mask = (1 << 14),    ///< CAN2 power on bitmask
};

/// Typedef of CAN queues and data
typedef struct {
    LPC_CAN_TypeDef *pCanRegs;      ///< The pointer to the CAN registers
    QueueHandle_t rxQ;              ///< TX queue
    QueueHandle_t txQ;              ///< RX queue
    uint16_t droppedRxMsgs;         ///< Number of messages dropped if no space found during the CAN interrupt that queues the RX messages
    uint16_t rxQWatermark;          ///< Watermark of the FreeRTOS Rx Queue
    uint16_t txQWatermark;          ///< Watermark of the FreeRTOS Tx Queue
    uint16_t txMsgCount;            ///< Number of messages sent
    uint16_t rxMsgCount;            ///< Number of received messages
    can_void_func_t bus_error;      ///< When serious BUS error occurs
    can_void_func_t data_overrun;   ///< When we read the CAN buffer too late for incoming message
} can_struct_t ;

/// Structure of both CANs
can_struct_t g_can_structs[can_max] = { {LPC_CAN1}, {LPC_CAN2}};

/**
 * This type of CAN interrupt should lead to "bus error", but note that intr_berr is not the right
 * one as that one is triggered upon each type of CAN error which may be a simple "retry" that
 * can be recovered.  intr_epi or intr_warn should work for this selection.
 */
static const can_intr_t g_can_bus_err_intr = intr_epi;


/** @{ Private functions */
/**
 * Sends a message using an available buffer.  Initially this chose one out of the three buffers but that's
 * a little tricky to use when messages are always queued since one of the 3 buffers can be starved and not
 * get sent.  So therefore some of that logic is #ifdef'd out to only use one HW buffer.
 *
 * @returns true if the message was written to the HW buffer to be sent, otherwise false if the HW buffer(s) are busy.
 *
 * Notes:
 *  - Using the TX message counter and the TPM bit, we can ensure that the HW chooses between the TX1/TX2/TX3
 *    in a round-robin fashion otherwise there is a possibility that if the CAN Tx queue is always full,
 *    a low message ID can be starved even if it was amongst the first ones written using this method call.
 *
 * @warning This should be called from critical section since this method is not thread-safe
 */
static bool CAN_tx_now (can_struct_t *struct_ptr, can_msg_t *msg_ptr)
{
    // 32-bit command of CMR register to start transmission of one of the buffers
    enum {
        go_cmd_invalid = 0,
        go_cmd_tx1 = 0x21,
        go_cmd_tx2 = 0x41,
        go_cmd_tx3 = 0x81,
    };

    LPC_CAN_TypeDef *pCAN = struct_ptr->pCanRegs;
    const uint32_t can_sr_reg = pCAN->SR;
    volatile can_msg_t *pHwMsgRegs = NULL;
    uint32_t go_cmd = go_cmd_invalid;

    if (can_sr_reg & tx1_avail){
        pHwMsgRegs = (can_msg_t*)&(pCAN->TFI1);
        go_cmd = go_cmd_tx1;
    }
#if 0
    else if (can_sr_reg & tx2_avail){
        pHwMsgRegs = (can_msg_t*)&(pCAN->TFI2);
        go_cmd = go_cmd_tx2;
    }
    else if (can_sr_reg & tx3_avail){
        pHwMsgRegs = (can_msg_t*)&(pCAN->TFI3);
        go_cmd = go_cmd_tx3;
    }
#endif
    else {
        /* No buffer available, return failure */
        return false;
    }

    /* Copy the CAN message to the HW CAN registers and write the 8 TPM bits.
     * We set TPM bits each time by using the txMsgCount because otherwise if TX1, and TX2 are always
     * being written with a lower message ID, then TX3 will starve and never be sent.
     */
#if 0
    // Higher number will be sent later, but how do we handle the rollover from 255 to 0 because then the
    // newly written 0 will be sent, and buffer that contains higher TPM can starve.
    const uint8_t tpm = struct_ptr->txMsgCount;
    msg_ptr->frame |= tpm;
#endif
    *pHwMsgRegs = *msg_ptr;
    struct_ptr->txMsgCount++;

    #if CAN_TESTING
    go_cmd &= (0xF0);
    go_cmd = (1 << 4); /* Self reception */
    #endif

    /* Send the message! */
    pCAN->CMR = go_cmd;
    return true;
}

static void CAN_handle_isr(const can_t can)
{
    can_struct_t *pStruct = CAN_STRUCT_PTR(can);
    LPC_CAN_TypeDef *pCAN = pStruct->pCanRegs;
    const uint32_t rbs = (1 << 0);
    const uint32_t ibits = pCAN->ICR;
    UBaseType_t count;
    can_msg_t msg;

    /* Handle the received message */
    if ((ibits & intr_rx) | (pCAN->GSR & rbs)) {
        if( (count = uxQueueMessagesWaitingFromISR(pStruct->rxQ)) > pStruct->rxQWatermark) {
            pStruct->rxQWatermark = count;
        }

        can_msg_t *pHwMsgRegs = (can_msg_t*) &(pCAN->RFS);
        if (xQueueSendFromISR(pStruct->rxQ, pHwMsgRegs, NULL)) {
            pStruct->rxMsgCount++;
        }
        else {
            pStruct->droppedRxMsgs++;
        }
        pCAN->CMR = 0x04; // Release the receive buffer, no need to bitmask
    }

    /* A transmit finished, send any queued message(s) */
    if (ibits & intr_all_tx) {
        if( (count = uxQueueMessagesWaitingFromISR(pStruct->txQ)) > pStruct->txQWatermark) {
            pStruct->txQWatermark = count;
        }
        if (xQueueReceiveFromISR(pStruct->txQ, &msg, NULL)) {
            CAN_tx_now(pStruct, &msg);
        }
    }

    /* We only enable interrupt when a valid callback exists, so no need
     * to check for the callback function being NULL
     */
    if (ibits & g_can_bus_err_intr) {
        pStruct->bus_error(ibits);
    }
    if (ibits & intr_ovrn) {
        pStruct->data_overrun(ibits);
    }
}
/** @} */

/**
 * Actual ISR Handler (mapped to startup file's interrupt vector function name)
 * This interrupt is shared between CAN1, and CAN2
 */
#ifdef __cplusplus
extern "C" {
#endif
void CAN_IRQHandler(void)
{
    const uint32_t pconp = LPC_SC->PCONP;

    /* Reading registers without CAN powered up will cause DABORT exception */
    if (pconp & can1_pconp_mask) {
        CAN_handle_isr(can1);
    }

    if (pconp & can2_pconp_mask) {
        CAN_handle_isr(can2);
    }
}
#ifdef __cplusplus
}
#endif



bool CAN_init(can_t can, uint32_t baudrate_kbps, uint16_t rxq_size, uint16_t txq_size,
              can_void_func_t bus_off_cb, can_void_func_t data_ovr_cb)
{
    if (!CAN_VALID(can)){
        return false;
    }

    can_struct_t *pStruct = CAN_STRUCT_PTR(can);
    LPC_CAN_TypeDef *pCAN = pStruct->pCanRegs;
    bool failed = true;

    /* Enable CAN Power, and select the PINS
     * CAN1 is at P0.0, P0.1 and P0.21, P0.22
     * CAN2 is at P0.4, P0.5 and P2.7,  P2.8
     * On SJ-One board, we have P0.0, P0.1, and P2.7, P2.8
     */
    if (can1 == can) {
        LPC_SC->PCONP |= can1_pconp_mask;
        LPC_PINCON->PINSEL0 &= ~(0xF << 0);
        LPC_PINCON->PINSEL0 |=  (0x5 << 0);
    }
    else if (can2 == can){
        LPC_SC->PCONP |= can2_pconp_mask;
        LPC_PINCON->PINSEL4 &= ~(0xF << 14);
        LPC_PINCON->PINSEL4 |=  (0x5 << 14);
    }

    /* Create the queues with minimum size of 1 to avoid NULL pointer reference */
    if (!pStruct->rxQ) {
        pStruct->rxQ = xQueueCreate(rxq_size ? rxq_size : 1, sizeof(can_msg_t));
    }
    if (!pStruct->txQ) {
        pStruct->txQ = xQueueCreate(txq_size ? txq_size : 1, sizeof(can_msg_t));
    }

    /* The CAN dividers must all be the same for both CANs
     * Set the dividers of CAN1, CAN2, ACF to CLK / 1
     */
    lpc_pclk(pclk_can1, clkdiv_1);
    lpc_pclk(pclk_can2, clkdiv_1);
    lpc_pclk(pclk_can_flt, clkdiv_1);

    pCAN->MOD = can_mod_reset;
    pCAN->IER = 0x0; // Disable All CAN Interrupts
    pCAN->GSR = 0x0; // Clear error counters
    pCAN->CMR = 0xE; // Abort Tx, release Rx, clear data over-run

    /**
     * About the AFMR register :
     *                      B0              B1
     * Filter Mode |    AccOff bit  |   AccBP bit   |   CAN Rx interrupt
     * Off Mode             1               0           No messages accepted
     * Bypass Mode          X               1           All messages accepted
     * FullCAN              0               0           HW acceptance filtering
     */
    LPC_CANAF->AFMR = afmr_disabled;

    // Clear pending interrupts and the CAN Filter RAM
    LPC_CANAF_RAM->mask[0] = pCAN->ICR;
    memset((void*)&(LPC_CANAF_RAM->mask[0]), 0, sizeof(LPC_CANAF_RAM->mask));

    /* Zero out the filtering registers */
    LPC_CANAF->SFF_sa     = 0;
    LPC_CANAF->SFF_GRP_sa = 0;
    LPC_CANAF->EFF_sa     = 0;
    LPC_CANAF->EFF_GRP_sa = 0;
    LPC_CANAF->ENDofTable = 0;

    /* Do not accept any messages until CAN filtering is enabled */
    LPC_CANAF->AFMR = afmr_disabled;

    /* Set the baud-rate. You can verify the settings by visiting:
     * http://www.kvaser.com/en/support/bit-timing-calculator.html
     */
    do {
        const uint32_t baudDiv = sys_get_cpu_clock() / (1000 * baudrate_kbps);
        const uint32_t SJW = 3;
        const uint32_t SAM = 0;
        uint32_t BRP = 0, TSEG1 = 0, TSEG2 = 0, NT = 0;

        /* Calculate suitable nominal time value
         * NT (nominal time) = (TSEG1 + TSEG2 + 3)
         * NT <= 24
         * TSEG1 >= 2*TSEG2
         */
        failed = true;
        for(NT=24; NT > 0; NT-=2) {
            if ((baudDiv % NT)==0) {
                BRP = baudDiv / NT - 1;
                NT--;
                TSEG2 = (NT/3) - 1;
                TSEG1 = NT -(NT/3) - 1;
                failed = false;
                break;
            }
        }

        if (!failed) {
            pCAN->BTR  = (SAM << 23) | (TSEG2<<20) | (TSEG1<<16) | (SJW<<14) | BRP;
            // CANx->BTR = 0x002B001D; // 48Mhz 100Khz
        }
    } while (0);

    /* If everything okay so far, enable the CAN interrupts */
    if (!failed) {
        /* At minimum, we need Rx/Tx interrupts */
        pCAN->IER = (intr_rx | intr_all_tx);

        /* Enable BUS-off interrupt and callback if given */
        if (bus_off_cb) {
            pStruct->bus_error = bus_off_cb;
            pCAN->IER |= g_can_bus_err_intr;
        }
        /* Enable data-overrun interrupt and callback if given */
        if (data_ovr_cb) {
            pStruct->data_overrun = data_ovr_cb;
            pCAN->IER |= intr_ovrn;
        }

        /* Finally, enable the actual CPU core interrupt */
        vTraceSetISRProperties(CAN_IRQn, "CAN", IP_can);
        NVIC_EnableIRQ(CAN_IRQn);
    }

    /* return true if all is well */
    return (false == failed);
}


bool CAN_tx (can_t can, can_msg_t *pCanMsg, uint32_t timeout_ms)
{
    if (!CAN_VALID(can) || !pCanMsg || CAN_is_bus_off(can)) {
        return false;
    }

    bool ok = false;
    can_struct_t *pStruct = CAN_STRUCT_PTR(can);
    LPC_CAN_TypeDef *CANx = pStruct->pCanRegs;

    /* Try transmitting to one of the available buffers */
    taskENTER_CRITICAL();
    do {
        ok = CAN_tx_now(pStruct, pCanMsg);
    } while(0);
    taskEXIT_CRITICAL();

    /* If HW buffer not available, then just queue the message */
    if (!ok) {
        if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
            ok = xQueueSend(pStruct->txQ, pCanMsg, OS_MS(timeout_ms));
        }
        else {
            ok = xQueueSend(pStruct->txQ, pCanMsg, 0);
        }

        /* There is possibility that before we queued the message, we got interrupted
         * and all hw buffers were emptied meanwhile, and our queued message will now
         * sit in the queue forever until another Tx interrupt takes place.
         * So we dequeue it here if all are empty and send it over.
         */
        taskENTER_CRITICAL();
        do {
            can_msg_t msg;
            if (tx_all_avail == (CANx->SR & tx_all_avail) &&
                xQueueReceive(pStruct->txQ, &msg, 0)
            ) {
                ok = CAN_tx_now(pStruct, &msg);
            }
        } while(0);
        taskEXIT_CRITICAL();
    }

    return ok;
}

bool CAN_rx (can_t can, can_msg_t *pCanMsg, uint32_t timeout_ms)
{
    bool ok = false;

    if (CAN_VALID(can) && pCanMsg)
    {
        if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
            ok = xQueueReceive(CAN_STRUCT_PTR(can)->rxQ, pCanMsg, OS_MS(timeout_ms));
        }
        else {
            uint64_t msg_timeout = sys_get_uptime_ms() + timeout_ms;
            while (! (ok = xQueueReceive(CAN_STRUCT_PTR(can)->rxQ, pCanMsg, 0))) {
                if (sys_get_uptime_ms() > msg_timeout) {
                    break;
                }
            }
        }
    }

    return ok;
}

bool CAN_is_bus_off(can_t can)
{
    const uint32_t bus_off_mask = (1 << 7);
    return (!CAN_VALID(can)) ? true : !! (CAN_STRUCT_PTR(can)->pCanRegs->GSR & bus_off_mask);
}

void CAN_reset_bus(can_t can)
{
    if (CAN_VALID(can)) {
        CAN_STRUCT_PTR(can)->pCanRegs->MOD = can_mod_reset;

        #if CAN_TESTING
            CAN_STRUCT_PTR(can)->pCanRegs->MOD = can_mod_selftest;
        #else
            CAN_STRUCT_PTR(can)->pCanRegs->MOD = can_mod_normal_tpm;
        #endif
    }
}

uint16_t CAN_get_rx_watermark(can_t can)
{
    return CAN_VALID(can) ? CAN_STRUCT_PTR(can)->rxQWatermark : 0;
}

uint16_t CAN_get_tx_watermark(can_t can)
{
    return CAN_VALID(can) ? CAN_STRUCT_PTR(can)->txQWatermark : 0;
}

uint16_t CAN_get_tx_count(can_t can)
{
    return CAN_VALID(can) ? CAN_STRUCT_PTR(can)->txMsgCount : 0;
}

uint16_t CAN_get_rx_count(can_t can)
{
    return CAN_VALID(can) ? CAN_STRUCT_PTR(can)->rxMsgCount : 0;
}

uint16_t CAN_get_rx_dropped_count(can_t can)
{
    return CAN_VALID(can) ? CAN_STRUCT_PTR(can)->droppedRxMsgs : 0;
}

void CAN_bypass_filter_accept_all_msgs(void)
{
    LPC_CANAF->AFMR = afmr_bypass;
}

can_std_id_t CAN_gen_sid(can_t can, uint16_t id)
{
    /* SCC in datasheet is defined as can controller - 1 */
    const uint16_t scc = (can);
    can_std_id_t ret;

    ret.can_num = scc;
    ret.disable = (0xffff == id) ? 1 : 0;
    ret.fc_intr = 0;
    ret.id = id;

    return ret;
}

can_ext_id_t CAN_gen_eid(can_t can, uint32_t id)
{
    /* SCC in datasheet is defined as can controller - 1 */
    const uint16_t scc = (can);
    can_ext_id_t ret;

    ret.can_num = scc;
    ret.id = id;

    return ret;
}

bool CAN_fullcan_add_entry(can_t can, can_std_id_t id1, can_std_id_t id2)
{
    /* Return if invalid CAN given */
    if (!CAN_VALID(can)) {
        return false;
    }

    /* Check for enough room for more FullCAN entries
     * Each entry takes 2-byte entry, and 12-byte message RAM.
     */
    const uint16_t existing_entries = CAN_fullcan_get_num_entries();
    const uint16_t size_per_entry = sizeof(can_std_id_t) + sizeof(can_fullcan_msg_t);
    if ((existing_entries * size_per_entry) >= sizeof(LPC_CANAF_RAM->mask)) {
        return false;
    }

    /* Locate where we should write the next entry */
    uint8_t *base = (uint8_t*) &(LPC_CANAF_RAM->mask[0]);
    uint8_t *next_entry_ptr = base + LPC_CANAF->SFF_sa;

    /* Copy the new entry into the RAM filter */
    LPC_CANAF->AFMR = afmr_disabled;
    do {
        const uint32_t entries = ((uint32_t) id2.raw & UINT16_MAX) | ((uint32_t) id1.raw << 16);
        * (uint32_t*) (next_entry_ptr) = entries;

        /* The new start of Standard Frame Filter is after the two new entries */
        const uint32_t new_sff_sa = LPC_CANAF->SFF_sa + sizeof(id1) + sizeof(id2);
        LPC_CANAF->SFF_sa = new_sff_sa;

        /* Next filters start at SFF_sa (they are disabled) */
        LPC_CANAF->SFF_GRP_sa = new_sff_sa;
        LPC_CANAF->EFF_sa     = new_sff_sa;
        LPC_CANAF->EFF_GRP_sa = new_sff_sa;
        LPC_CANAF->ENDofTable = new_sff_sa;
    } while(0);
    LPC_CANAF->AFMR = afmr_fullcan;

    return true;
}

can_fullcan_msg_t* CAN_fullcan_get_entry_ptr(can_std_id_t fc_id)
{
    /* Number of entries depends on how far SFF_sa is from base of 0 */
    const uint16_t num_entries = CAN_fullcan_get_num_entries();
    uint16_t idx = 0;

    /* The FullCAN entries are at the base of the CAN RAM */
    const can_std_id_t *id_list = (can_std_id_t*) &(LPC_CANAF_RAM->mask[0]);

    /* Find the standard ID entered into the RAM
     * Once we find the ID, its message's RAM location is after
     * LPC_CANAF->ENDofTable based on the index location.
     *
     * Note that due to MSB/LSB of the CAN RAM, we check in terms of 16-bit WORDS
     * and LSB word match means we will find it at index + 1, and MSB word match
     * means we will find it at the index.
     */
    for (idx = 0; idx < num_entries; idx+=2) {
        if (id_list[idx].id == fc_id.id) {
            ++idx;
            break;
        }
        if (id_list[idx+1].id == fc_id.id) {
            break;
        }
    }

    can_fullcan_msg_t *real_entry = NULL;
    if (idx < num_entries) {
        /* If we find an index, we have to convert it to the actual message pointer */
        can_fullcan_msg_t *base_msg_entry = (can_fullcan_msg_t*)
                                                   (((uint8_t*) &(LPC_CANAF_RAM->mask[0])) + LPC_CANAF->ENDofTable);
        real_entry = (base_msg_entry + idx);
    }

    return real_entry;
}

bool CAN_fullcan_read_msg_copy(can_fullcan_msg_t *pMsg, can_fullcan_msg_t *pMsgCopy)
{
    const uint8_t *can_ram_base = (uint8_t*) &(LPC_CANAF_RAM->mask[0]);
    const uint8_t *start = can_ram_base + LPC_CANAF->ENDofTable;        // Actual FullCAN msgs are stored after this
    const uint8_t *end   = can_ram_base + sizeof(LPC_CANAF_RAM->mask);  // Last byte of CAN RAM + 1
    bool new_msg_received = false;

    /* Validate the input pointers.  pMsg must be within range of our RAM filter
     * where the actual FullCAN message should be stored at
     */
    const uint8_t *ptr = (uint8_t*) pMsg;
    if (ptr < start || ptr >= end || !pMsgCopy) {
        return false;
    }

    /* If semaphore bits change, then HW has updated the message so read it again.
     * After HW writes new message, semaphore bits are changed to 0b11.
     */
    while (0 != pMsg->semphr) {
        new_msg_received  = true;
        pMsg->semphr = 0;
        *pMsgCopy = *pMsg;
    }

    return new_msg_received;
}

uint8_t CAN_fullcan_get_num_entries(void)
{
    return LPC_CANAF->SFF_sa / sizeof(can_std_id_t);
}

bool CAN_setup_filter(const can_std_id_t *std_id_list,           uint16_t sid_cnt,
                      const can_std_grp_id_t *std_group_id_list, uint16_t sgp_cnt,
                      const can_ext_id_t *ext_id_list,           uint16_t eid_cnt,
                      const can_ext_grp_id_t *ext_group_id_list, uint16_t egp_cnt)
{
    bool ok = true;
    uint32_t i = 0;
    uint32_t temp32 = 0;

    // Count of standard IDs must be even
    if (sid_cnt & 1) {
        return false;
    }

    LPC_CANAF->AFMR = afmr_disabled;
    do {
        /* Filter RAM is after the FulLCAN entries */
        uint32_t can_ram_base_addr = (uint32_t) &(LPC_CANAF_RAM->mask[0]);

        /* FullCAN entries take up 2 bytes each at beginning RAM, and 12-byte sections at the end */
        const uint32_t can_ram_end_addr  = can_ram_base_addr + sizeof(LPC_CANAF_RAM->mask) -
                        ( sizeof(can_fullcan_msg_t) * CAN_fullcan_get_num_entries());

        /* Our filter RAM is after FullCAN entries */
        uint32_t *ptr = (uint32_t*) (can_ram_base_addr + LPC_CANAF->SFF_sa);

        /* macro to swap top and bottom 16-bits of 32-bit DWORD */
        #define CAN_swap32(t32) do {                    \
                        t32 = (t32 >> 16) | (t32 << 16);\
        } while (0)

        /**
         * Standard ID list and group list need to swapped otherwise setting the wrong
         * filter will make the CAN ISR go into a loop for no apparent reason.
         * It looks like the filter data is motorolla big-endian format.
         * See "configuration example 5" in CAN chapter.
         */
        #define CAN_add_filter_list(list, ptr, end, cnt, entry_size, swap)  \
                do { if (NULL != list) {                              \
                     if ((uint32_t)ptr + (cnt * entry_size) < end) {  \
                         for (i = 0; i < (cnt * entry_size)/4; i++) { \
                             if(swap) {                               \
                                 temp32 = ((uint32_t*)list) [i];      \
                                 CAN_swap32(temp32);                  \
                                 ptr[i] = temp32;                     \
                             }                                        \
                             else {                                   \
                                 ptr[i] = ((uint32_t*)list) [i];      \
                             }                                        \
                         }                                            \
                         ptr += (cnt * entry_size)/4;                 \
                } else { ok = false; } } } while(0)

        /* The sa (start addresses) are byte address offset from CAN RAM
         * and must be 16-bit (WORD) aligned
         * LPC_CANAF->SFF_sa should already be setup by FullCAN if used, or
         * set to zero by the can init function.
         */
        CAN_add_filter_list(std_id_list, ptr, can_ram_end_addr, sid_cnt, sizeof(can_std_id_t), true);

        LPC_CANAF->SFF_GRP_sa = ((uint32_t)ptr - can_ram_base_addr);
        CAN_add_filter_list(std_group_id_list, ptr, can_ram_end_addr, sgp_cnt, sizeof(can_std_grp_id_t), true);

        LPC_CANAF->EFF_sa = ((uint32_t)ptr - can_ram_base_addr);
        CAN_add_filter_list(ext_id_list, ptr, can_ram_end_addr, eid_cnt, sizeof(can_ext_id_t), false);

        LPC_CANAF->EFF_GRP_sa = ((uint32_t)ptr - can_ram_base_addr);
        CAN_add_filter_list(ext_group_id_list, ptr, can_ram_end_addr, egp_cnt, sizeof(can_ext_grp_id_t), false);

        /* End of table is where the FullCAN messages are stored */
        LPC_CANAF->ENDofTable = ((uint32_t)ptr - can_ram_base_addr);
    } while(0);

    /* If there was no FullCAN entry, then SFF_sa will be zero.
     * If it was zero, we just enable the AFMR, but if it was not zero, that means
     * FullCAN entry was added, so we restore AMFR to fullcan enable
     */
    LPC_CANAF->AFMR = (0 == LPC_CANAF->SFF_sa) ? afmr_enabled : afmr_fullcan;

    return ok;
}

#if CAN_TESTING
#include <printf_lib.h>
#define CAN_ASSERT(x)   if (!(x)) { u0_dbg_printf("Failed at %i, BUS: %s MOD: 0x%08x, GSR: 0x%08x\n"\
                                           "IER/ICR: 0x%08X/0x%08x BTR: 0x%08x"\
                                           "\nLine %i: %s\n", __LINE__, \
                                           CAN_is_bus_off(can1) ? "OFF" : "ON", \
                                           (int)LPC_CAN1->MOD, (int)LPC_CAN1->GSR, \
                                           (int)LPC_CAN1->IER, (int)LPC_CAN1->ICR, \
                                           (int)LPC_CAN1->BTR, \
                                           __LINE__, #x); return false; }
void CAN_test_bufoff_cb(uint32_t d)
{
    u0_dbg_printf("CB: BUS OFF\n");
}
void CAN_test_bufovr_cb(uint32_t d)
{
    u0_dbg_printf("CB: DATA OVR\n");
}

bool CAN_test(void)
{
    uint32_t i = 0;

    #define can_test_msg(msg, id, rxtrue) do {              \
            u0_dbg_printf("Send ID: 0x%08X\n", id);         \
            msg.msg_id = id;                                \
            CAN_ASSERT(CAN_tx(can1, &msg, 0));              \
            msg.msg_id = 0;                                 \
            CAN_ASSERT(rxtrue == CAN_rx(can1, &msg, 10));   \
            if (rxtrue) CAN_ASSERT(id == msg.msg_id);       \
         } while(0)

    u0_dbg_printf("  Test init()\n");
    CAN_ASSERT(!CAN_init(can_max, 100, 0, 0, NULL, NULL));
    CAN_ASSERT(CAN_init(can1, 100, 5, 5, CAN_test_bufoff_cb, CAN_test_bufovr_cb));
    CAN_ASSERT(LPC_CAN1->MOD == can_mod_reset);
    CAN_bypass_filter_accept_all_msgs();

    CAN_ASSERT(g_can_rx_qs[0] != NULL);
    CAN_ASSERT(g_can_tx_qs[0] != NULL);
    CAN_ASSERT(LPC_CANAF->SFF_sa     == 0);
    CAN_ASSERT(LPC_CANAF->SFF_GRP_sa == 0);
    CAN_ASSERT(LPC_CANAF->EFF_sa     == 0);
    CAN_ASSERT(LPC_CANAF->EFF_GRP_sa == 0);
    CAN_ASSERT(LPC_CANAF->ENDofTable == 0);

    CAN_reset_bus(can1);
    CAN_ASSERT(LPC_CAN1->MOD == can_mod_selftest);

    /* Create a message, and test tx with bad input */
    uint32_t id = 0x100;
    can_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.frame = 0;
    msg.msg_id = id;
    msg.frame_fields.is_29bit = 0;
    msg.frame_fields.data_len = 8;
    msg.data.qword = 0x1122334455667788;
    CAN_ASSERT(!CAN_tx(can_max, &msg, 0));  // Invalid CAN
    CAN_ASSERT(!CAN_rx(can1, NULL, 0));     // Invalid message pointer

    /* Send msg and test receive */
    u0_dbg_printf("  Test Tx/Rx\n");
    can_test_msg(msg, 0x100, true);
    can_test_msg(msg, 0x200, true);
    can_test_msg(msg, 0x300, true);
    can_test_msg(msg, 0x400, true);
    can_test_msg(msg, 0x500, true);

    const can_std_id_t slist[]      = { CAN_gen_sid(can1, 0x100), CAN_gen_sid(can1, 0x110),   // 2 entries
                                        CAN_gen_sid(can1, 0x120), CAN_gen_sid(can1, 0x130)    // 2 entries
    };
    const can_std_grp_id_t sglist[] = { {CAN_gen_sid(can1, 0x200), CAN_gen_sid(can1, 0x210)}, // Group 1
                                      {CAN_gen_sid(can1, 0x220), CAN_gen_sid(can1, 0x230)}  // Group 2
    };
    const can_ext_id_t elist[]      = { CAN_gen_eid(can1, 0x7500), CAN_gen_eid(can1, 0x8500)};
    const can_ext_grp_id_t eglist[] = { {CAN_gen_eid(can1, 0xA000), CAN_gen_eid(can1, 0xB000)} }; // Group 1

    /* Test filter setup */
    u0_dbg_printf("  Test filter setup\n");
    CAN_setup_filter(slist, 4, sglist, 2, elist, 2, eglist, 1);

    /* We use offset of zero if 2 FullCAN messages are added, otherwise 4 if none were added above */
    const uint8_t offset = 4;
    CAN_ASSERT(LPC_CANAF->SFF_sa     == 4  - offset);
    CAN_ASSERT(LPC_CANAF->SFF_GRP_sa == 12 - offset);
    CAN_ASSERT(LPC_CANAF->EFF_sa     == 20 - offset);
    CAN_ASSERT(LPC_CANAF->EFF_GRP_sa == 28 - offset);
    CAN_ASSERT(LPC_CANAF->ENDofTable == 36 - offset);
    for ( i = 0; i < 10; i++) {
        u0_dbg_printf("%2i: 0x%08X\n", i, (uint32_t) LPC_CANAF_RAM->mask[i]);
    }

    /* Send a message defined in filter */
    u0_dbg_printf("  Test filter messages\n");
    msg.frame = 0;
    msg.frame_fields.is_29bit = 0;
    msg.frame_fields.data_len = 8;
    msg.data.qword = 0x1122334455667788;

    /* Test reception of messages defined in the filter */
    u0_dbg_printf("  Test message reception according to filter\n");
    msg.frame_fields.is_29bit = 0;
    can_test_msg(msg, 0x100, true);   // standard id
    can_test_msg(msg, 0x110, true);   // standard id
    can_test_msg(msg, 0x120, true);   // standard id
    can_test_msg(msg, 0x130, true);   // standard id
    can_test_msg(msg, 0x200, true);   // Start of standard ID group
    can_test_msg(msg, 0x210, true);   // Last of standard ID group
    can_test_msg(msg, 0x220, true);   // Start of standard ID group
    can_test_msg(msg, 0x230, true);   // Last of standard ID group

    msg.frame_fields.is_29bit = 1;
    can_test_msg(msg, 0x7500, true);   // extended id
    can_test_msg(msg, 0x8500, true);   // extended id
    can_test_msg(msg, 0xA000, true);   // extended id group start
    can_test_msg(msg, 0xB000, true);   // extended id group end

    u0_dbg_printf("  Test messages that should not be received\n");
    /* Send a message not defined in filter */
    msg.frame_fields.is_29bit = 0;
    can_test_msg(msg, 0x0FF, false);
    can_test_msg(msg, 0x111, false);
    can_test_msg(msg, 0x131, false);
    can_test_msg(msg, 0x1FF, false);
    can_test_msg(msg, 0x211, false);
    can_test_msg(msg, 0x21f, false);
    can_test_msg(msg, 0x231, false);

    msg.frame_fields.is_29bit = 1;
    can_test_msg(msg, 0x7501, false);
    can_test_msg(msg, 0x8501, false);
    can_test_msg(msg, 0xA000-1, false);
    can_test_msg(msg, 0xB000+1, false);

    /* Test FullCAN */
    u0_dbg_printf("  Test FullCAN\n");
    CAN_init(can1, 100, 5, 5, CAN_test_bufoff_cb, CAN_test_bufovr_cb);
    CAN_reset_bus(can1);
    id = 0x100;
    CAN_ASSERT(0 == CAN_fullcan_get_num_entries());

    CAN_ASSERT(CAN_fullcan_add_entry(can1, CAN_gen_sid(can1, id), CAN_gen_sid(can1, id+1)));
    CAN_ASSERT(2 == CAN_fullcan_get_num_entries());
    CAN_ASSERT(LPC_CANAF->SFF_sa     == 4);
    CAN_ASSERT(LPC_CANAF->SFF_GRP_sa == 4);
    CAN_ASSERT(LPC_CANAF->EFF_sa     == 4);
    CAN_ASSERT(LPC_CANAF->EFF_GRP_sa == 4);
    CAN_ASSERT(LPC_CANAF->ENDofTable == 4);

    CAN_ASSERT(CAN_fullcan_add_entry(can1, CAN_gen_sid(can1, id+2), CAN_gen_sid(can1, id+3)));
    CAN_ASSERT(4 == CAN_fullcan_get_num_entries());
    CAN_ASSERT(LPC_CANAF->SFF_sa     == 8);

    for ( i = 0; i < 3; i++) {
        u0_dbg_printf("%2i: 0x%08X\n", i, (uint32_t) LPC_CANAF_RAM->mask[i]);
    }

    can_fullcan_msg_t *fc1 = CAN_fullcan_get_entry_ptr(CAN_gen_sid(can1, id));
    can_fullcan_msg_t *fc2 = CAN_fullcan_get_entry_ptr(CAN_gen_sid(can1, id+1));
    can_fullcan_msg_t *fc3 = CAN_fullcan_get_entry_ptr(CAN_gen_sid(can1, id+2));
    can_fullcan_msg_t *fc4 = CAN_fullcan_get_entry_ptr(CAN_gen_sid(can1, id+3));
    CAN_ASSERT((LPC_CANAF_RAM_BASE + LPC_CANAF->SFF_sa) == (uint32_t)fc1);
    CAN_ASSERT((LPC_CANAF_RAM_BASE + LPC_CANAF->SFF_sa + 1*sizeof(can_fullcan_msg_t)) == (uint32_t)fc2);
    CAN_ASSERT((LPC_CANAF_RAM_BASE + LPC_CANAF->SFF_sa + 2*sizeof(can_fullcan_msg_t)) == (uint32_t)fc3);
    CAN_ASSERT((LPC_CANAF_RAM_BASE + LPC_CANAF->SFF_sa + 3*sizeof(can_fullcan_msg_t)) == (uint32_t)fc4);

    can_fullcan_msg_t fc_temp;
    CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc1, &fc_temp));
    CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc2, &fc_temp));
    CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc3, &fc_temp));
    CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc4, &fc_temp));

    /* Send message, see if fullcan captures it */
    msg.frame = 0;
    msg.msg_id = id;
    msg.frame_fields.is_29bit = 0;
    msg.frame_fields.data_len = 8;

    #define can_test_fullcan_msg(fc, msg_copy, id)              \
        do {                                                    \
        msg.msg_id = id;                                        \
        CAN_ASSERT(CAN_tx(can1, &msg, 0));                      \
        CAN_ASSERT(!CAN_rx(can1, &msg, 10));                    \
        CAN_ASSERT(CAN_fullcan_read_msg_copy(fc, &msg_copy));   \
        CAN_ASSERT(fc->msg_id == id)                            \
        } while(0)
    can_test_fullcan_msg(fc1, fc_temp, id+0);   CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc2, &fc_temp));
    can_test_fullcan_msg(fc2, fc_temp, id+1);   CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc3, &fc_temp));
    can_test_fullcan_msg(fc3, fc_temp, id+2);   CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc4, &fc_temp));
    can_test_fullcan_msg(fc4, fc_temp, id+3);   CAN_ASSERT(!CAN_fullcan_read_msg_copy(fc1, &fc_temp));

    u0_dbg_printf("  \n--> All tests successful! <--\n");
    return true;
}
#endif

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

/**
 * @file
 * @brief This provides simplified way to send/receive data from Nordic wireless chip.
 *          ALL WIRELESS DATA should be sent/received through this API.
 * @ingroup  WIRELESS
 */
#ifndef WIRELESS_H__
#define WIRELESS_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include "src/mesh.h"
#include "src/mesh_typedefs.h"



/**
 * Initializes nordic driver layer and the mesh network layer.
 * This is already called before main() function is run.
 * @returns true if everything is successful.
 */
bool wireless_init(void);

/**
 * Calls mesh_service() routine.
 * This is encapsulated because this function will only call mesh_service() when there
 * is either a pending packet or a packet received as indicated by nordic interrupt.
 * This function is called through RIT or through FreeRTOS tick hook, so you don't have
 * to call it yourself.
 */
void wireless_service(void);

/// Just a wrapper around mesh_is_ack_required() to put all wireless related API at this file.
static inline bool wireless_is_ack_required(mesh_packet_t *pkt) {
    return mesh_is_ack_required(pkt);
}

/// Just a wrapper around mesh_send() to put all wireless related API at this file.
static inline bool wireless_send(uint8_t dst_addr, mesh_protocol_t protocol, const void *data, uint8_t len, uint8_t max_hops) {
    return mesh_send(dst_addr, protocol, data, len, max_hops);
}

/// Just a wrapper around mesh_send_formed_pkt() to put all wireless related API at this file.
static inline bool wireless_send_formed_pkt(mesh_packet_t *pkt) {
    return mesh_send_formed_pkt(pkt);
}

/// Just a wrapper around mesh_form_pkt() to put all wireless related API at this file.
#define wireless_form_pkt(pkt, dst, protocol, max_hops, num_ptrs, args...)\
            mesh_form_pkt(pkt, dst, protocol, max_hops, num_ptrs, args)

/// Just a wrapper around mesh_deform_pkt() to put all wireless related API at this file.
#define wireless_deform_pkt(pkt, num_ptrs, args...)\
            mesh_deform_pkt(pkt, num_ptrs, args)

/**
 * Call this function periodically to get a queued packet.
 * @param timeout_ms  The number of milliseconds to wait for a packet.  If FreeRTOS is
 *                     running, then this becomes the queue wait time.
 * @returns true if a packet was dequeued, or false if there is no packet.
 *
 * Here is an example of how to send a packet and ensure the destined node received the data :
 * @code
 *      wireless_send(destination, mesh_pkt_ack, "Hello", 5, 3);
 *      mesh_packet_t rx;
 *
 *      if (wireless_get_ack_pkt(&rx, 100)) {
 *          // We got an ACK, but to be sure it came from the right place, check:
 *          //     if(rx.nwk.src == destination)
 *      }
 * @endcode
 */
char wireless_get_rx_pkt (mesh_packet_t *pkt, const uint32_t timeout_ms);

/// Same as wireless_get_rx_pkt(), except this will retrieve an ACK response
char wireless_get_ack_pkt(mesh_packet_t *pkt, const uint32_t timeout_ms);

/// Flush all received data of mesh (ACKs and RX packets)
/// @returns the discarded packet count
int wireless_flush_rx(void);



#ifdef __cplusplus
}
#endif
#endif /* WIRELESS_H__ */

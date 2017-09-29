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
 * @brief      Simple Mesh Network Algorithm.
 * @ingroup    WIRELESS
 * VERSION:    @see mesh_config.h version and change history information.
 *
 *   @par Simple Mesh Algorithm for low powered radios.
 *   This library has small footprint with minimal code and RAM requirements.
 *   Each node can be a repeater node, and contains its own routing table.
 *
 *   @par Types of Packets
 *   	- NACK : "Fire-and-Forget" packet, no acknowledgment from receiver.
 *   	- ACK  : Receiver will auto-acknowledge, and packet is retransmitted if ACK not received.
 *   	- Broadcast, and Application ACK
 *
 *	@par Example Initialization Code:
 *	Assuming the radio functions and application receive function is provided, init() code is simple :
 *
 *	@code
 *	    // Example mesh callback function :
 *	    int my_radio_init(void *data, int len)
 *	    {
 *	        // Initialize your radio chip.
 *	    }
 *
 *      // Example timer function
 *      int my_timer_get(void *pData, int len) {
 *          const char ok = (sizeof(uint32_t) == len);           // Size passed in will be uint32_t
 *          const uint32_t timerValueMs = your_sys_timer_get();  // Get your system timer value
 *          if (ok) {
 *              uint32_t *timer = (uint32_t*)pData;
 *              *timer = timerValueMs;
 *          }
 *          return ok;
 *      }
 *
 *		Mesh_DriverStruct driver;
 *		driver.radio_init = my_radio_init;
 *		driver.radio_send = my_radio_send;
 *		driver.radio_recv = my_radio_recv;
 *		driver.app_recv   = my_app_recv;
 *		driver.get_timer  = my_timer_get;
 *
 *		char my_node_id = 0x10;
 *		mesh_init(my_node_id, true, &driver, true);
 *	@endcode
 *
 *
 *	@par Sample code for a node receiving data :
 *	@code
 *	    // Global variables
 *		mesh_packet_t packet;
 *		bool rx = false;
 *
 *		// Application receive callback
 *		int my_app_recv(void *data, int len)
 *		{
 *          memcpy(&packet, data, len);
 *          rx = true;
 *		}
 *
 *      // main loop :
 *		while(1) {
 *			mesh_service();
 *
 *			if(rx) {
 *			    rx = false;
 *				if(mesh_is_ack_required(&packet)) {
 *					mesh_send_ack("Hello Back", 10, &packet);
 *				}
 *			}
 *		}
 *	@endcode
 *
 *  @warning Care needs to be taken for single radio systems when they send out a packet.
 *  You don't want the mesh route discovery packet to be repeated at the same time by the
 *  nearby nodes because their data will collide and nothing will go through.  The radio
 *  send function can then be modified with logic similar to :
 *  @code
 *  char my_radio_send(char* pData, int len)
 *  {
 *    if (mesh_get_node_address() != pkt->nwk.src) {
 *      if (MESH_ZERO_ADDR == pkt->mac.dst) {
 *          const uint32_t timeSlotDelayUs = ((rand() % slots) + 1) * pkt_air_time_us;
 *          delay_us(timeSlotDelayUs); ///< Maximize mesh nodes to repeat the packet and not collide.
 *      }
 *    }
 *  }
 *  @endcode
 */
#ifndef MESH_NET_H__
#define MESH_NET_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "mesh_typedefs.h"



/**
 * Initializes the Mesh Network.
 * @param local_node_id  Node ID of your local node.
 * @param is_mesh_node   true, if this node can participate in Mesh to repeat packets.
 * @param node_name      Node name, maximum as large as a payload.
 * @param driver         The structure of the radio driver.
 * @param discovery      If true, then a broadcast message with data: "Hello\n" is sent to
 *                       each node within 3 hops away for discovery and route update purpose.
 * @note Unless changed otherwise, the retry count contains default value; @see mesh_set_retry_count()
 */
bool mesh_init(const uint8_t local_node_id, const bool is_mesh_node, const char *node_name,
               const mesh_driver_t driver,  const bool discovery);

/**
 * This method allows to change the node address after mesh_init() has been called.
 * @param local_node_id  Node ID of your local node.
 */
bool mesh_set_node_address(const uint8_t local_node_id);
uint8_t mesh_get_node_address(void); ///< @returns our mesh node address.

/**
 * @param count   Number of retries used for ACK packet if ACK not received; see note below.
 *                Max value is MESH_RETRY_COUNT_MAX.  Optimal value is 2 (default unless changed).
 *
 * @note The retries apply if routes do not change but the actual retry count may be higher if
 *       nodes change routes frequency.  For example, if retries are set to 2, and a node N1
 *       sends packet to N3 through N2, then the packet may be sent 6 times:
 *          - Send original packet, if no ACK, then re-send 2 more times.
 *          - Remove N3 route through N2 and re-send original packet to discover new route.
 *          - Re-send packet 2 more times if ACK still not received.
 */
void mesh_set_retry_count(const uint8_t count);

/**
 * This method should be called periodically.
 * This method gets the radio data, and runs the mesh algorithm.  When a packet is received
 * for you, the app_recv() call-back will be called.
 */
void mesh_service(void);

/**
 * Sends a new packet.
 * @param dst       The destination address to send data to.
 * @param type      The type of packet: ack, nack, app-ack @see mesh_protocol_t
 * @param pData     The pointer to the data to send.
 * @param len       The length of the data to send.
 * @param hop_count_max See the 3 use cases below :
 *  -  If this is a broadcast message, this controls the max hops packet can take.
 *  -  If the route is known, then this parameter will not have any effect.
 *  -  If the route is unknown, then this controls how far the packet can travel
 *      to search for our destined node.  You can pass MESH_HOP_COUNT_MAX if you
 *      want the mesh algorithm to figure it out at the cost of utilizing the
 *      network a little more and decreasing its efficiency.
 *
 * @returns true if the packet was sent successfully
 * @warning This function SHOULD NOT be used to send an ACK back because infinite
 *           loop of back-and-forth ACKs may occur. @see mesh_send_ack()
 * @note  This method and mesh_service() can be called from different threads because
 *          mesh_service() will skip its execution if we are inside a critical section.
 */
bool mesh_send(const uint8_t dst, const mesh_protocol_t type,
               const void* pData, const uint8_t len,
               const uint8_t hop_count_max);

/**
 * Form a packet by copying the data from the given pointers as variable arguments.
 * @see parameters of mesh_send()
 * @param num_ptrs  The number of data pointer pairs; see below.
 * @param ...       The pairs of data pointer and the size; see below.
 * @returns true if packet was formed correctly.  Error may be returned if you tried to send
 *          too much data that the payload cannot hold.
 * @warning Once a packet is formed, it should be immediately sent because packet route
 *          may change later, so the packet may not make it through.
 * @code
 *      mesh_packet_t pkt;
 *
 *      // Copy single NULL string
 *      mesh_form_pkt(&pkt, 1, mesh_pkt_ack, 1, 1, "\0", 1);
 *
 *      // Copy two strings:
 *      mesh_form_pkt(&pkt, 1, mesh_pkt_ack, 1,
 *                    2,           // Two pairs below
 *                    "hello", 5,  // First pair's data, and the size
 *                    "world", 5); // Second pair's data, and the size
 *
 *      // Copy three variables (string, int, and float):
 *      int my_int = 123;
 *      float my_flt = 1.23;
 *      mesh_form_pkt(&pkt, 1, mesh_pkt_ack, 1,
 *                    3,
 *                    "copyme", 6,
 *                    &my_int, sizeof(my_int),
 *                    &my_flt, sizeof(my_flt));
 * @endcode
 */
bool mesh_form_pkt(mesh_packet_t *pkt, const uint8_t dst,
                   const mesh_protocol_t type, const uint8_t hop_count_max,
                   uint8_t num_ptrs, ...);

/**
 * Send a packet that is already formed.
 * @param pkt The packet pointer formed by mesh_form_pkt().
 */
bool mesh_send_formed_pkt(mesh_packet_t *pkt);

/**
 * This does the opposite of mesh_form_pkt().  Instead of copying data from the
 * pointers and storing to the packet data, this will copy the data from the packet
 * and store to your pointers.
 * @param pkt       The mesh packet received.
 * @param num_ptrs  The number of pairs of pointers.
 * @return false if the packet was smaller than the data being asked to store.
 *
 * @code
 *      // Assume we got a packet with a uint32_t and a float :
 *      uint32_t my_int = 0;
 *      float my_flt = 0;
 *      mesh_deform_pkt(&pkt, 2,
 *                      &my_flt, sizeof(my_flt),
 *                      &my_int, sizeof(my_int));
 * @endcode
 */
bool mesh_deform_pkt(mesh_packet_t *pkt, uint8_t num_ptrs, ...);


/**
 * After a packet is obtained, this method should be called to check if ACK is required.
 * Use mesh_send_ack() method to send the acknowledgment back.
 * @param  pPacket  The received packet pointer.
 * @return TRUE if application is required to send ACKnowledgment back.
 */
static inline bool mesh_is_ack_required(const mesh_packet_t* pPacket)
{
    return (mesh_pkt_ack_app == pPacket->info.pkt_type);
}

/**
 * Check if the ACK has been received from a node that we sent an ACK packet to.
 * @param pRxPkt  The packet that you received
 * @param sentDstAddr  The destination address that you originally sent the packet to.
 * @returns true if the given packet is an ACK of a packet you sent earlier.
 */
static inline bool mesh_is_ack_ok(const mesh_packet_t* pRxPkt, const uint8_t sentDstAddr)
{
    return (mesh_pkt_ack_rsp == pRxPkt->info.pkt_type && pRxPkt->nwk.src == sentDstAddr);
}

/**
 * This method can be used to send application acknowledgment of received packet.
 * This simplifies sending an acknowledgment back because the user does not need to know
 * about some of the parameters for mesh_send() function.
 * @param   p_orig_pkt  The unmodified original packet that was received.
 */
static inline bool mesh_send_ack(char* pData, uint8_t len, const mesh_packet_t* p_orig_pkt)
{
    /* Response needs to be NACK otherwise loop of ACKs will occur */
    return mesh_send(p_orig_pkt->nwk.src, mesh_pkt_ack_rsp, pData, len, p_orig_pkt->info.hop_count_max);
}

/**
 * Allows user to query all the routing entries.
 * @param route_num  The route number to query : 0 - (N-1)
 * When NULL is returned, no more route exists.  If Non-NULL entry is returned, user
 * can look at destination, the route to next destination, and number of hops it is away.
 */
const mesh_rte_table_t* mesh_get_routing_entry(const uint8_t route_num);

/**
 * @return The number of routing entries this node is maintaining.
 */
uint8_t mesh_get_num_routing_entries(void);

/**
 * @return true if our routing table contains the route for the given destination address.
 */
bool mesh_is_route_known(const uint8_t addr);

/**
 * If there are is no packet received from your radio, and there are not any pending
 * packets, then mesh_service() doesn't have to be called periodically.
 *
 * @returns The number of packets that are in the pending state, means they are waiting
 *          to be acknowledged, or repeated after a timeout.
 */
uint8_t mesh_get_pnd_pkt_count(void);

/**
 * @returns the expected number of milliseconds it should take for the destination node
 *          to send us an ACK packet.  This is the most ideal time assuming no packet
 *          is lost while sending or receiving the ACK.
 *          If the route is not known, then the timeout returned is based on the assumption
 *          that it is MESH_RTE_DISCOVERY_HOPS (#define'd) hops away.
 */
uint32_t mesh_get_expected_ack_time(uint8_t node_addr);

/**
 * @returns similar to mesh_get_expected_ack_time(), but the returned value is the
 *          max expected timeout after we exhaust our retries.
 */
uint32_t mesh_get_max_timeout_before_packet_fails(uint8_t node_addr);

/**
 * @{ Mesh API to get error types and reset errors
 * Mesh layer keeps error bit fields during its operation.  The error mask can be obtained
 * and masked with mesh_error_mask_t to detect the error type, and it can be reset by this API.
 * @code
 *      mesh_error_mask_t err = mesh_get_error_mask();
 *      if (e & mesh_err_dup_node) {
 *          // Duplicate node with our address found
 *      }
 * @endcode
 */
mesh_error_mask_t mesh_get_error_mask(void); ///< @return errors encountered during mesh network operation.
void mesh_reset_error_mask(void);            ///< Resets the error mask to zero.
/** @} */

#if MESH_USE_STATISTICS
/**
 * @returns The mesh network statistics structure.
 */
mesh_stats_t mesh_get_stats(void);
#endif



#ifdef __cplusplus
}
#endif
#endif /* MESH_NET_H__ */

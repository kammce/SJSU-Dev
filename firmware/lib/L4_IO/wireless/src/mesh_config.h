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
* @brief    Mesh configuration file.
* @ingroup  WIRELESS
*/
#ifndef MESH_CONFIG_H__
#define MESH_CONFIG_H__
#ifdef __cplusplus
extern "C" {
#endif



/**
 * Each payload header contains mesh version to detect version mismatch.
 *
 * Version info :
 *   3c  - No change.  Changed all "m_" to "g_" (coding standard)
 *   3b  - No change to algorithm; added more methods:
 *          - mesh_is_ack_ok()
 *          - mesh_get_expected_ack_time()
 *          - mesh_get_max_timeout_before_packet_fails()
 *   3-  - Previous version should've been 3, so fixed this documentation error here.
 *   2b- - Each route now has a score, the route will lowest score is removed first.
 *       - If a packet comes in with unknown route, a NULL packet is sent back
 *          such that source node will know the route for next packet.
 *   2- - Added an implicit ping packet with each node having a description.
 *      - Fixed bug at mesh_deform_pkt()
 *      - Check for failure of app_recv() callback.
 *      - Macro of hop-count to use when a routed packet fails and a new
 *        route needs to be discovered.
 *  1d- Fixed a bug at mesh_handle_mesh_packet()
 *  1c- - Changed mesh_service() to not take the system time value.
 *        Instead, mesh network will obtain the time when needed from the
 *        newly added timer driver function.
 *      - Mesh network discovery packets are now queued and are only sent
 *        if the destined node is not heard until MESH_PKT_DISC_TIMEOUT_MS
 *      - Major bugfix since size of m_our_pnd_pkts_size was out of bounds.
 *  1b- Add mesh_form_packet() and mesh_send_formed_packet()
 *  1a- Minor update to add mesh_error_mask_t
 *  1 - Initial version
 */
#define MESH_VERSION                3

/**
 * The payload that your radio driver can carry.  Part of the payload
 * consists of the mesh transport header (8 bytes as of version 2).
 */
#define MESH_PAYLOAD                32

/**
 * Defines the number of buffers we use for various purposes :
 *  - Routing table consisting of destination, and source address (4 bytes each)
 *  - Previous packets to avoid duplicate transmission (3 bytes each)
 *  - Mesh packets used to retransmit a lost packet (payload + 4 bytes each)
 *
 *  The formula for the RAM requirement is :
 *  (4 * N) + (3 * N) + N*(PL + 4) + M*(PL + 4)
 *  where N = MESH_MAX_NODES
 *    and M = MESH_MAX_PEND_PKTS
 *
 *  This should be ideally the max nodes this node can communicate with.  If there
 *  are too many neighboring nodes, such as 20, a lower number like 10 can be used,
 *  however, it will decrease efficiency of the mesh network as routes may need
 *  to be rediscovered as they may be over-written.  Furthermore, the node may drop
 *  packets that it may be responsible to repeat.
 */
#define MESH_MAX_NODES              4

/**
 * This defines dedicated buffer size of OUR packets sent to others by mesh_send().
 * The pending packets' buffer is used for retry logic until an ACK has been received.
 *
 * You may send more packets than this number, but then you will compromise the packet
 * delivery (reliability) because some packets that need to be retried may be erased.
 *
 * Minimum should be 2, one for outgoing packet, and one for an ACK packet.
 */
#define MESH_MAX_PEND_PKTS          2

/**
 * @{ Mesh packet timeout and route configuration.
 *
 * MESH_ACK_TIMEOUT_MS :
 * Number of milliseconds of timeout for retry packet and packet repeat.
 * If response time is set to 5, and a node is 2 hops away, we wait 10ms for the
 * ACK from that node.  This should be a little more than the maximum time you
 * expect to send a packet, and receive an ACK back.
 *
 * For example, if it takes 500uS to send a packet, then this should be set
 * to maybe 2ms since it will also take another 500uS to get the ACK back.
 *
 * MESH_PKT_DISC_TIMEOUT_MS
 * The packet discovery timeout should be roughly half of the ACK timeout.
 * This is only used when discovering a new route.  So this is a "hold off"
 * time for intermediate node(s) to wait for destination to respond before
 * they repeat the packet hoping it will go to its final destination.
 *
 * MESH_RTE_DISCOVERY_HOPS
 * When a route is discovered, it is saved for future use.  When a packet to
 * this known route fails, the packet needs to discover a new route, and we
 * use this many hops to find its new route.  If route still fails, then the
 * user should manually send a new packet with higher max-hop-count
 */
#define MESH_ACK_TIMEOUT_MS          8  ///< Packet is retried if an ACK is not received within this time.
#define MESH_PKT_DISC_TIMEOUT_MS     4  ///< Destined node is given this time before we send repeat the packet.
#define MESH_RTE_DISCOVERY_HOPS      3  ///< Number of hops to use when a routed packet fails.
/** @} */

/**
 * @{ Special mesh addresses - Do not change these.
 */
#define MESH_BROADCAST_ADDR         0xFF
#define MESH_ZERO_ADDR              0x00
/** @} */

/**
 * Keep statistical counters and enable mesh_get_stats() method.
 * An ACK_RSP packet contains statistical data of another node at the
 * expense of using a few more bytes of RAM.
 * @see mesh_stats_t
 * @see mesh_get_stats()
 */
#define MESH_USE_STATISTICS         1

/**
 * Optionally, define the debug print method.
 */
#if 0
    #include <stdio.h>
    #include <time.h>
    #define MESH_DEBUG_PRINTF(X,...)    printf("    %li: " X "\n", time(NULL)%100, ##__VA_ARGS__)
#elif 0
    #include <stdio.h>
    #include "lpc_sys.h"
    #define MESH_DEBUG_PRINTF(X, p...)  printf("  " X " (%u)\n", ##p, (unsigned int)sys_get_uptime_ms())
#endif

/**
 * If this is set to non-zero, then the mesh.c will include unit tests
 * with the entry point being mesh_test();
 */
#define MESH_INCLUDE_TESTS          0



#ifdef __cplusplus
}
#endif
#endif /* MESH_CONFIG_H__ */

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
* @brief    Mesh type defines
* @ingroup  WIRELESS
*/
#ifndef MESH_TYPEDEFS_H_
#define MESH_TYPEDEFS_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "mesh_config.h"



#ifdef WIN32 /* For Visual Studio */
	#define __attribute__(x)
	#define packed
#else
	/* Nothing to include for GCC */
#endif

/** Typedef of Function Pointer */
typedef int(*mesh_fptr_t)(void* pData, int data_len);

/**
 * Structure of the mesh driver used to send/receive data.
 * Examples of these functions are given at mesh.h
 * The functions should return 0 upon failure.
 */
typedef struct
{
    mesh_fptr_t app_recv;   ///< Application call-back. Return true upon success.
    mesh_fptr_t get_timer;  ///< Get timer value in ms. Return true upon success.
    mesh_fptr_t radio_init; ///< Initialize the RADIO.  Return true upon success.
    mesh_fptr_t radio_send; ///< Send a packet.         Return value not checked.
    mesh_fptr_t radio_recv; ///< Receive a packet.      Return true if valid packet returned.
} mesh_driver_t;

/// Routing table type
typedef struct {
    uint8_t dst;      ///< Destination ID
    uint8_t next_hop; ///< Next destination to get to dst
    uint8_t num_hops; ///< Number of hops to dst
    uint8_t score;    ///< The score of this route (higher if used more often)
} mesh_rte_table_t;

#if MESH_USE_STATISTICS
/**
 * Statistics structure that is sent if MESH_USE_STATISTICS is set to non-zero value.
 * This data is sent by the node sending ack response packet.
 * Current size is 12 bytes, and this should be kept to a minimum such that an
 * ACK response packet can accommodate this data into the payload.
 */
typedef struct {
    uint16_t pkts_sent;             ///< Packets we sent out with our address
    uint16_t pkts_intercepted;      ///< Packets we receipted within our mesh network
    uint16_t pkts_repeated;         ///< Packets we repeated for others
    uint16_t pkts_retried;          ///< Packets we retried with our address
    uint16_t pkts_retried_others;   ///< Packets retried for others
    uint8_t rte_entries;            ///< Current number of routing entries
    uint8_t rte_overwritten;        ///< Number of times a routing entry over-wrote an existing entry
} __attribute__((packed)) mesh_stats_t;
#endif

/// Mesh packet type
typedef enum {
    mesh_pkt_nack=0,    ///< No ACK - Must be value of 0
    mesh_pkt_ack,       ///< Automatic ACK by mesh_service()
    mesh_pkt_ack_app,   ///< Application is required to ACK ASAP
    mesh_pkt_ack_rsp,   ///< Response packet of an ACK
} mesh_protocol_t;

/// Mesh error types
typedef enum {
    mesh_err_none = 0,
    mesh_err_ver_mismatch = (1 << 0), ///< A node sent data that is running different software version of this mesh network
    mesh_err_dup_node = (1 << 1),     ///< Another node with the same address is present in the network.
    mesh_err_app_recv = (1 << 2),     ///< app_recv() callback returned 0
} mesh_error_mask_t;

/**
 * Mesh packet header type
 * 4-bit hop-count gets us 15 hops max, good enough for simple mesh.
 */
typedef struct {
    uint8_t version     : 3;   ///< Mesh Version (1-7) -- MUST BE 1st member of this struct
    uint8_t retries_rem : 3;   ///< Packet retries remaining (pad to 8 bits)
    uint8_t pkt_type    : 2;   ///< The type of mesh packet @see mesh_protocol_t

    uint8_t hop_count     : 4; ///< Hop count of packet (1st packet counts as 0 hops)
    uint8_t hop_count_max : 4; ///< Max hop count limit

    uint8_t pkt_seq_num;  ///< Sequence number of the packet.
    uint8_t data_len;     ///< Length of the packet data
} __attribute__((packed)) mesh_pkt_info_t;

/// This is fixed to 15 due to the size of the hop_count and hop_count_max
#define MESH_HOP_COUNT_MAX      15

/// This field is limited to value of 7 because of the bit-field of retry count
#define MESH_RETRY_COUNT_MAX    7

/**
 * Mesh packet address type
 */
typedef struct {
    uint8_t src;    ///< Source address
    uint8_t dst;    ///< Destination address
} __attribute__((packed)) mesh_pkt_addr_t;

/// Size of the header used by the payload
#define MESH_PAYLOAD_HEADER_SIZE    (sizeof(mesh_pkt_info_t) + sizeof(mesh_pkt_addr_t) + sizeof(mesh_pkt_addr_t))

/// Size of the data payload (Do not modify)
#define MESH_DATA_PAYLOAD_SIZE      (MESH_PAYLOAD - MESH_PAYLOAD_HEADER_SIZE)

#if (MESH_PAYLOAD <= 8) /* M_TODO : How do I calculate a compile time sizeof() instead of using hard-coded value? */
#error "Mesh payload size is too small; it should be bigger than MESH_PAYLOAD_HEADER_SIZE"
#endif

/**
 * The structure of a single packet.
 */
typedef struct
{
    mesh_pkt_addr_t nwk;                    ///< Packet network address
    mesh_pkt_addr_t mac;                    ///< Packet physical address
    mesh_pkt_info_t info;                   ///< Packet header
	uint8_t data[MESH_DATA_PAYLOAD_SIZE];   ///< Actual data within the payload
} __attribute__((packed)) mesh_packet_t;



#ifdef __cplusplus
}
#endif
#endif /* MESH_TYPEDEFS_H_ */

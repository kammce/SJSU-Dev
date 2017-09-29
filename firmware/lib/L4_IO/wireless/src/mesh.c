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
#include "mesh.h"
#include <string.h>   // memcpy(), memset(), strncpy()
#include <stdarg.h>
#include <stdlib.h>



/** @{ Guard against bad mesh configuration : */
#if (MESH_MAX_NODES > 255)
#error "MESH_MAX_NODES cannot be greater than 255, logic will break due to the use of uint8_t"
#endif
#if (MESH_RESP_TIME_PER_HOP_MS*MESH_HOP_COUNT_MAX >= 65535)
#error "The ACK timeout is beyond uint16 limit in combination with MAX HOPS, try setting to lower timeout"
#endif
#if (MESH_MAX_NODES < 2)
#error "Max nodes must be 2 or more"
#endif
#if (MESH_MAX_PEND_PKTS < 2)
#error "Max pending packets should be 2 or more"
#endif
/** @} */

/** If debug not defined, define it to empty to be able to compile */
#ifndef MESH_DEBUG_PRINTF
#define MESH_DEBUG_PRINTF(x, p...)
#endif



/**********************************************/
/****            Private Typedefs          ****/
/**********************************************/
/**
 * Fields that make a packet unique are kept in history to
 * avoid duplicate packet handling.
 */
typedef struct {
    uint8_t src;     ///< Sender address
    uint8_t pkt_id;  ///< Sender's packet id (sequence)
    uint8_t retries; ///< Packet retry count
} __attribute__((packed)) mesh_pkt_history_t ;

/**
 * Mesh Pending packet type
 * Only 16-bit timer is needed which can supply up to 65,535ms
 * value, which is far above a useful mesh packet retry value
 */
typedef struct {
    mesh_packet_t pkt;        ///< The packet itself
    uint16_t timer_ms;        ///< Running time.  MUST BE UINT16 due to hard-coded usage of UINT16_MAX.
    uint16_t timeout_ms : 15; ///< Target time when timer expires.  We don't need a lot of bits for this.
    uint16_t disc_pkt : 1;    ///< Flag if this is a route discovery packet.
} __attribute__((packed)) mesh_pnd_pkt_t;

/// Macro to get size of array
#define MESH_ARRAY_SIZEOF(x)  (sizeof(x) / sizeof(x[0]))



/*************************************************************************/
/****                           Private Variables                     ****/
/**** preceded by m to mark as private member of this class (or file) ****/
/*************************************************************************/
static volatile bool g_locked = false;   ///< Simple protection for simultaneous access of data structures
static bool g_rpt_node = false;          ///< Flag if we participate in the mesh network
static uint8_t g_our_node_id = 1;        ///< Our Node ID
static uint8_t g_retry_count = 2;        ///< Number of retries for ACK packet
static mesh_driver_t g_driver = { 0 };   ///< Radio send/recv functions
static mesh_error_mask_t g_error_mask = mesh_err_none;

static char g_our_name[MESH_DATA_PAYLOAD_SIZE] = { 0 };        ///< Name of our name used for PING response
static mesh_rte_table_t g_rte_table[MESH_MAX_NODES];           ///< Our routing table entries
static mesh_pkt_history_t g_pkt_hist[MESH_MAX_NODES];          ///< Our packet history
static mesh_pkt_history_t *g_pkt_hist_wptr = &(g_pkt_hist[0]); ///< Packet history write pointer (circular write)
static mesh_pnd_pkt_t g_mesh_pnd_pkts[MESH_MAX_NODES];         ///< Pending packets of other mesh nodes
static mesh_pnd_pkt_t g_our_pnd_pkts[MESH_MAX_PEND_PKTS];      ///< Pending packets sent by us

static const uint8_t g_rte_tbl_size       = MESH_ARRAY_SIZEOF(g_rte_table);
static const uint8_t g_pkt_history_size   = MESH_ARRAY_SIZEOF(g_pkt_hist);
static const uint8_t g_mesh_pnd_pkts_size = MESH_ARRAY_SIZEOF(g_mesh_pnd_pkts);
static const uint8_t g_our_pnd_pkts_size  = MESH_ARRAY_SIZEOF(g_our_pnd_pkts);

#if MESH_USE_STATISTICS
static mesh_stats_t g_mesh_stats = { 0 };
#endif



/**********************************************/
/****            Private functions         ****/
/**********************************************/
static inline bool mesh_is_same_packet(const mesh_packet_t *p1, const mesh_packet_t *p2)
{
    return (p1->nwk.dst == p2->nwk.dst &&
            p1->nwk.src == p2->nwk.src &&
            p1->info.pkt_seq_num == p2->info.pkt_seq_num
            );
}

static uint8_t mesh_get_next_seq_num(void)
{
    /* Generate unique packet sequence number */
    static uint8_t s_next_packet_id = 0;
    return ++s_next_packet_id;
}

static void mesh_incr_soft_timers_for_arr(mesh_pnd_pkt_t *arr, const uint8_t arr_size, const uint32_t delta_time)
{
    uint32_t timer = 0;
    uint8_t i = 0;

    for (i = 0; i < arr_size; i++) {
        if (MESH_ZERO_ADDR != arr[i].pkt.nwk.dst) {
            /* Check if adding delta will overflow uint16, if so, set it to max value */
            timer = arr[i].timer_ms;
            timer += delta_time;
            arr[i].timer_ms = (timer <= UINT16_MAX) ? timer : UINT16_MAX;
        }
    }
}

/**
 * Handles the software timers used for packet retry timeout.
 * If a packet is pending, its timer is incremented by the delta computed
 * from the last time this function is called.
 */
static bool mesh_update_soft_timers(void)
{
    static uint32_t s_prev_time_ms = 0;
    uint32_t time_now_ms = 0;
    const bool ok = g_driver.get_timer(&time_now_ms, sizeof(time_now_ms));
    const uint32_t delta = (time_now_ms - s_prev_time_ms);

    s_prev_time_ms = time_now_ms;
    mesh_incr_soft_timers_for_arr(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size, delta);
    mesh_incr_soft_timers_for_arr(&g_our_pnd_pkts[0],  g_our_pnd_pkts_size,  delta);
    return ok;
}

static inline bool mesh_send_packet(mesh_packet_t *pkt)
{
    #if MESH_USE_STATISTICS
    /* If we are not the source, then we must be repeating the packet */
    if (pkt->nwk.src == g_our_node_id) {
        g_mesh_stats.pkts_sent++;
    }
    else {
        g_mesh_stats.pkts_repeated++;
    }
    #endif

    MESH_DEBUG_PRINTF("SEND TO %i THRU %i MAX HOPS %i", pkt->nwk.dst, pkt->mac.dst, pkt->info.hop_count_max);
    pkt->mac.src = g_our_node_id;
    return (g_driver.radio_send((void*)pkt, sizeof(*pkt)));
}

static void mesh_send_retry_packet(mesh_packet_t *pkt)
{
    #if MESH_USE_STATISTICS
    if (pkt->nwk.src == g_our_node_id) {
        g_mesh_stats.pkts_retried++;
    }
    else {
        g_mesh_stats.pkts_retried_others++;
    }
    #endif

    pkt->info.retries_rem--;
    mesh_send_packet(pkt);
}

/**
 * Gets the routing table entry based on destination ID.
 * If destination ID is not found in the table, NULL entry is returned
 */
static mesh_rte_table_t* mesh_find_rte_tbl_entry(const uint8_t dst_id)
{
    uint8_t i = 0;
    mesh_rte_table_t *entry = NULL;

    for (i = 0; i < g_rte_tbl_size; i++) {
        if (dst_id == g_rte_table[i].dst) {
            entry = &g_rte_table[i];
            break;
        }
    }

    return entry;
}

/**
 * Gets the routing table entry to modify based on destination ID.
 * If destination ID is found, that entry is returned to update its route.
 * If destination ID is not located, either an empty entry is returned or
 * an oldest entry is returned to be over-written.
 */
static mesh_rte_table_t* mesh_get_rte_to_modify(const uint8_t dst_id)
{
    /* Is the destination already present in the routing table? */
    mesh_rte_table_t *entry = mesh_find_rte_tbl_entry(dst_id);
    uint8_t lowest = 0;
    uint8_t i = 0;

    /* No entry found with dst_id, so find empty entry */
    if (NULL == entry) {
        entry = mesh_find_rte_tbl_entry(MESH_ZERO_ADDR);

        /* No free routing entries, over-write least used entry */
        if (NULL == entry) {
            entry = &(g_rte_table[0]);
            lowest = entry->score;
            for (i = 1; i < g_rte_tbl_size; i++) {
                if (g_rte_table[i].score < lowest) {
                    lowest = g_rte_table[i].score;
                    entry = &(g_rte_table[i]);
                }
            }
            memset(entry, 0, sizeof(*entry));

            #if MESH_USE_STATISTICS
            g_mesh_stats.rte_overwritten++;
            #endif
        }
    }

    return entry;
}

/**
 * Removes a routing table entry by destination node (if found).
 */
static inline void mesh_remove_rte_entry(const uint8_t dst_node_id)
{
    mesh_rte_table_t *entry = mesh_find_rte_tbl_entry(dst_node_id);
    if (NULL != entry) {
        memset(entry, 0, sizeof(*entry));
    }
}


/**
 * Updates the routing score of the given entry; but NULL entry means a NOP
 * If the given entries' score reaches max value, everyone's score is reduced
 * to half.  We may get into a situation when repeated traffic of one node
 * will zero out scores of other nodes, but the extra logic to handle this
 * less recurring case is not worth the effort :)
 * Remember KISS: Keep it simple stupid!
 */
static void mesh_update_rte_scores(mesh_rte_table_t *entry)
{
    uint8_t i = 0;

    if (NULL != entry) {
        /* If max value reached for the score, reduce everyone's score */
        if (UINT8_MAX == ++(entry->score)) {
            for (i = 0; i < g_rte_tbl_size; i++) {
                g_rte_table[i].score /= 2;
            }
        }
    }
}

/**
 * Finds a possible free slot from the pending packet array.
 * If a slot is not found, then the slot with least amount of retries+timeout is returned
 * assuming that the slot with highest retries+timeout will need to be retransmitted.
 */
static mesh_pnd_pkt_t* mesh_get_pnd_pkt_slot(mesh_pnd_pkt_t *arr, const uint8_t size_of_array)
{
    uint8_t i = 0;
    mesh_pnd_pkt_t *entry = NULL;
    uint32_t pkt_timeout = 0;
    uint32_t highest_timeout = 0;

    /* Find the free entry marked by nwk.dst being 0 */
    for (i = 0; i < size_of_array; i++) {
        if (MESH_ZERO_ADDR == arr[i].pkt.nwk.dst) {
            entry = &arr[i];
            break;
        }
    }

    /* If no free pending packets, then :
     * Find the entry with highest retries remaining because it is likely that an entry with
     * least retries remaining will need to be retried.
     */
    if (NULL == entry) {
        entry = &arr[0];

        for (i = 0; i < size_of_array; i++) {
            pkt_timeout = (arr[i].pkt.info.retries_rem);
            pkt_timeout <<= sizeof(arr[i].timer_ms) * 8;
            pkt_timeout |= arr[i].timer_ms;

            if (highest_timeout < pkt_timeout) {
                highest_timeout = pkt_timeout;
                entry = &arr[i];
            }
        }
    }

    return entry;
}

/**
 * If a packet is sent through us (mesh node), we add this packet as a pending
 * packet such that we can re-transmit the packet if an ACK has not been received.
 *
 * We also add a packet as pending if mesh_send() was called, such that
 * we can retransmit the packet if an ACK has not been received.
 *
 * This function assumes the packet is an ACK type packet to ensure delivery.
 * @param num_hops  The number of expected hops packet can take to set its retry time.
 */
static void mesh_pending_packets_add(const mesh_packet_t *pPkt, const uint8_t num_hops)
{
    mesh_pnd_pkt_t *entry = NULL;
    uint16_t timeout_ms = (1 + num_hops) * MESH_ACK_TIMEOUT_MS;

    /*
     * We have to update soft timers before we add a pending packet because if mesh_service()
     * is not called periodically, then the delta will be large, and the packet will be sent
     * again too soon.
     */
    mesh_update_soft_timers();

    /* We don't want mesh packets to take precedence over our own pending packets, so
     * we use different pending packets arrays for our own pending packets.
     */
    if (g_our_node_id == pPkt->nwk.src) {
        entry = mesh_get_pnd_pkt_slot(&g_our_pnd_pkts[0], g_our_pnd_pkts_size);
    }
    else {
        entry = mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size);
        /* Route discovery packet needs to use special timeout */
        if (MESH_ZERO_ADDR == pPkt->mac.dst) {
            entry->disc_pkt = true;
            timeout_ms = MESH_PKT_DISC_TIMEOUT_MS;
        }
    }

    /* Copy the packet and set the timeout value.
     * We should have route information, and hop_count already set.
     * TO DO: Should we reinitialize the retry count?
     * If we don't then we let the source dictate the retries throughout the mesh nodes.
     * If we do, then we increase the chances of getting the packet through.
     */
    entry->timer_ms    = 0;
    entry->timeout_ms  = timeout_ms;
    entry->pkt         = *pPkt;
    entry->pkt.info.retries_rem = g_retry_count; /* DO THIS AFTER COPYING THE PACKET!!! */

    MESH_DEBUG_PRINTF("ADD PND PKT NWK %i/%i NEXT %i TIMEOUT %ims",
                      pPkt->nwk.src, pPkt->nwk.dst, pPkt->mac.dst, entry->timeout_ms);
}

/**
 * Handles the timeout and retry logic for the pending packets array.
 */
static void mesh_handle_pnd_pkts_for_arr(const mesh_packet_t *pRxPkt, mesh_pnd_pkt_t *arr, const uint8_t size_of_array)
{
    uint8_t i = 0;
    bool clear = false;
    mesh_pnd_pkt_t *pnd = NULL;

    for (i = 0; i < size_of_array; i++) {
        pnd = &arr[i];

        /* Was this pending packet a route discovery packet? */
        if (pnd->disc_pkt) {
            /* If destined node responded, so no need to repeat this packet. */
            if (NULL != pRxPkt &&
                pRxPkt->nwk.src == pnd->pkt.nwk.dst && /* Destined node responded */
                pRxPkt->nwk.dst == pnd->pkt.nwk.src)
            {
                MESH_DEBUG_PRINTF("REMOVE RTE DISC PKT: DST %i RESPONDED TO %i",
                                  pnd->pkt.nwk.src, pnd->pkt.nwk.dst);
                memset(pnd, 0, sizeof(*pnd));
            }
            /*
             * Another case is when another node repeats the packet who knows the route, so
             * we don't need to repeat the discovery packet.
             */
            else if (NULL != pRxPkt &&
                MESH_ZERO_ADDR != pRxPkt->mac.dst &&
                mesh_is_same_packet(pRxPkt, &(pnd->pkt)))
            {
                MESH_DEBUG_PRINTF("REMOVE RTE DISC PKT: RTE %i RPT FOR NWK %i/%i",
                                  pnd->pkt.mac.src, pnd->pkt.nwk.src, pnd->pkt.nwk.dst);
                memset(pnd, 0, sizeof(*pnd));
            }
            /* Packet timeout occurred, and destined node did not respond
             * so it is time to repeat the packet.
             */
            else if (pnd->timer_ms >= pnd->timeout_ms) {
                MESH_DEBUG_PRINTF("TIMEOUT: SEND DISC PKT FOR NWK %i/%i",
                                  pnd->pkt.nwk.src, pnd->pkt.nwk.dst);
                mesh_send_packet(&(pnd->pkt));
                memset(pnd, 0, sizeof(*pnd));
            }
        }
        /* Is this a pending packet with a destination ? */
        else if (MESH_ZERO_ADDR != pnd->pkt.nwk.dst) {
            clear = false;

            /* If the response is received, clear the pending packet :
             * For packet N1 --> N2 --> N3, nwk.src = 1, and nwk.dst = 3
             * For resp : N3 --> N3 --> N1,  rx.src = 3, and  rx.dst = 1
             * The response should be a ACK_RSP packet to accommodate for the fact
             * that N1 and N3 may be sending ACK packets to each other.
             */
            if (NULL != pRxPkt &&
                    mesh_pkt_ack_rsp == pRxPkt->info.pkt_type &&
                    pnd->pkt.nwk.src == pRxPkt->nwk.dst &&
                    pnd->pkt.nwk.dst == pRxPkt->nwk.src)
            {
                MESH_DEBUG_PRINTF("CLR PND PKT: ACK_RSP OK WITH NWK %i/%i", pRxPkt->nwk.src, pRxPkt->nwk.dst);
                clear = true;
            }
            /* An intermediate node repeated ACK_RSP packet, meaning it got the packet */
            else if (NULL != pRxPkt &&
                    mesh_pkt_ack_rsp == pRxPkt->info.pkt_type &&
                    pnd->pkt.mac.dst == pRxPkt->mac.src &&         /* Next node sent this packet out */
                    mesh_is_same_packet(&(pnd->pkt), pRxPkt)       /* Network src/dst/id matches */
            ){
                MESH_DEBUG_PRINTF("CLR PND PKT: %i RPT ACK_RSP PKT FOR NWK %i/%i",
                                  pRxPkt->mac.src, pRxPkt->nwk.src, pRxPkt->nwk.dst);
                clear = true;
            }
            /* If packet goes from N1 --> N2 --> N3 with ACK, and we hear back
             * from N2 sending packet out, we do not need to re-send it to N2.
             */
            else if (NULL != pRxPkt &&
                    pRxPkt->info.pkt_type &&
                    pRxPkt->mac.src == pnd->pkt.mac.dst &&   /* Destined node repeated packet */
                    mesh_is_same_packet(&(pnd->pkt), pRxPkt) /* Network src/dst/id matches */
            ){
                pnd->timer_ms = 0;
                pnd->pkt.info.retries_rem = 0;
                MESH_DEBUG_PRINTF("%i RPT PKT, NO RPT WITH NWK %i/%i TO MAC %i",
                                  pRxPkt->mac.src, pRxPkt->nwk.src, pRxPkt->nwk.dst, pRxPkt->mac.dst);
                // We do not clear the packet here, since we want to clear the routing entry
                // and retry the packet to find a new route.
            }
            /* Is it time to retransmit? */
            else if(pnd->timer_ms >= pnd->timeout_ms)
            {
                pnd->timer_ms = 0;

                if (pnd->pkt.info.retries_rem > 0) {
                    MESH_DEBUG_PRINTF("RETRY PKT WITH NWK %i/%i", pnd->pkt.nwk.src, pnd->pkt.nwk.dst);
                    mesh_send_retry_packet(&(pnd->pkt));
                }
                else {
                    /* Were we the source and was it through an intermediate node?
                     * If so, then remove the routing node, and retry packet delivery
                     * with unknown node.
                     * We only want to do this if we are the source node because we
                     * don't want the intermediate node to repeat 2x the retry count.
                     */
                    if (mesh_pkt_ack_rsp != pnd->pkt.info.pkt_type &&
                            pnd->pkt.nwk.src == g_our_node_id &&    /* Source was us */
                            pnd->pkt.mac.dst != pnd->pkt.nwk.dst && /* Through intermediate node */
                            pnd->pkt.mac.dst != MESH_ZERO_ADDR
                    ) {
                        pnd->pkt.mac.dst = MESH_ZERO_ADDR;         /* Route is now unknown  */
                        pnd->pkt.info.hop_count_max = MESH_RTE_DISCOVERY_HOPS;
                        MESH_DEBUG_PRINTF("RETRY PKT AND DISC NEW RTE WITH NWK %i/%i",
                                          pnd->pkt.nwk.src, pnd->pkt.nwk.dst);
                        mesh_send_retry_packet(&(pnd->pkt));
                        pnd->pkt.info.retries_rem = g_retry_count; /* Reset retry count */
                    }
                    else {
                        /* Retries have reached zero */
                        MESH_DEBUG_PRINTF("CLR PND PKT: FAILED WITH NWK %i/%i", pnd->pkt.nwk.src, pnd->pkt.nwk.dst);
                        clear = true;
                    }

                    /* We no longer hear from nwk.dst, so remove the route */
                    MESH_DEBUG_PRINTF("RTE %i REMOVED", pnd->pkt.nwk.dst);
                    mesh_remove_rte_entry(pnd->pkt.nwk.dst);

                    /**
                     * TO DO : FUTURE FEATURE :
                     * We could also send a special packet back to origin node to remove its
                     * route of pkt.nwk.dst through us, since we no longer hear from that node.
                     * Currently, origin node will timeout, and remove the route by itself at
                     * the expense of time, and retries, however, it is less code this way.
                     */
                }
            }

            if (clear) {
                memset(pnd, 0, sizeof(*pnd));
            }
        } // end if
    } // end loop
}

/**
 * Retransmits pending packets whose response (ACK) has not been received.
 * @param pRxPkt  The pointer to the packet that just came in.  This can be
 *                NULL indicating that there is no incoming packet and the
 *                call to this function is just to retransmit pending packets.
 */
static inline void mesh_handle_pending_packets(const mesh_packet_t *pRxPkt)
{
    mesh_handle_pnd_pkts_for_arr(pRxPkt, &g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size);
    mesh_handle_pnd_pkts_for_arr(pRxPkt, &g_our_pnd_pkts[0],  g_our_pnd_pkts_size);
}

/**
 * Adds the packet to history if not added already and updates routing table
 * based on the packet we just got.
 * @param duplicate  returned value is true, if the packet is a duplicate.
 * @param is_retry_packet  return value is true, if the duplicate packet is a retry packet.
 */
static void mesh_update_history_and_routing(const mesh_packet_t *pPkt, bool *duplicate, bool *is_retry_packet)
{
    bool duplicate_packet = false;
    uint8_t i = 0;
    mesh_pkt_history_t new_pkt;
    mesh_rte_table_t *entry = NULL;

    new_pkt.src = pPkt->nwk.src;
    new_pkt.pkt_id = pPkt->info.pkt_seq_num;
    new_pkt.retries = pPkt->info.retries_rem;

    /* Check if we have the unique id in our history */
    for(i = 0; i < g_pkt_history_size; i++) {
        mesh_pkt_history_t *existing = &g_pkt_hist[i];
        if(existing->src == new_pkt.src && existing->pkt_id == new_pkt.pkt_id) {
            duplicate_packet = true;
            /* Packet is a duplicate, but does it differ only by retry count? */
            *is_retry_packet = (existing->retries != new_pkt.retries);
            existing->retries = new_pkt.retries;
            break;
        }
    }

    /* If not duplicate packet, add to our history of duplicate packets.
     * For this case, we only update routing table based on nwk.src because
     * multiple nodes might've sent us this packet.
     */
    if (!duplicate_packet)
    {
        *g_pkt_hist_wptr = new_pkt;
        /* if the pointer was at the last element, reset back to zero */
        if (g_pkt_hist_wptr == &(g_pkt_hist[g_pkt_history_size-1])) {
            g_pkt_hist_wptr = &(g_pkt_hist[0]);
        }
        else {
            g_pkt_hist_wptr++;
        }

        /* Update the routing table when a packet arrives to us through an
         * intermediate node, but we don't want to add our own route if our
         * own packet comes from an intermediate node.
         */
        if (pPkt->mac.src != pPkt->nwk.src && g_our_node_id != pPkt->nwk.src) {
            entry = mesh_get_rte_to_modify(pPkt->nwk.src);
            mesh_update_rte_scores(entry);

            /* We only want to add or modify this route if hops are less to nwk.src through this mac.src
             * If we get entry->dst being zero, that means, we got an entry that needs to be over-written.
             * If new hop count is less than or equal to, we need to update the route even if it is equal
             * such that we can keep a copy of the latest route.
             */
            if (MESH_ZERO_ADDR == entry->dst || pPkt->info.hop_count <= entry->num_hops) {
                entry->dst = pPkt->nwk.src;
                entry->next_hop = pPkt->mac.src;
                entry->num_hops = pPkt->info.hop_count;
            }
        }
    }

    /* Update the routing table :
     * Whoever sent us this packet is 0 hops away.
     * We still want to write or over-write this because it is possible that
     * we have got the packet through a repeater node even though it is a
     * duplicate packet from the source node.
     */
    entry = mesh_get_rte_to_modify(pPkt->mac.src);
    entry->dst = pPkt->mac.src;
    entry->next_hop = pPkt->mac.src;
    entry->num_hops = 0;
    mesh_update_rte_scores(entry);

    *duplicate = duplicate_packet;
}

/**
 * Handles the packet such that we can participate in the mesh network
 * and route it appropriately.
 */
static void mesh_handle_mesh_packet(mesh_packet_t *pPkt)
{
    bool ensure_delivery = false;
    uint8_t num_hops = 0;
    mesh_rte_table_t *entry = mesh_find_rte_tbl_entry(pPkt->nwk.dst);

    /* This function is only called when hop_count is less then hop_count_max
     * so we just increment the hop_count and send the packet regardless.
     */
    pPkt->info.hop_count++;

    /* Packet destined for us means we are intermediate node, so repeat the packet */
    if (g_our_node_id == pPkt->mac.dst) {
        MESH_DEBUG_PRINTF("RPT ROUTED PKT WITH NWK %i/%i", pPkt->nwk.src, pPkt->nwk.dst);
        const bool ack_pkt = (mesh_pkt_ack == pPkt->info.pkt_type) || (mesh_pkt_ack_app == pPkt->info.pkt_type);

        const uint8_t next_dst = entry ? entry->next_hop : MESH_ZERO_ADDR;
        const bool ack_rsp = (mesh_pkt_ack_rsp == pPkt->info.pkt_type) && /* ACK's response packet */
                              (pPkt->nwk.dst  != next_dst) &&  /* Through intermediate node */
                              (MESH_ZERO_ADDR != next_dst);    /* with known route */

        ensure_delivery = ack_pkt | ack_rsp;
    }
    /* Packet doesn't know its next route, so we repeat it hoping it will find its destination
     * But if we have its route already, then the case doesn't apply, so we should proceed
     * to repeat the packet.
     */
    else if (MESH_ZERO_ADDR == pPkt->mac.dst && NULL == entry) {
        MESH_DEBUG_PRINTF("ADD RTE DISC PKT WITH NWK %i/%i", pPkt->nwk.src, pPkt->nwk.dst);
        mesh_pending_packets_add(pPkt, pPkt->info.hop_count_max);
        return;
    }
    else if (MESH_ZERO_ADDR != pPkt->mac.dst){
        MESH_DEBUG_PRINTF("WE ARE NOT MAC TO RPT PKT WITH NWK %i/%i", pPkt->nwk.src, pPkt->nwk.dst);
        return;
    }

    /* Fill the next route information if next hop is known */
    if (entry) {
        pPkt->mac.dst = entry->next_hop;
        num_hops = entry->num_hops;
    }
    else {
        pPkt->mac.dst = MESH_ZERO_ADDR;
    }

    mesh_send_packet(pPkt);

    /* If packet type requires ACK, and we are the next node responsible to
     * deliver the packet, then we add this packet to our list of packets
     * waiting ACK and repeat it if we don't hear back from destined node.
     *
     * TO DO If mac.dst is not known, should we still ensure delivery ?
     * There are two possibilities why this can occur :
     *   - Our routing table was too small and the entry was removed.
     *   - The destined node is no longer reachable through us.
     *
     * We can ensure delivery still because if will fail eventually, then
     * the origin node will erase its route and find a new route later.
     */
    if(ensure_delivery) {
        if (!entry) {
            MESH_DEBUG_PRINTF("ERROR: PKT WITH NWK %i/%i HAS NO RTE THRU US", pPkt->nwk.src, pPkt->nwk.dst);
        }
        mesh_pending_packets_add(pPkt, num_hops);
    }
}

/**
 * Handles a packet addressed to us.
 * @param dup If true, then packet is not queued for application.
 * @param ack If true and packet type requires an ACK, then an ACK packet is sent.
 */
static void mesh_handle_our_packet(mesh_packet_t *pPkt, const bool dup, const bool ack)
{
    /* Only queue the data if not a duplicate, and if data length is > 0
     * because data length being zero is the PING packet
     */
    if (!dup && pPkt->info.data_len > 0) {
        MESH_DEBUG_PRINTF("Q PKT FROM %i THRU %i", pPkt->nwk.src, pPkt->mac.src);
        if (!g_driver.app_recv(pPkt, sizeof(*pPkt))) {
            g_error_mask |= mesh_err_app_recv;
        }
    }

    /* If ACK is required, then piggy-back our stats data if possible. */
    if (ack && mesh_pkt_ack == pPkt->info.pkt_type) {
        /* Zero byte data means it's a ping packet, send back our description */
        MESH_DEBUG_PRINTF("SEND ACK BACK TO %i", pPkt->nwk.src);
        if (0 == pPkt->info.data_len) {
            const uint8_t size = sizeof(g_our_name) <= sizeof(pPkt->data) ?
                                 sizeof(g_our_name) :  sizeof(pPkt->data);
            mesh_send_ack(g_our_name, size, pPkt);
        }
#if MESH_USE_STATISTICS
        else if (!mesh_send_ack((char*)&g_mesh_stats, sizeof(g_mesh_stats), pPkt))
#else
        else
#endif
        {
            /* If we use statistics and packet send fails, it is probably due to
             * pay-load size too small for g_stats structure size, so re-send it.
             */
            mesh_send_ack(NULL, 0, pPkt);
        }
    }
    /* We should also send an ACK if the sender doesn't know its route such
     * that we can be "discovered" even if it's not an ACK packet.
     * NULL Packet of ACK_RSP will not be queued for the source so this
     * packet would go un-noticed, which is what we want.
     * @note This function is not called for broadcast message.
     */
    else if (MESH_ZERO_ADDR == pPkt->mac.dst) {
        MESH_DEBUG_PRINTF("NWK %i CANNOT LOCATE US; SEND PING PKT WITH NACK", pPkt->nwk.src);
        mesh_send_ack(NULL, 0, pPkt);
    }
}












/**********************************************/
/****             Public functions         ****/
/**********************************************/
bool mesh_init(const uint8_t id,
               const bool is_rpt_node,
               const char *node_name,
               const mesh_driver_t d,
               const bool send_discovery_packet)
{
    bool status = false;
    const uint8_t discovery_hops = 1; // Some reasonable default value.

    /* Node ID cannot be broadcast address or zero */
    if (MESH_ZERO_ADDR == id || MESH_BROADCAST_ADDR == id) {
        return status;
    }

    /* NULL driver */
    if (NULL == d.app_recv || NULL == d.radio_init ||
        NULL == d.radio_recv || NULL == d.radio_send ||
        NULL == d.get_timer) {
        return status;
    }

    memset(&g_our_pnd_pkts[0], 0, sizeof(g_our_pnd_pkts));
    memset(&g_mesh_pnd_pkts[0], 0, sizeof(g_mesh_pnd_pkts));
    memset(&g_rte_table[0], 0, sizeof(g_rte_table));
    memset(&g_pkt_hist[0], 0, sizeof(g_pkt_hist));
    #if MESH_USE_STATISTICS
    memset(&g_mesh_stats, 0, sizeof(g_mesh_stats));
    #endif

    g_our_node_id = id;
    g_rpt_node = is_rpt_node;
    g_driver = d;
    memset(g_our_name, 0, sizeof(g_our_name));
    strncpy(g_our_name, node_name, sizeof(g_our_name)-1);

    /* If init works, then send discovery packet if asked */
    status = g_driver.radio_init(NULL, 0);
    if (status && send_discovery_packet) {
        // Send a small welcome packet to everyone.
        status = mesh_send(MESH_BROADCAST_ADDR, false, "HELLO\n", 6, discovery_hops);
    }

    /* Test to make sure our software timer callback returns good value */
    if (status) {
        status = mesh_update_soft_timers();
    }

    return status;
}

bool mesh_set_node_address(const uint8_t local_node_id)
{
    bool success = false;
    if (MESH_ZERO_ADDR != local_node_id && MESH_BROADCAST_ADDR != local_node_id) {
        g_our_node_id = local_node_id;
        success = true;
    }
    return success;
}

uint8_t mesh_get_node_address(void)
{
    return g_our_node_id;
}

void mesh_set_retry_count(const uint8_t count)
{
    if (count <= MESH_RETRY_COUNT_MAX) {
        g_retry_count = count;
    }
}

void mesh_service(void)
{
    mesh_packet_t packet = { {0},{0} };
    mesh_packet_t *pMeshPacket = NULL;

    /* Data structures are locked, so just return for now.
     * Better luck next time :)
     */
    if (g_locked) {
        return;
    }

    if (g_driver.radio_recv(&packet, sizeof(packet)))
    {
        //MESH_DEBUG_PRINTF("Rx PKT FROM %i THRU %i", packet.nwk.src, packet.mac.src);

        #if MESH_USE_STATISTICS
        g_mesh_stats.pkts_intercepted++;
        g_mesh_stats.rte_entries = mesh_get_num_routing_entries();
        #endif

        /* If there is version mismatch, we need to completely discard this packet */
        if (MESH_VERSION != packet.info.version) {
            MESH_DEBUG_PRINTF("ERROR: VERSION MISMATCH");
            g_error_mask |= mesh_err_ver_mismatch;
        }
        /* For single radio systems, we shouldn't ever get our own packet back */
        else if (packet.mac.src == g_our_node_id) {
            MESH_DEBUG_PRINTF("ERROR: DUPLICATE NODE WITH OUR ADDRESS");
            g_error_mask |= mesh_err_dup_node;
        }
        else {
            /* Update history and routing and get status if packet is a duplicate or retry packet */
            bool duplicate = false;
            bool is_retry_packet = false;
            mesh_update_history_and_routing(&packet, &duplicate, &is_retry_packet);

            /* If packet is complete duplicate, then we do not consider it unique */
            const bool unique_packet = !duplicate || is_retry_packet;

            /**
             * @note
             * When N1 sends a packet to 4 nodes it can transmit to, all four nodes will repeat the
             * packet if the route is unknown, so we need these nodes to discard the packets they
             * repeat to each other.
             * However, when the origin node doesn't get an ACK back, it will r-esend the packet
             * with the retries_rem decremented, such that all these nodes can repeat the packet
             * instead of throwing it away.
             * The destined node needs to only send data to application layer if packet is not
             * a duplicate, however, for every re-sent packet, it needs to ACK back the packet.
             */

            if (!unique_packet)
            {
                MESH_DEBUG_PRINTF("DISCARD DUP PKT FROM %i NWK %i/%i",
                                  packet.mac.src, packet.nwk.src, packet.nwk.dst);
            }
            /* Can't do something with our own packet which we may receive when someone
             * repeats our own packet back to us, but we do need to update our routing
             * table and we need to be smart about a fact that our repeater node may
             * have repeated a packet, and we don't need to re-send it to that node.
             */
            else if (g_our_node_id == packet.nwk.src) {
                pMeshPacket = &packet;
                MESH_DEBUG_PRINTF("GOT MY OWN PKT FROM %i GOING TO %i", packet.mac.src, packet.nwk.dst);
            }
            else if(MESH_BROADCAST_ADDR == packet.nwk.dst)
            {
                MESH_DEBUG_PRINTF("RX BROADCAST PKT FROM %i THRU %i", packet.nwk.src, packet.mac.src);
                if (!g_driver.app_recv(&packet, sizeof(packet))) {
                    g_error_mask |= mesh_err_app_recv;
                }

                /* Repeat broadcast packet blindly without any routing mess */
                if (packet.info.hop_count++ < packet.info.hop_count_max) {
                    MESH_DEBUG_PRINTF("TX BROADCAST PKT FROM %i THRU %i", packet.nwk.src, packet.mac.src);
                    mesh_send_packet(&packet);
                }
            }
            else if (g_our_node_id == packet.nwk.dst)
            {
                MESH_DEBUG_PRINTF("OUR PKT FROM %i THRU %i", packet.nwk.src, packet.mac.src);
                pMeshPacket = &packet;

                const bool pkt_should_be_acked = unique_packet;
                mesh_handle_our_packet(&packet, duplicate, pkt_should_be_acked);
            }
            else if (g_rpt_node && packet.info.hop_count < packet.info.hop_count_max) {
                pMeshPacket = &packet;
                mesh_handle_mesh_packet(pMeshPacket);
            }
            else {
                MESH_DEBUG_PRINTF("DISCARD PKT NWK %i/%i MAC %i/%i HOPS %i/%i SEQ:%i RT:%i",
                                   packet.nwk.src, packet.nwk.dst,
                                   packet.mac.src, packet.mac.dst,
                                   packet.info.hop_count, packet.info.hop_count_max,
                                   packet.info.pkt_seq_num, packet.info.retries_rem);
            }
        } /* else */
    }

    /* Service the timeouts for the pending packets */
    mesh_update_soft_timers();

    /* Check if this packet is response to a pending packet, and clear the
     * pending packet.  If timeout occurs within one of the pending packets,
     * then we will send out the packet again.
     */
    mesh_handle_pending_packets(pMeshPacket);
}

bool mesh_send(const uint8_t dst, const mesh_protocol_t type,
               const void* pData, const uint8_t len,
               const uint8_t hop_count_max)
{
    bool status = false;
    mesh_packet_t packet;

    /* Reject NULL data or size larger than payload's data
     * Data  Len  Accept ?
     * NULL  0      yes Ping packet, accept it.
     * NULL  #      no  Data is NULL when len defined, reject it.
     *  d    0      no  Data is not NULL but len is NULL, reject it.
     *  d    #      yes
     */
    if(len <= sizeof(packet.data) && !(!!pData ^ !!len)) {
        const uint8_t pair_cnt = 0 == len ? 0 : 1;
        if (mesh_form_pkt(&packet, dst, type, hop_count_max, pair_cnt, pData, len)) {
            status = mesh_send_formed_pkt(&packet);
        }
    }

    return status;
}

bool mesh_form_pkt(mesh_packet_t *pkt, const uint8_t dst,
                   const mesh_protocol_t type, const uint8_t hop_count_max,
                   uint8_t num_ptrs, ...)
{
    bool ok = false;

    if(MESH_ZERO_ADDR == dst || dst == g_our_node_id   ||   /* Invalid destination */
       hop_count_max > MESH_HOP_COUNT_MAX ||                /* Hop count overflow */
       NULL == pkt
    ) {
        ok = false;
        return ok;
    }

    /* All good so far, so zero out the structure */
    memset(pkt, 0, sizeof(*pkt));

    /* Force broadcast packet to be NACK type otherwise the type given by user */
    pkt->info.pkt_type = (MESH_BROADCAST_ADDR == dst) ? mesh_pkt_nack : type;

    /* Populate packet header */
    pkt->info.version = MESH_VERSION;
    // Redundant due to memset() :
    // pkt->info.hop_count = 0;
    pkt->info.retries_rem = g_retry_count;
    pkt->info.pkt_seq_num = mesh_get_next_seq_num();

    pkt->nwk.dst = dst;
    pkt->nwk.src = g_our_node_id;
    pkt->mac.src = g_our_node_id;

    /* Copy the data, and set the data_len */
    va_list vl;
    va_start(vl, num_ptrs);
    if (num_ptrs > 0) {
        ok = true; // We are okay unless we change it to false
        while(num_ptrs--) {
            const void *p = va_arg(vl, void*);
            const int   s = va_arg(vl, int);
            const uint8_t next_len = (s + pkt->info.data_len);

            if (next_len <= sizeof(pkt->data)) {
                memcpy( (&(pkt->data[0]) + pkt->info.data_len), p, s);
                pkt->info.data_len = next_len;
            }
            else {
                ok = false;
                break;
            }
        }
    }
    /* For the case of ping packet when data will be NULL */
    else {
        ok = true;
    }
    va_end(vl);

    /* Populate routing info last, but it could be NULL */
    g_locked = true;
    mesh_rte_table_t *entry = mesh_find_rte_tbl_entry(dst);
    mesh_update_rte_scores(entry);
    g_locked = false;

    if (NULL == entry) {
        pkt->info.hop_count_max = hop_count_max;
        pkt->mac.dst = MESH_ZERO_ADDR;
    }
    else {
        pkt->info.hop_count_max = entry->num_hops;
        pkt->mac.dst = entry->next_hop;
    }

    return ok;
}

bool mesh_deform_pkt(mesh_packet_t *pkt, uint8_t num_ptrs, ...)
{
    bool ok = true;
    va_list args;
    va_start(args, num_ptrs);

    int offset = 0;
    while(offset < sizeof(pkt->data) && num_ptrs--) {
        void *p  = va_arg(args, void*);
        int size = va_arg(args, int);

        if ((offset + size) > sizeof(pkt->data)) {
            ok = false;
            break;
        }
        /* User can specify NULL to skip the copy operation */
        else if (p) {
            memcpy(p, &pkt->data[0] + offset, size);
        }
        offset += size;
    }
    va_end(args);

    return (ok);
}

bool mesh_send_formed_pkt(mesh_packet_t *pkt)
{
    bool ok = false;

    /* We don't want a task to send a packet, while mesh_service() is simultaneously
     * trying to send a packet too.  We also want to add to pending packets and lock
     * out mesh_send() from accessing the structures.
     */
    g_locked = true;
    if (NULL != pkt && (ok = mesh_send_packet(pkt))) {
        /* Ensure delivery of ACK or APP_ACK packet */
        const bool ack_pkt = (mesh_pkt_ack == pkt->info.pkt_type || mesh_pkt_ack_app == pkt->info.pkt_type);

        /* Ensure delivery of ACK's response to the next node */
        const bool rsp_pkt = (mesh_pkt_ack_rsp == pkt->info.pkt_type &&
                              pkt->mac.dst != MESH_ZERO_ADDR &&
                              pkt->nwk.dst != pkt->mac.dst);

        if (ack_pkt || rsp_pkt) {
            mesh_pending_packets_add(pkt, pkt->info.hop_count_max);
        }
    }
    g_locked = false;

    return ok;
}

const mesh_rte_table_t* mesh_get_routing_entry(const uint8_t route_num)
{
    mesh_rte_table_t *entry = NULL;
    uint8_t idx = 0, found_entries = 0;

    // Routing table may have blank entries which do not account for route_num
    for (idx = 0; idx < g_rte_tbl_size; idx++) {
        if (MESH_ZERO_ADDR != g_rte_table[idx].dst) {
            if (route_num == found_entries) {
                entry = &g_rte_table[idx];
                break;
            }
            ++found_entries;
        }
    }

    return entry;
}

uint8_t mesh_get_num_routing_entries(void)
{
    uint8_t idx = 0, found_entries = 0;

    for (idx = 0; idx < g_rte_tbl_size; idx++) {
        if (MESH_ZERO_ADDR != g_rte_table[idx].dst) {
            ++found_entries;
        }
    }

    return found_entries;
}

bool mesh_is_route_known(const uint8_t addr)
{
    uint8_t idx = 0;

    for (idx = 0; idx < g_rte_tbl_size; idx++) {
        if (addr == g_rte_table[idx].dst) {
            return true;
        }
    }

    return false;
}

uint8_t mesh_get_pnd_pkt_count(void)
{
    uint8_t i = 0, count = 0;

    for (i = 0; i < g_mesh_pnd_pkts_size; i++) {
        if (MESH_ZERO_ADDR != g_mesh_pnd_pkts[i].pkt.nwk.dst) {
            ++count;
        }
    }

    for (i = 0; i < g_our_pnd_pkts_size; i++) {
        if (MESH_ZERO_ADDR != g_our_pnd_pkts[i].pkt.nwk.dst) {
            ++count;
        }
    }

    return count;
}

uint32_t mesh_get_expected_ack_time(uint8_t node_addr)
{
    mesh_rte_table_t *e =  mesh_find_rte_tbl_entry(node_addr);
    uint32_t timeout = e ? (1 + e->num_hops) * MESH_ACK_TIMEOUT_MS :
                           (MESH_ACK_TIMEOUT_MS * MESH_RTE_DISCOVERY_HOPS);
    return timeout;
}

uint32_t mesh_get_max_timeout_before_packet_fails(uint8_t node_addr)
{
    mesh_rte_table_t *e =  mesh_find_rte_tbl_entry(node_addr);
    uint32_t timeout = e ? (1 + e->num_hops) * g_retry_count * MESH_ACK_TIMEOUT_MS :
                           (g_retry_count * MESH_ACK_TIMEOUT_MS * MESH_RTE_DISCOVERY_HOPS);
    return timeout;
}

#if MESH_USE_STATISTICS
mesh_stats_t mesh_get_stats(void)
{
    return g_mesh_stats;
}
#endif

mesh_error_mask_t mesh_get_error_mask(void)
{
    return g_error_mask;
}

void mesh_reset_error_mask(void)
{
    g_error_mask = mesh_err_none;
}


#if (MESH_INCLUDE_TESTS)
#include "mesh_test.c.inc"
#endif

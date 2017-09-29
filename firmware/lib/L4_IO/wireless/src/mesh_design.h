/**
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
 * @brief    This is a documentation only file for the mesh algorithm.
 * @ingroup  WIRELESS
 *
 * @par Features
 * This mesh algorithm features :
 *  - Addressed nodes with auto-retries, and auto-acknowledge.
 *  - Minimal RAM footprint with no HEAP usage.
 *  - Each node contains routing table and can act like a repeater node.
 *  - Each packet contains its retry counts, and maximum hops it can take.
 *
 * Detailed features :
 *  - Each packet sent can use existing route, and if route has changed, a new
 *      route is automatically discovered using a special retry packet.
 *  - Each node's ACK contains some piggy-back data about the node itself.
 *  - Duplicate packets are absorbed but an ACK is still replied if its a
 *      duplicate, but a retry packet.
 *  - An ACK packet or a response to an ACK all use retries; even the repeating
 *      nodes participate to make sure the packet is delivered.
 *
 * @par Example use case
 * Suppose there are 4 nodes in your network, and the nodes only know about
 * their neighbors (ie: N1 only knows N2, but doesn't know where is N3 and N4):
 *
 *      N1      N2      N3      N4
 *
 * Each node maintains a routing table of 3 fields :
 *  - Destination Node
 *  - Next Destination
 *  - Hop count to destination node
 *
 * Here is how the mesh algorithm works step-by-step when N1 sends data to N4 :
 * D=Destination, S=Source, MD=Mesh Destination, MS=Mesh Source
 * Table: X|Y|Z where X=TO, Y=Next destination, Z=Hops
 *
 * ========================================================================
 * Here is how a packet is sent to unknown node N4
 * ========================================================================
 *              N1                  N2              N3              N4
 * Route Table : <Empty>        <Empty>         <Empty>         <Empty>
 * Packet      : D(4), S(1)
 *              MD(0),MS(1)
 *
 *              N1                  N2              N3              N4
 * Route Table : <Empty>        D1|D1|0         <Empty>         <Empty>
 * Packet      :                 D(4), S(1)
 *                              MD(0),MS(2)
 *
 *              N1                  N2              N3              N4
 * Route Table : <Empty>        D1|D1|0         D2|D2|0         <Empty>
 *                                              D1|D2|1
 * Packet      :                                 D(4), S(1)
 *                                              MD(0),MS(3)
 *
 *              N1                  N2              N3              N4
 * Route Table : <Empty>        D1|D1|0         D2|D2|0         D3|D3|0
 *                                              D1|D2|1         D1|D3|2
 * Packet      :                                                D(4), S(1)
 *                                                             MD(0),MS(3)
 *
 * ========================================================================
 * Now let's take a look at how the packet travels back :
 * ========================================================================
 * TODO
 *
 *
 * ========================================================================
 * Future work and improvement possibilities :
 *  - RSS (Received Signal Strength) should be factored into the routing
 *      table for those radios that support it.
 *  - If routing table is full, the route least used should be removed,
 *      therefore we need to add a "score" variable to the routing table
 *      structure.
 *  - Implement "packet_status" for each packet sent?
 *          ie: "BUSY", "RETRYING", "DELIVERED", "TIMEOUT", or "ERROR"
 *  - Implement special packet to query N-th routing entry of a node.
 *  - An ACK packet should contain what sequence number the ACK is for
 *    such that more than one packet with an ACK can be out at a time.
 * ========================================================================
 * How some scenarios are handled :
 *
 * Scenario 1 : N1 --> N2 --> N3
 *  N1 knows N3, but what if N3 disappears, and is now directly
 *  reachable without any intermediate node ?
 *  N1 will send packet with MD being N2, but instead, N3 will directly
 *  respond back, and now N1 will update its routing table.
 *
 * Scenario 2 : N1 --> N2 --> N3
 *  N1 knows N3, but N2 has disappeared and a new node N2b has replaced N2.
 *  N1 will retry its packets, and eventually realize after retry count-down
 *  that N3 no longer has a good route entry, so that route entry will be
 *  removed, and when N1 resends the packet again, new route discovery will
 *  be performed through N2b.
 *
 * Scenario 3 : N1 --> N2 --> N3
 *  N1 knows N3, but say that N2 can no longer reach N3.
 *  After some retries, N2 will give up on N3 destined packet because N3
 *  would not have sent an ACK back.
 *    Option 1 (not implemented) :  N2 can send a special packet to N1
 *          telling it to remove its route of N3 through N2.
 *    Option 2 (simpler) : N1 will send retries through N2, and after N3
 *          is not reachable, its route will be removed, and when a packet
 *          is sent again, new route discovery will be performed.
 *
 * ========================================================================
 */

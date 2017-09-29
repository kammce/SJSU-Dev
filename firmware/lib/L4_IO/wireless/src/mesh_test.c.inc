
#if (4 != MESH_MAX_NODES)
#error "Mest unit tests require MESH_MAX_NODES to be value of 4"
#endif

#include <assert.h>
#include <stdio.h>

static void mesh_test_routing_table(void);
static void mesh_test_rx(void);
static void mesh_test_tx(void);
static void mesh_test_methods(void);
static void mesh_test_rpt_node(void);

void mesh_test(void)
{
    printf("Sizes: %i %i %i\n", sizeof(mesh_pkt_info_t), sizeof(mesh_pkt_addr_t), sizeof(mesh_pkt_addr_t));
    printf("Payload header size is %i\n", MESH_PAYLOAD_HEADER_SIZE);

    // Size of payload should be 32 for the tests
    assert(32 == MESH_PAYLOAD);

    mesh_test_routing_table();
    mesh_test_rx();
    mesh_test_tx();
    mesh_test_methods();
    mesh_test_rpt_node();

    puts("Tests successful");
}



/** @{ Call counts and returns */
static int cc_init = 0;
static int cc_send = 0;
static int cc_receive = 0;
static int cc_app_receive = 0;
static int ret_receive = 0;
static mesh_packet_t ret_MeshPkt = { {0} };
static mesh_packet_t test_last_sent_pkt = { {0} };
/** @} */

static void mesh_test_reset(uint8_t our_node_id)
{
    g_our_node_id = our_node_id;
    memset(&g_mesh_pnd_pkts[0], 0, sizeof(g_mesh_pnd_pkts));
    memset(&g_rte_table[0], 0, sizeof(g_rte_table));
    memset(&g_pkt_hist[0], 0, sizeof(g_pkt_hist));
    memset(&g_our_pnd_pkts[0], 0, sizeof(g_our_pnd_pkts));
    cc_init = cc_send = cc_receive = cc_app_receive = ret_receive = 0;
}

static int mesh_stub_init(void* pData, int len)
{
    cc_init++;
    return 1;
}
static int mesh_stub_send(void* pData, int len)
{
    if (len <= sizeof(test_last_sent_pkt)) {
        memcpy(&test_last_sent_pkt, pData, len);
    }
    cc_send++;
    return 1;
}
static int mesh_stub_receive(void* pData, int len)
{
    cc_receive++;
    if (ret_receive) {
        memcpy(pData, &ret_MeshPkt, len);
    }
    return ret_receive;
}
static int mesh_stub_appReceive(void* pData, int len)
{
    cc_app_receive++;
    return 1;
}

static int mesh_stub_timer_get(void *data, int len)
{
    return 1;
}

#define test_counts(i, s, r, ar) \
            assert(i == cc_init); \
            assert(s == cc_send); \
            assert(r == cc_receive); \
            assert(ar == cc_app_receive); \
            cc_init = cc_send = cc_receive = cc_app_receive = 0
static void mesh_test_rx(void)
{
    int i = 0;

    mesh_driver_t driver;
    driver.radio_init = mesh_stub_init;
    driver.radio_recv = mesh_stub_receive;
    driver.radio_send = mesh_stub_send;
    driver.app_recv = mesh_stub_appReceive;
    driver.get_timer = mesh_stub_timer_get;

    /* Test init and init failures */
    puts("Test mesh init()");
    const uint8_t our_id = 2;
    mesh_test_reset(our_id);
    assert(false == mesh_init(0, 1, "name", driver, false));
    assert(false == mesh_init(MESH_BROADCAST_ADDR,  1, "name", driver, false));
    assert(true == mesh_init(our_id, 1, "name", driver, false));
    test_counts(1, 0, 0, 0);
    assert(true == mesh_init(our_id, 1, "name", driver, true));
    test_counts(1, 1, 0, 0);

    /* Nothing to receive, call to receive() should be made */
    mesh_service();
    test_counts(0, 0, 1, 0);

    puts("Test version mismatch, duplicate packet, and Rx our own packet");
    /* Version mismatch */
    ret_receive = 1;
    ret_MeshPkt.info.version = MESH_VERSION + 1;
    ret_MeshPkt.info.pkt_seq_num = 1;
    mesh_service();
    /* Correct mesh version */
    test_counts(0, 0, 1, 0);
    ret_MeshPkt.info.version = MESH_VERSION;
    mesh_service();
    test_counts(0, 0, 1, 0);
    /* Duplicate packet : */
    mesh_service();
    test_counts(0, 0, 1, 0);
    /* Rx our own packet */
    ret_MeshPkt.nwk.src = our_id;
    mesh_service();
    test_counts(0, 0, 1, 0);
    /* Duplicate node sending a packet */
    ret_MeshPkt.mac.src = our_id;
    mesh_service();
    test_counts(0, 0, 1, 0);

    puts("Rx broadcast and my packet");
    ret_receive = 1;
    ret_MeshPkt.nwk.src = our_id + 1; ret_MeshPkt.nwk.dst = our_id;
    ret_MeshPkt.mac.src = our_id + 1; ret_MeshPkt.mac.dst = our_id;
    ret_MeshPkt.info.data_len = 1;
    ret_MeshPkt.info.pkt_seq_num++;
    ret_MeshPkt.info.version = MESH_VERSION;
    ret_MeshPkt.info.pkt_type = mesh_pkt_nack;
    ret_MeshPkt.info.hop_count = 0; ret_MeshPkt.info.hop_count_max = 0;
    mesh_service();
    test_counts(0, 0, 1, 1);
    /* Rx broadcast packet */
    ret_MeshPkt.info.pkt_seq_num++;
    ret_MeshPkt.info.hop_count = 0;
    ret_MeshPkt.info.hop_count_max = 0;
    ret_MeshPkt.nwk.dst = MESH_BROADCAST_ADDR;
    mesh_service();
    test_counts(0, 0, 1, 1);
    /* Rx and repeat broadcast packet */
    ret_MeshPkt.info.pkt_seq_num++;
    ret_MeshPkt.info.hop_count = 1;
    ret_MeshPkt.info.hop_count_max = 2;
    ret_MeshPkt.nwk.dst = MESH_BROADCAST_ADDR;
    mesh_service();
    test_counts(0, 1, 1, 1);
    ret_receive = 0;

    /* dst_1 sending packet to dst_2 */
    puts("Test Mesh Repeat");
    const uint8_t dst_1 = our_id + 14;
    const uint8_t dst_2 = our_id + 15;
    memset(&g_rte_table[0], 0, sizeof(g_rte_table));
    ret_receive = 1;
    /* Repeat due to unknown mac destination */
    ret_MeshPkt.nwk.src = dst_1;
    ret_MeshPkt.nwk.dst = dst_2;
    ret_MeshPkt.mac.src = dst_1;
    ret_MeshPkt.mac.dst = MESH_ZERO_ADDR;
    ret_MeshPkt.info.pkt_seq_num++;
    ret_MeshPkt.info.hop_count = 0;
    ret_MeshPkt.info.hop_count_max = 1;
    mesh_service();
    test_counts(0, 0, 1, 0); /* No repeat yet */

    /* Make sure packet added to pending queue */
    assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == dst_2);
    mesh_service();
    test_counts(0, 0, 1, 0); /* No repeat yet */
    assert(MESH_PKT_DISC_TIMEOUT_MS == g_mesh_pnd_pkts[0].timeout_ms);

    /* Timeout occurred, should repeat now */
    g_mesh_pnd_pkts[0].timer_ms = MESH_PKT_DISC_TIMEOUT_MS;
    mesh_service();
    test_counts(0, 1, 1, 0);
    assert(0 == g_mesh_pnd_pkts[0].pkt.nwk.dst); /* Packet should be cleared */

    /* Send a new packet, test no repeat when destination responds */
    ret_MeshPkt.info.pkt_seq_num++;
    ret_MeshPkt.mac.dst = MESH_ZERO_ADDR; /* This is a route discovery packet */
    mesh_service();
    test_counts(0, 0, 1, 0);
    /* Packet should be added to pending packet */
    assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == dst_2);
    /* Suppose destination responds */
    ret_MeshPkt.nwk.src = dst_2;
    ret_MeshPkt.nwk.dst = dst_1;
    ret_MeshPkt.mac.src = dst_2;
    ret_MeshPkt.mac.dst = dst_1;
    ret_MeshPkt.info.pkt_seq_num++;
    mesh_service();
    /* Pending packet should be cleared, and we shouldn't repeat this packet */
    test_counts(0, 0, 1, 0);
    assert(0 == g_mesh_pnd_pkts[0].pkt.nwk.dst);

    /* No repeat due to max hop count */
    ret_MeshPkt.info.pkt_seq_num++;
    ret_MeshPkt.info.hop_count_max = 1;
    ret_MeshPkt.info.hop_count = 1;
    mesh_service();
    test_counts(0, 0, 1, 0);
    /* Don't repeat due to another MAC destination */
    ret_MeshPkt.info.hop_count_max = 10;
    ret_MeshPkt.info.hop_count = 1;
    ret_MeshPkt.info.pkt_seq_num++;
    ret_MeshPkt.mac.dst = our_id + 3;
    mesh_service();
    test_counts(0, 0, 1, 0);
    ret_receive = 0;

    puts("Test Mesh Repeat after timeout");
    /* Pretend we are N2 between N1 and N3, and N1 <--> N3 are too far away */
    mesh_test_reset(our_id);
    g_rte_table[0].dst = 1; g_rte_table[0].next_hop = 1; g_rte_table[0].num_hops = 0;
    g_rte_table[1].dst = 3; g_rte_table[1].next_hop = 3; g_rte_table[1].num_hops = 0;
    ret_receive = 1;
    {
        /* Suppose N1 is sending packet to N3 through us (N2) */
        ret_MeshPkt.nwk.src = 1; ret_MeshPkt.nwk.dst = 3;
        ret_MeshPkt.mac.src = 1; ret_MeshPkt.mac.dst = 2;
        ret_MeshPkt.info.hop_count = 0; ret_MeshPkt.info.hop_count_max = 1;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack;
        mesh_service();
        test_counts(0, 1, 1, 0);

        /* Test to make sure we've added this packet to our list of pending packets */
        assert(MESH_ACK_TIMEOUT_MS == g_mesh_pnd_pkts[0].timeout_ms);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.src == 1);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == 3);
        assert(g_mesh_pnd_pkts[0].pkt.mac.src == our_id);
        assert(g_mesh_pnd_pkts[0].pkt.mac.dst == 3);
        assert(g_mesh_pnd_pkts[0].pkt.info.hop_count == 1);
        assert(g_mesh_pnd_pkts[0].pkt.info.retries_rem == g_retry_count);

        /* Test to make sure our routing table stays the same */
        assert(g_rte_table[0].dst == 1);
        assert(g_rte_table[0].next_hop == 1);
        assert(g_rte_table[0].num_hops == 0);

        mesh_service();
        test_counts(0, 0, 1, 0);

        /* Now suppose timeout occurs on the packet, we should resend it */
        g_mesh_pnd_pkts[0].timer_ms = g_mesh_pnd_pkts[0].timeout_ms;
        ret_receive = 0;
        mesh_service();
        test_counts(0, 1, 1, 0);

        /* Now suppose N3 sends response back */
        ret_receive = 1;
        ret_MeshPkt.nwk.src = 3; ret_MeshPkt.nwk.dst = 1;
        ret_MeshPkt.mac.src = 3; ret_MeshPkt.mac.dst = our_id;
        ret_MeshPkt.info.hop_count = 0; ret_MeshPkt.info.hop_count_max = 1;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack_rsp;
        mesh_service();
        test_counts(0, 1, 1, 0);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.src == 0);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == 0);
        assert(g_mesh_pnd_pkts[0].pkt.mac.src == 0);
        assert(g_mesh_pnd_pkts[0].pkt.mac.dst == 0);
    }
    ret_receive = 0;

    puts("Test Mesh no repeat when destined node repeats");
    /* Pretend we are N2, sending packet to N4 through N3.
     * If we hear N3 sending the packet, we shouldn't repeat it to N3 again.
     */
    mesh_test_reset(our_id);
    g_rte_table[0].dst = 3; g_rte_table[0].next_hop = 3; g_rte_table[0].num_hops = 0;
    g_rte_table[1].dst = 4; g_rte_table[1].next_hop = 3; g_rte_table[1].num_hops = 1;
    ret_receive = 0;
    {
        assert(mesh_send(4, true, "hello", 5, 2));
        test_counts(0, 1, 0, 0);
        mesh_service();
        test_counts(0, 0, 1, 0);

        /* Test to make sure we've added this packet to our list of pending packets */
        uint8_t idx = 0;
        assert((MESH_ACK_TIMEOUT_MS*2) == g_our_pnd_pkts[idx].timeout_ms);
        assert(g_our_pnd_pkts[idx].pkt.nwk.src == our_id);
        assert(g_our_pnd_pkts[idx].pkt.nwk.dst == 4);
        assert(g_our_pnd_pkts[idx].pkt.mac.src == our_id);
        assert(g_our_pnd_pkts[idx].pkt.mac.dst == 3);
        assert(g_our_pnd_pkts[idx].pkt.info.hop_count_max == 1);

        /* Test to make sure our routing table stays the same */
        assert(g_rte_table[0].dst == 3);
        assert(g_rte_table[1].dst == 4);

        mesh_service();
        test_counts(0, 0, 1, 0);

        /* Now suppose N3 repeats packet for us, and we hear it */
        ret_receive = 1;
        ret_MeshPkt.nwk.src = 2; ret_MeshPkt.nwk.dst = 4;
        ret_MeshPkt.mac.src = 3; ret_MeshPkt.mac.dst = 4;
        ret_MeshPkt.info.hop_count = 1; ret_MeshPkt.info.hop_count_max = 1;
        ret_MeshPkt.info.pkt_seq_num = g_mesh_pnd_pkts[0].pkt.info.pkt_seq_num;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack;
        mesh_service();
        test_counts(0, 0, 1, 0);

        /* Packet should be resent to discover new route */
        puts("Packet should be resent to discover new route");
        g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
        mesh_service();
        test_counts(0, 1, 1, 0);

        /* Now suppose timeout occurs on the packet, we should resend it */
        ret_receive = 0;
        for ( i=0; i<g_retry_count; i++) {
            g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
            mesh_service();
            test_counts(0, 1, 1, 0);
        }

        /* Since this is our own packet, we will resend it again without route info */
        for ( i=0; i<g_retry_count; i++) {
            g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
            mesh_service();
            test_counts(0, 1, 1, 0);
        }

        /* After max retries, packet should be removed */
        g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
        mesh_service();
        test_counts(0, 0, 1, 0);
    }
    ret_receive = 0;

    puts("Test Mesh Repeat max retries and remove route");
    /* Pretend we are N2 between N1 and N3, and N1 <--> N3 are too far away */
    mesh_test_reset(our_id);
    g_rte_table[0].dst = 1; g_rte_table[0].next_hop = 1; g_rte_table[0].num_hops = 0;
    g_rte_table[1].dst = 3; g_rte_table[1].next_hop = 3; g_rte_table[1].num_hops = 0;
    ret_receive = 1;
    {
        /* Suppose N1 is sending packet to N3 through us (N2) */
        ret_MeshPkt.nwk.src = 1; ret_MeshPkt.nwk.dst = 3;
        ret_MeshPkt.mac.src = 1; ret_MeshPkt.mac.dst = our_id;
        ret_MeshPkt.info.hop_count = 0; ret_MeshPkt.info.hop_count_max = 1;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack;
        mesh_service();
        test_counts(0, 1, 1, 0);

        /* Test to make sure we've added this packet to our list of pending packets */
        assert(MESH_ACK_TIMEOUT_MS == g_mesh_pnd_pkts[0].timeout_ms);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.src == 1);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == 3);
        assert(g_mesh_pnd_pkts[0].pkt.mac.src == our_id);
        assert(g_mesh_pnd_pkts[0].pkt.mac.dst == 3);
        assert(g_mesh_pnd_pkts[0].pkt.info.hop_count == 1);

        /* Now suppose timeout occurs on the packet, we should resend it */
        for ( i=0; i<g_retry_count; i++) {
            g_mesh_pnd_pkts[0].timer_ms = g_mesh_pnd_pkts[0].timeout_ms;
            ret_receive = 0;
            mesh_service();
            test_counts(0, 1, 1, 0);
        }

        /* Now we should send delete route packet */
        g_mesh_pnd_pkts[0].timer_ms = g_mesh_pnd_pkts[0].timeout_ms;
        mesh_service();
        test_counts(0, 0, 1, 0);

        /* Pending packet should be deleted */
        assert(g_mesh_pnd_pkts[0].pkt.nwk.src == 0);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == 0);
        assert(g_mesh_pnd_pkts[0].pkt.mac.src == 0);
        assert(g_mesh_pnd_pkts[0].pkt.mac.dst == 0);

        /* Our route to N3 should be deleted */
        assert(g_rte_table[1].dst == 0);
        assert(g_rte_table[1].next_hop == 0);
        assert(g_rte_table[1].num_hops == 0);

        /* Suppose we get a packet to remove route for N1, make sure we handle it */
        /*
        ret_receive = 1;
        ret_MeshPkt.nwk.src = our_id; ret_MeshPkt.nwk.dst = 1;
        ret_MeshPkt.mac.src = our_id + 10; ret_MeshPkt.mac.dst = 0;
        ret_MeshPkt.info.hop_count = 0; ret_MeshPkt.info.hop_count_max = 1;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.pkt_type = mesh_pkt_del_rte;

        mesh_service();
        test_counts(0, 0, 1, 0);
        assert(g_rte_table[0].dst == 0);
        assert(g_rte_table[0].next_hop == 0);
        assert(g_rte_table[0].num_hops == 0);
        */
    }
    ret_receive = 0;

    puts("Test packet receive with an ack back");
    mesh_test_reset(our_id);
    ret_receive = 1;
    {
        /* Suppose N10 is sending packet to us */
        const uint8_t n10 = 10;
        ret_MeshPkt.nwk.src = n10; ret_MeshPkt.nwk.dst = our_id;
        ret_MeshPkt.mac.src = n10; ret_MeshPkt.mac.dst = our_id;
        ret_MeshPkt.info.hop_count = 0; ret_MeshPkt.info.hop_count_max = 1;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack;

        mesh_service();
        test_counts(0, 1, 1, 1);

        assert(test_last_sent_pkt.mac.src == our_id);
        assert(test_last_sent_pkt.mac.dst == n10);
        assert(test_last_sent_pkt.nwk.src == our_id);
        assert(test_last_sent_pkt.nwk.dst == n10);
        assert(mesh_pkt_ack_rsp == test_last_sent_pkt.info.pkt_type);
    }
    ret_receive = 0;

    puts("Test duplicate and retry packet handling");
    mesh_test_reset(our_id);
    ret_receive = 1;
    {
        /* Suppose N1 is sending packet to us */
        const uint8_t n1 = 10;
        ret_MeshPkt.nwk.src = n1; ret_MeshPkt.nwk.dst = our_id;
        ret_MeshPkt.mac.src = n1; ret_MeshPkt.mac.dst = our_id;
        ret_MeshPkt.info.hop_count = 0; ret_MeshPkt.info.hop_count_max = 1;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack;
        ret_MeshPkt.info.retries_rem = 0;

        /* Make sure we receive, send to application, and send ack back */
        mesh_service();
        test_counts(0, 1, 1, 1);

        /* Duplicate packet should not be received by application and ACK shouldn't be sent */
        mesh_service();
        test_counts(0, 0, 1, 0);

        /* Retry packed should be ACK'd back but not received by application */
        ret_MeshPkt.info.retries_rem++;
        mesh_service();
        test_counts(0, 1, 1, 0);
    }
    ret_receive = 0;
}

static void mesh_test_tx(void)
{
    mesh_packet_t packet;
    uint8_t i = 0;

    const uint8_t our_id = 2;
    mesh_test_reset(our_id);

    puts("Test mesh_send()");
    puts("  Test bad parameters to mesh_send()");
    assert(0 == mesh_send(0, true, (void*)"hello", 5, 1));
    assert(0 == mesh_send(1, true, NULL, 5, 1));
    assert(0 == mesh_send(1, true, (void*)"hello", sizeof(packet.data)+1, 1));
    assert(0 == mesh_send(1, true, (void*)"hello", sizeof(packet.data), MESH_HOP_COUNT_MAX + 1));
    assert(0 == mesh_send(our_id, true, (void*)"hello", sizeof(packet.data), MESH_HOP_COUNT_MAX));

    /* NACK packet should not be added to pending packets */
    puts("  Test NACK packet");
    assert(1 == mesh_send(our_id+1, false, (void*)"hello", sizeof(packet.data), MESH_HOP_COUNT_MAX));
    test_counts(0, 1, 0, 0);
    assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == 0);

    puts("  Test ACK packet");
    /* ACK packet should be added to pending packets */
    g_rte_table[0].dst = our_id + 1;
    g_rte_table[0].next_hop = our_id + 1;
    g_rte_table[0].num_hops = 0;
    uint8_t idx = 0;
    assert(1 == mesh_send(our_id+1, true, (void*)"hello", sizeof(packet.data), MESH_HOP_COUNT_MAX));
    test_counts(0, 1, 0, 0);
    assert(g_our_pnd_pkts[idx].pkt.nwk.dst == our_id+1);

    /* Make sure packet retransmits */
    for ( i=0; i<g_retry_count; i++) {
        g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
        mesh_service();
        test_counts(0, 1, 1, 0);
        assert(test_last_sent_pkt.nwk.src == our_id);
        assert(test_last_sent_pkt.nwk.dst == our_id + 1);
        assert(test_last_sent_pkt.mac.src == our_id);
    }

    /* Make sure packet retransmission stops */
    g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
    mesh_service();
    test_counts(0, 0, 1, 0);
    assert(g_our_pnd_pkts[idx].pkt.nwk.dst == 0);

    puts("  Test ACK packet with 2x the retries after first set of retry fails");
    mesh_test_reset(our_id);
    const uint8_t n3 = our_id + 1;
    const uint8_t n4 = n3 + 1;
    g_rte_table[0].dst = n3;
    g_rte_table[0].next_hop = n3;
    g_rte_table[0].num_hops = 0;
    g_rte_table[1].dst = n4;
    g_rte_table[1].next_hop = n3;
    g_rte_table[1].num_hops = 1;

    assert(1 == mesh_send(n4, true, (void*)"hello", sizeof(packet.data), MESH_HOP_COUNT_MAX));
    test_counts(0, 1, 0, 0);
    assert(g_our_pnd_pkts[idx].pkt.nwk.dst == n4);
    assert(g_our_pnd_pkts[idx].pkt.mac.dst == n3);

    /* Make sure packet retransmits to n3 for the retry count first */
    for (i=0; i<g_retry_count; i++) {
        g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
        mesh_service();
        test_counts(0, 1, 1, 0);
        assert(test_last_sent_pkt.nwk.src == our_id);
        assert(test_last_sent_pkt.nwk.dst == n4);
        assert(test_last_sent_pkt.mac.src == our_id);
        assert(test_last_sent_pkt.mac.dst == n3);
    }

    /* Make sure packet retransmits to n3 with new route discovery */
    /* Plus one for retry count because when we send data without the route,
     * that is considered original packet and then retries are performed.
     */
    for (i=0; i<g_retry_count+1; i++) {
        g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
        mesh_service();
        test_counts(0, 1, 1, 0);
    }

    /* Make sure no more retries occur, and pending packet is cleared */
    g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
    mesh_service();
    test_counts(0, 0, 1, 0);
    assert(g_our_pnd_pkts[0].pkt.nwk.dst == 0);

    puts("  Test pending packet of ACK_RSP");
    mesh_test_reset(our_id);
    /* Sending to n3 shouldn't add pending packet because N3 route is not known */
    mesh_send(3, mesh_pkt_ack_rsp, "hello", 5, 0);
    test_counts(0, 1, 0, 0);
    assert(g_our_pnd_pkts[idx].pkt.nwk.dst == 0);

    /* If n3 is our immediate neighbor, we still shouldn't add pending packet for ACK_RSP */
    mesh_rte_table_t *e = mesh_get_rte_to_modify(3);
    e->dst = 3;
    e->next_hop = 3;
    mesh_send(3, mesh_pkt_ack_rsp, "hello", 5, 0);
    test_counts(0, 1, 0, 0);
    assert(g_our_pnd_pkts[0].pkt.nwk.dst == 0);

    /* Suppose n4 is reachable through n3, sending packet to n4 should add to pending packets */
    e = mesh_get_rte_to_modify(4);
    e->dst = 4;
    e->next_hop = 3;
    e->num_hops = 1;
    mesh_send(4, mesh_pkt_ack_rsp, "hello", 5, 0);
    test_counts(0, 1, 0, 0);
    assert(g_our_pnd_pkts[idx].pkt.nwk.dst == 4);
    assert(g_our_pnd_pkts[idx].pkt.mac.dst == 3);

    ret_receive = 1;
    {
        /* Suppose N3 is sending random packet to n4, this shouldn't remove our pending packet */
        const uint8_t n3 = 3;
        const uint8_t n4 = 4;
        idx = 0;

        /* Make sure we receive, send to application, and send ack back */
        ret_MeshPkt = g_our_pnd_pkts[idx].pkt;
        ret_MeshPkt.nwk.src = n3;
        /* Have to clear history otherwise duplicate packet will be rejected */
        memset(&g_pkt_hist[0], 0, sizeof(g_pkt_hist));
        mesh_service();
        test_counts(0, 0, 1, 0);
        assert(g_our_pnd_pkts[idx].pkt.nwk.dst == 4);
        assert(g_our_pnd_pkts[idx].pkt.mac.dst == 3);

        /* Suppose another mac sends the packet, this shouldn't conflict with our ACK_RSP packet */
        ret_MeshPkt = g_our_pnd_pkts[0].pkt;
        ret_MeshPkt.mac.src = n4 + 1;
        ret_MeshPkt.mac.dst = n4;
        /* Have to clear history otherwise duplicate packet will be rejected */
        memset(&g_pkt_hist[0], 0, sizeof(g_pkt_hist));
        mesh_service();
        test_counts(0, 0, 1, 0);
        assert(g_our_pnd_pkts[idx].pkt.nwk.dst == 4);
        assert(g_our_pnd_pkts[idx].pkt.mac.dst == 3);

        /* Now suppose N3 repeats our ACK_RSP packet, we should clear our pending packet */
        ret_MeshPkt = g_our_pnd_pkts[0].pkt;
        ret_MeshPkt.mac.src = n3;
        ret_MeshPkt.mac.dst = n4;
        /* Have to clear history otherwise duplicate packet will be rejected */
        memset(&g_pkt_hist[0], 0, sizeof(g_pkt_hist));
        mesh_service();
        test_counts(0, 0, 1, 0);
        assert(g_our_pnd_pkts[0].pkt.nwk.dst == 0);
    }
    ret_receive = 0;

    /* Make sure the ACK packet is retried */
    mesh_send(4, mesh_pkt_ack_rsp, "hello", 5, 0);
    test_counts(0, 1, 0, 0);
    assert(g_our_pnd_pkts[idx].pkt.nwk.dst == 4);
    assert(g_our_pnd_pkts[idx].pkt.mac.dst == 3);

    for (i=0; i<g_retry_count ; i++) {
        g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
        mesh_service();
        test_counts(0, 1, 1, 0);
    }

    g_our_pnd_pkts[idx].timer_ms = g_our_pnd_pkts[idx].timeout_ms;
    mesh_service();
    test_counts(0, 0, 1, 0);

    puts("  Test ACK_RSP packet's delivery to next node");
    mesh_test_reset(our_id);
    ret_receive = 1;
    {
        const uint8_t n1 = 1;
        const uint8_t n2 = 2;
        const uint8_t n3 = 3;
        const uint8_t n4 = 4;

        /* Suppose an ACK_RSP packet goes from N1 -> N2 -> N3 -> N4
         * N1 should repeat packet if N2 is not heard sending the ACK_RSP to N3
         * N2 should repeat packet if N3 is not heard sending the ACK_RSP to N4
         * We have already tested the case when N1 doesn't hear N2 repeating it.
         * Now we test the case when an arbitary intermediate node (N2) can
         * become responsible to repeat the ACK_RSP packet if its part of the route.
         */
        e = mesh_get_rte_to_modify(1); e->dst = 1; e->next_hop = 1; e->num_hops = 0;
        e = mesh_get_rte_to_modify(3); e->dst = 3; e->next_hop = 3; e->num_hops = 0;
        e = mesh_get_rte_to_modify(4); e->dst = 4; e->next_hop = 3; e->num_hops = 1;

        /* N1 sending packet to us shouldn't do anything */
        ret_MeshPkt.nwk.src = ret_MeshPkt.mac.src = n1;
        ret_MeshPkt.nwk.dst = our_id; ret_MeshPkt.mac.dst = our_id;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack_rsp;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        mesh_service();
        test_counts(0, 0, 1, 1);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == 0);
        assert(g_our_pnd_pkts[0].pkt.nwk.dst == 0);

        /* N1 sending packet to N3, also shouldn't do anything.
         * But we will send n3 it's packet that is routed through us.
         */
        ret_MeshPkt.nwk.src = ret_MeshPkt.mac.src = n1;
        ret_MeshPkt.nwk.dst = n3;
        ret_MeshPkt.mac.dst = n2;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack_rsp;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.hop_count_max = 1;
        mesh_service();
        test_counts(0, 1, 1, 0);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == 0);
        assert(g_our_pnd_pkts[0].pkt.nwk.dst == 0);

        /* If N1 is sending to N4, now we can ensure that N3 sends our ACK_RSP packet.
         * If we do not hear back from N3 sending the packet, it probably lost it, so
         * we should repeat it for N1.
         */
        ret_MeshPkt.nwk.src = ret_MeshPkt.mac.src = n1;
        ret_MeshPkt.nwk.dst = n4;
        ret_MeshPkt.mac.dst = n2;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack_rsp;
        ret_MeshPkt.info.pkt_seq_num++;
        mesh_service();
        test_counts(0, 1, 1, 0);
        assert(g_mesh_pnd_pkts[0].pkt.nwk.dst == n4);
        assert(g_our_pnd_pkts[0].pkt.nwk.dst == 0);
    }
    ret_receive = 0;
}

static void mesh_test_routing_table(void)
{
    puts("Test Routing Table logic");
    static mesh_packet_t mReceivedPkt = { {0},{0} };
    bool is_retry_packet = false;
    bool duplicate = false;
    g_our_node_id = 5;

    /* Test when a n1 --> n4 through unknown path and we are n2 */
    memset(&g_rte_table[0], 0, sizeof(g_rte_table));
    mReceivedPkt.nwk.src = 1;
    mReceivedPkt.nwk.dst = 4;
    mReceivedPkt.mac.src = 1;
    mReceivedPkt.mac.dst = 0;
    mReceivedPkt.info.hop_count = 0;
    mReceivedPkt.info.pkt_seq_num++;
    mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
    assert(g_rte_table[0].dst == 1);
    assert(g_rte_table[0].next_hop == 1);
    assert(g_rte_table[0].num_hops == 0);

    /* Test when n2 sends us packet which originated from n1 and is destined for n4 */
    memset(&g_rte_table[0], 0, sizeof(g_rte_table));
    mReceivedPkt.nwk.src = 1;
    mReceivedPkt.nwk.dst = 4;
    mReceivedPkt.mac.src = 2;
    mReceivedPkt.mac.dst = 0;
    mReceivedPkt.info.hop_count = 1;
    mReceivedPkt.info.pkt_seq_num++;
    mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
    assert(g_rte_table[0].dst == 1);
    assert(g_rte_table[0].next_hop == 2);
    assert(g_rte_table[0].num_hops == 1);
    assert(g_rte_table[1].dst == 2);
    assert(g_rte_table[1].next_hop == 2);
    assert(g_rte_table[1].num_hops == 0);

    /* Test when we get a packet destined for us from intermediate node */
    g_our_node_id = 4;
    memset(&g_rte_table[0], 0, sizeof(g_rte_table));
    mReceivedPkt.nwk.src = 1;
    mReceivedPkt.nwk.dst = 4;
    mReceivedPkt.mac.src = 3;
    mReceivedPkt.mac.dst = 0;
    mReceivedPkt.info.hop_count = 2;
    mReceivedPkt.info.pkt_seq_num++;
    mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
    assert(g_rte_table[0].dst == 1);
    assert(g_rte_table[0].next_hop == 3);
    assert(g_rte_table[0].num_hops == 2);
    assert(g_rte_table[1].dst == 3);
    assert(g_rte_table[1].next_hop == 3);
    assert(g_rte_table[1].num_hops == 0);

    /* Test mesh_iterate_routing_table() */
    (g_rte_table[3].dst = 3);
    (g_rte_table[3].next_hop = 3);
    (g_rte_table[3].num_hops = 0);
    assert(NULL != mesh_get_routing_entry(0));
    assert(NULL == mesh_get_routing_entry(3));
    assert(NULL != mesh_get_routing_entry(1));
    assert(NULL != mesh_get_routing_entry(2));
    assert(1 == mesh_get_routing_entry(0)->dst);
    assert(3 == mesh_get_routing_entry(2)->dst);

    /* Test duplicate packet detection */
    g_our_node_id = 2;
    memset(&g_rte_table[0], 0, sizeof(g_rte_table));
    mReceivedPkt.nwk.src = 1;
    mReceivedPkt.nwk.dst = 2;
    mReceivedPkt.mac.src = 1;
    mReceivedPkt.mac.dst = 0;
    mReceivedPkt.info.retries_rem = 0;
    mReceivedPkt.info.pkt_seq_num++;
    /* Test original packet */
    mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
    assert(!duplicate);
    assert(!is_retry_packet);
    /* Test duplicate */
    mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
    assert(duplicate);
    assert(!is_retry_packet);
    /* Test retry packet */
    mReceivedPkt.info.retries_rem++;
    mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
    assert(duplicate);
    assert(is_retry_packet);

    /* Test routes being added and updated properly */
    do {
        const uint8_t n0 = 1;
        const uint8_t n1 = 2;
        const uint8_t n2 = 3;
        const uint8_t n3 = 4;
        const uint8_t n4 = 5;
        mesh_test_reset(n3);

        /* N2 sends a packet */
        mReceivedPkt.nwk.src = n2;
        mReceivedPkt.nwk.dst = n3;
        mReceivedPkt.mac.src = n2;
        mReceivedPkt.mac.dst = n3;
        mReceivedPkt.info.pkt_seq_num++;
        mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);

        const mesh_rte_table_t *e = NULL;
        e = mesh_find_rte_tbl_entry(n2);
        assert(NULL != e); assert(e->num_hops == 0); assert(e->next_hop == n2);

        /* N1 sends a packet through N2 */
        mReceivedPkt.nwk.src = n1;
        mReceivedPkt.nwk.dst = n3;
        mReceivedPkt.mac.src = n2;
        mReceivedPkt.mac.dst = n3;
        mReceivedPkt.info.hop_count = 1;
        mReceivedPkt.info.pkt_seq_num++;
        mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
        e = mesh_find_rte_tbl_entry(n2);
        assert(NULL != e); assert(e->num_hops == 0); assert(e->next_hop == n2);
        e = mesh_find_rte_tbl_entry(n1);
        assert(NULL != e); assert(e->num_hops == 1); assert(e->next_hop == n2);

        /* N0 sends a packet through N1 and N2 with two hops */
        mReceivedPkt.nwk.src = n0;
        mReceivedPkt.nwk.dst = n3;
        mReceivedPkt.mac.src = n2;
        mReceivedPkt.mac.dst = n3;
        mReceivedPkt.info.hop_count = 2;
        mReceivedPkt.info.pkt_seq_num++;
        mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
        e = mesh_find_rte_tbl_entry(n0);
        assert(NULL != e); assert(e->num_hops == 2); assert(e->next_hop == n2);
        e = mesh_find_rte_tbl_entry(n2);
        assert(NULL != e); assert(e->num_hops == 0); assert(e->next_hop == n2);
        e = mesh_find_rte_tbl_entry(n1);
        assert(NULL != e); assert(e->num_hops == 1); assert(e->next_hop == n2);

        /* Assume N0 is reacheable through N4 with less hops */
        mReceivedPkt.nwk.src = n0;
        mReceivedPkt.nwk.dst = n3;
        mReceivedPkt.mac.src = n4;
        mReceivedPkt.mac.dst = n3;
        mReceivedPkt.info.hop_count = 1;
        mReceivedPkt.info.pkt_seq_num++;
        mesh_update_history_and_routing(&mReceivedPkt, &duplicate, &is_retry_packet);
        e = mesh_find_rte_tbl_entry(n0);
        assert(NULL != e); assert(e->num_hops == 1); assert(e->next_hop == n4);
        e = mesh_find_rte_tbl_entry(n1);
        assert(NULL != e); assert(e->num_hops == 1); assert(e->next_hop == n2);
        e = mesh_find_rte_tbl_entry(n2);
        assert(NULL != e); assert(e->num_hops == 0); assert(e->next_hop == n2);
        e = mesh_find_rte_tbl_entry(n4);
        assert(NULL != e); assert(e->num_hops == 0); assert(e->next_hop == n4);
    } while (0);
}

static void mesh_test_methods(void)
{
    puts("Test methods");

    do {
        mesh_packet_t p1, p2;
        memset(&p1, 0, sizeof(p1));
        memset(&p2, 0, sizeof(p2));
        assert(mesh_is_same_packet(&p1, &p2));

        p1.nwk.dst = p2.nwk.dst = 1;
        assert(mesh_is_same_packet(&p1, &p2));

        p1.nwk.src = p2.nwk.src = 1;
        assert(mesh_is_same_packet(&p1, &p2));

        p1.info.pkt_seq_num = p2.info.pkt_seq_num = 1;
        assert(mesh_is_same_packet(&p1, &p2));

        p1.nwk.dst++; assert(!mesh_is_same_packet(&p1, &p2));
        p1.nwk.dst--; assert(mesh_is_same_packet(&p1, &p2));

        p1.nwk.src++; assert(!mesh_is_same_packet(&p1, &p2));
        p1.nwk.src--; assert(mesh_is_same_packet(&p1, &p2));

        p1.info.pkt_seq_num++; assert(!mesh_is_same_packet(&p1, &p2));
        p1.info.pkt_seq_num--; assert(mesh_is_same_packet(&p1, &p2));
    } while(0);

    do {
        mesh_packet_t p;
        p.nwk.src = g_our_node_id;
        g_mesh_stats.pkts_sent = g_mesh_stats.pkts_repeated = 0;
        mesh_send_packet(&p);
        assert(1 == g_mesh_stats.pkts_sent);
        assert(0 == g_mesh_stats.pkts_repeated);

        p.nwk.src++;
        mesh_send_packet(&p);
        assert(1 == g_mesh_stats.pkts_sent);
        assert(1 == g_mesh_stats.pkts_repeated);
    } while(0);

    do {
        mesh_packet_t p;
        p.info.retries_rem = 2;
        mesh_send_retry_packet(&p);
        assert(1 == p.info.retries_rem);
    } while(0);

    puts("    Test mesh_send_packet()");
    do {
#if MESH_USE_STATISTICS
        mesh_packet_t p;
        p.nwk.src = g_our_node_id;
        p.nwk.dst = g_our_node_id + 1;
        g_mesh_stats.pkts_sent = 0;
        g_mesh_stats.pkts_repeated = 0;
        mesh_send_packet(&p);
        assert(g_mesh_stats.pkts_sent == 1);
        assert(g_mesh_stats.pkts_repeated == 0);
        p.nwk.src++;
        mesh_send_packet(&p);
        assert(g_mesh_stats.pkts_sent == 1);
        assert(g_mesh_stats.pkts_repeated == 1);
#endif
    } while(0);

    puts("    Test mesh_find_rte_tbl_entry() and mesh_get_rte_to_modify()");
    do {
        memset(g_rte_table, 0, sizeof(g_rte_table));
        assert(NULL == mesh_find_rte_tbl_entry(1));

        g_rte_table[3].dst = 4;
        assert(&g_rte_table[3] == mesh_find_rte_tbl_entry(4));

        g_rte_table[0].dst = 1;
        assert(&g_rte_table[0] == mesh_find_rte_tbl_entry(1));
        assert(&g_rte_table[0] == mesh_get_rte_to_modify(1));
        assert(&g_rte_table[1] == mesh_get_rte_to_modify(2));

        g_rte_table[1].dst = 2;
        assert(&g_rte_table[2] == mesh_get_rte_to_modify(3));

        g_rte_table[2].dst = 3;
        assert(&g_rte_table[2] == mesh_get_rte_to_modify(3));

        g_mesh_stats.rte_overwritten = 0;
        assert(&g_rte_table[0] == mesh_get_rte_to_modify(5));
        g_rte_table[0].dst = 5;
        assert(1 == g_mesh_stats.rte_overwritten);

        /* Route with lowest score should be returned */
        g_rte_table[0].score = g_rte_table[1].score = g_rte_table[2].score = 1;
        assert(&g_rte_table[3] == mesh_get_rte_to_modify(6));
        g_rte_table[3].dst = 6;

        g_rte_table[1].score = g_rte_table[2].score = g_rte_table[3].score = 2;
        assert(&g_rte_table[0] == mesh_get_rte_to_modify(1));
        g_rte_table[0].dst = 1;

        assert(&g_rte_table[0] == mesh_get_rte_to_modify(1));
        assert(&g_rte_table[1] == mesh_get_rte_to_modify(2));
        assert(&g_rte_table[2] == mesh_get_rte_to_modify(3));
        assert(&g_rte_table[3] == mesh_get_rte_to_modify(6));

        mesh_remove_rte_entry(3);
        assert(NULL ==  mesh_find_rte_tbl_entry(9));

    } while(0);

    puts("    Test mesh_pending_packets_add()");
    do {
        memset(g_mesh_pnd_pkts, 0, sizeof(g_mesh_pnd_pkts));
        mesh_packet_t pkt;
        memset(&pkt, 0, sizeof(pkt));

        pkt.nwk.dst = 4; mesh_pending_packets_add(&pkt, 1);
        pkt.nwk.dst = 2; mesh_pending_packets_add(&pkt, 1);
        pkt.nwk.dst = 1; mesh_pending_packets_add(&pkt, 1);
        assert(4 == g_mesh_pnd_pkts[0].pkt.nwk.dst);
        assert(2 == g_mesh_pnd_pkts[1].pkt.nwk.dst);
        assert(1 == g_mesh_pnd_pkts[2].pkt.nwk.dst);

        pkt.nwk.dst = 3; mesh_pending_packets_add(&pkt, 1);
        assert(3 == g_mesh_pnd_pkts[3].pkt.nwk.dst);
        assert(0 == g_our_pnd_pkts[0].pkt.nwk.dst);
    } while(0);

    do {
        mesh_test_reset(1);
        assert(&g_mesh_pnd_pkts[0] == mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size));
        g_mesh_pnd_pkts[0].pkt.nwk.dst = 2;
        assert(&g_mesh_pnd_pkts[1] == mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size));
        g_mesh_pnd_pkts[1].pkt.nwk.dst = 3;
        assert(&g_mesh_pnd_pkts[2] == mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size));
        g_mesh_pnd_pkts[2].pkt.nwk.dst = 4;
        assert(&g_mesh_pnd_pkts[3] == mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size));
        g_mesh_pnd_pkts[3].pkt.nwk.dst = 5;

        assert(&g_mesh_pnd_pkts[0] == mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size));

        g_mesh_pnd_pkts[0].pkt.info.retries_rem = 4;
        g_mesh_pnd_pkts[1].pkt.info.retries_rem = 3;
        g_mesh_pnd_pkts[2].pkt.info.retries_rem = 6;
        g_mesh_pnd_pkts[3].pkt.info.retries_rem = 5;
        /* The packet with highest retries remaining should be returned */
        assert(&g_mesh_pnd_pkts[2] == mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size));

        /* Packet with higher timeout should be returned */
        g_mesh_pnd_pkts[2].pkt.info.retries_rem = 6;
        g_mesh_pnd_pkts[3].pkt.info.retries_rem = 6;
        g_mesh_pnd_pkts[2].timer_ms = 100;
        g_mesh_pnd_pkts[3].timer_ms = 200;
        assert(&g_mesh_pnd_pkts[3] == mesh_get_pnd_pkt_slot(&g_mesh_pnd_pkts[0], g_mesh_pnd_pkts_size));
    } while(0);

    puts("    Test mesh_form() and mesh_deform()");
    do {
        int i = 0x12345678;
        float f = 1.23;
        mesh_packet_t pkt;
        mesh_form_pkt(&pkt, 12, mesh_pkt_ack_app, 2, 3,
                        "hello", 5,
                        &i, sizeof(i),
                        &f, sizeof(f));
        assert(12 == pkt.nwk.dst);
        assert(mesh_pkt_ack_app == pkt.info.pkt_type);
        assert(2 == pkt.info.hop_count_max);

        char s[6] = { 0 };
        int ii;
        float ff;
        assert(mesh_deform_pkt(&pkt, 3,
                        s, 5,
                        &ii, sizeof(ii),
                        &ff, sizeof(ff)));
        assert(i == ii);
        assert(f == ff);
        assert(0 == strcmp(s, "hello"));

        ff = 0;
        assert(mesh_deform_pkt(&pkt, 3,
                        s, 5,
                        NULL, sizeof(ii),
                        &ff, sizeof(ff)));
        assert(f == ff);

        // Assuming payload size is 32
        assert(true == mesh_form_pkt(&pkt, 12, mesh_pkt_ack_app, 2, 1, "123456789012345678901234", 24));
        assert(false == mesh_form_pkt(&pkt, 12, mesh_pkt_ack_app, 2, 1, "1234567890123456789012345", 25));
    }while(0);
}

static void mesh_test_rpt_node(void)
{
    const uint8_t n1 = 1;
    const uint8_t n2 = 2;
    const uint8_t n3 = 3;

    puts("    Test ACK back through repeater node");
    mesh_test_reset(n3);
    ret_receive = 1;
    {
        /* N1 sending packet to N3 through N2 */
        ret_MeshPkt.nwk.src = n1;
        ret_MeshPkt.nwk.dst = n3;
        ret_MeshPkt.mac.src = n2;
        ret_MeshPkt.mac.dst = MESH_ZERO_ADDR;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.hop_count = 1;
        ret_MeshPkt.info.data_len = 1;
        mesh_service();
        test_counts(0, 1, 1, 1);

        /* Check N3's reply */
        assert(n3 == test_last_sent_pkt.nwk.src);
        assert(n1 == test_last_sent_pkt.nwk.dst);
        assert(n3 == test_last_sent_pkt.mac.src);
        assert(n2 == test_last_sent_pkt.mac.dst);
        assert(0 == test_last_sent_pkt.info.hop_count);
        assert(1 == test_last_sent_pkt.info.hop_count_max);
    }

    puts("    Test route discovery packet repeat");
    mesh_test_reset(n2);
    ret_receive = 1;
    {
        /* N1 sending packet to N3 with route discovery */
        ret_MeshPkt.nwk.src = n1;
        ret_MeshPkt.nwk.dst = n3;
        ret_MeshPkt.mac.src = n1;
        ret_MeshPkt.mac.dst = MESH_ZERO_ADDR;
        ret_MeshPkt.info.pkt_type = mesh_pkt_ack;
        ret_MeshPkt.info.pkt_seq_num++;
        ret_MeshPkt.info.version = MESH_VERSION;
        ret_MeshPkt.info.hop_count = 0;
        ret_MeshPkt.info.hop_count_max = 1;
        mesh_service();
        test_counts(0, 0, 1, 0);

        /* Route discovery packet timeout triggers this packet to be repeated */
        g_mesh_pnd_pkts[0].timer_ms = 9999;
        mesh_service();
        test_counts(0, 1, 1, 0);

        /* Check N2's repeat packet */
        assert(n1 == test_last_sent_pkt.nwk.src);
        assert(n3 == test_last_sent_pkt.nwk.dst);
        assert(n2 == test_last_sent_pkt.mac.src);
        assert(MESH_ZERO_ADDR == test_last_sent_pkt.mac.dst);
        assert(1 == test_last_sent_pkt.info.hop_count);
    }
}

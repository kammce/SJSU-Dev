#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "command_handler.hpp"
#include "wireless.h"
#include "nrf_stream.hpp"
#include "ff.h"



#if MESH_USE_STATISTICS
static void wirelessHandlerPrintStats(CharDev& output, mesh_stats_t *s, uint8_t node)
{
    output.printf("N%u: Rx/Tx, Rte/Ovt, Retried/Mesh Retried/Repeated: \n", node);
    output.printf("    %3u/%-3u %u/%u, %u/%u/%u\n",
                    s->pkts_intercepted, s->pkts_sent,
                    s->rte_entries, s->rte_overwritten,
                    s->pkts_retried, s->pkts_retried_others, s->pkts_repeated);
}
#endif

static CMD_HANDLER_FUNC(wsStreamHandler)
{
    const int outBlockTime = 1;
    const int timeout_ms = 1000;
    int addr = 0;
    cmdParams.scanf("%i", &addr);
    cmdParams.eraseFirstWords(1);

    if (0 == addr || 0 == cmdParams.getLen())
    {
        output.putline("Parse error: try: 'stream <addr> <command>'");
    }
    else
    {
        // Send the command to another nordic
        NordicStream &n = NordicStream::getInstance();

        // Flush any stale data:
        char c = 0;
        while (n.getChar(&c, 5)) {
            ;
        }
        n.setDestAddr(addr);
        n.putline(cmdParams());
        n.flush();

        // Terminal sends unique last four chars to indicate end of output
        const char endOfTx[] = TERMINAL_END_CHARS;
        char lastChars[sizeof(endOfTx)] = { 0 };
        int count = 0;
        int dropped = 0;

        /**
         * Output the response
         * @warning We collect data from nordic and output to possibly UART0 @ 38400bps
         * If nordic floods us with data, our UART output may block because it is slow,
         * therefore you may want to look at the possible suggestions :
         *   - Use faster UART (115200bps, but you still won't match Nordic's 2000Kbps)
         *   - Use higher UART Tx queue size (but you may still overflow eventually)
         *     See SYS_CFG_UART0_TXQ_SIZE at sys_config.h
         *   - Increase the WIRELESS_RX_QUEUE_SIZE at sys_config.h and increase outBlockTime
         *     This will guarantee you can output 24 * N bytes before we overflow data.
         *   - At nrf_stream.cpp, slow down the output rate.  The best hack is to
         *     putchar() each data byte, which slows down the data rate to UART0
         */
        while (n.getChar(&c, timeout_ms))
        {
            /*
             * Terminate loop early if we see end of output characters
             * We intentionally will not print the last terminating character such that the
             * parent command handler can print that instead to indicate end of output.
             */
            memmove(&lastChars[0], &lastChars[1], sizeof(endOfTx) - 1);
            lastChars[sizeof(endOfTx) - 1] = c;
            if (0 == memcmp(&lastChars[0], &endOfTx[0], sizeof(endOfTx))) {
                break;
            }

            ++count;
            if (!output.putChar(c, outBlockTime)) {
                dropped++;
            }
        }

        int pkts = count / MESH_DATA_PAYLOAD_SIZE;
        if (0 != (count % MESH_DATA_PAYLOAD_SIZE)) {
            pkts++;
        }

        output.printf("    Received %i bytes over %i packets\n", count, pkts);
        if (dropped > 0) {
            output.printf("Whoops!  Approximately %i bytes could not be printed because the output "
                            "channel is too slow.  Please follow the suggestions at "
                            "file: %s a little bit above while loop at line number %i\n",
                            dropped, __FILE__, __LINE__);
        }
    }

    return true;
}

static CMD_HANDLER_FUNC(wsFileTxHandler)
{
    /**
     * If other node is running same software, we will just use its "file" handler:
     * buffer <offset> <num bytes> ...
     * commit <filename> <file offset> <num bytes from buffer>
     */
    char srcFile[128] = { 0 };
    char dstFile[128] = { 0 };
    int timeout = 1000;
    int addr = 0;
    FIL file;

    if (3 != cmdParams.scanf("%128s %128s %i", &srcFile[0], &dstFile[0], &addr)) {
        return false;
    }
    if (FR_OK != f_open(&file, srcFile, FA_OPEN_EXISTING | FA_READ)) {
        return false;
    }

    NordicStream &n = NordicStream::getInstance();
    n.setDestAddr(addr);

    char c = 0;
    char buffer[512];
    int expectedChecksum = 0;
    unsigned int bytesRead = 0;
    unsigned int fileOffset = 0;
    unsigned int retries = 0;
    unsigned int retriesMax = 3;
    STR_ON_STACK(response, 128);

    // Sorry for the dirty hack #define
    #define doRetry() ++retries; n.printf("\n"); n.flush(); while (n.getChar(&c, timeout)); goto retry

    output.printf("Transfer %s --> %i:%s\n", srcFile, addr, dstFile);
    while(FR_OK == f_read(&file, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        retry:
        if (retries >= retriesMax) {
            break;
        }

        n.printf("file buffer 0 %i\n", bytesRead);
        n.flush();

        expectedChecksum = 0;
        for (unsigned int i=0; i < bytesRead; i++) {
            n.putChar(buffer[i]);
            expectedChecksum += buffer[i];
        }
        n.flush();

        // Confirm the checksum: response should be something like "Checksum: 123"
        n.gets((char*) response(), response.getCapacity(), timeout);
        response.eraseFirstWords(1);
        if ( (int)response != expectedChecksum) {
            output.printf("ERROR: Checksum Expected %i Actual %i\n", expectedChecksum, (int) response);
            doRetry();
        }

        // Make sure the file was written correctly, response should be "OK"
        n.printf("file commit %s %i %i\n", dstFile, fileOffset, bytesRead);
        n.flush();
        n.gets((char*) response(), response.getCapacity(), timeout);
        if (!response.containsIgnoreCase("ok")) {
            output.printf("ERROR: Remote node did not acknowledge file write (%s)\n", response());
            doRetry();
        }

        response = "";
        fileOffset += bytesRead;
        output.printf("Sent %i/%i\n", fileOffset, file.fsize);

        // Reset the retries such that only if we fail continuously we will give up
        if (retries > 0) {
            --retries;
        }
    }

    f_close(&file);
    return true;
}

static CMD_HANDLER_FUNC(wsRxHandler)
{
    bool rx = false;
    int timeout_ms = 1000;
    mesh_packet_t pkt;

    cmdParams.scanf("%i", &timeout_ms);

    while (wireless_get_rx_pkt(&pkt, timeout_ms)) {
        output.printf("Received data from %i\n", pkt.nwk.src);
        for (int i = 0; i < pkt.info.data_len; i++) {
            output.putChar(pkt.data[i]);
        }
        output.printf("\n");
        rx = true;
    }

    if (!rx) {
        output.putline("No data received");
    }

    return true;
}

static CMD_HANDLER_FUNC(wsAddrHandler)
{
    int addr = (int) cmdParams;
    output.printf("Set address to %i: %s\n", addr, mesh_set_node_address(addr) ? "OK" : "FAILED");
    return true;
}

CMD_HANDLER_FUNC(wsRteHandler)
{
    const char * const line = "-------------------------\n";
    const int routes = mesh_get_num_routing_entries();

    output.printf(line);
    output.printf("Routing table size is %i\n", routes);
    output.printf(line);

    if (routes > 0)
    {
        output.printf("| DST | Next HOP | HOPS |\n");
        output.printf(line);

        const mesh_rte_table_t *e = NULL;
        uint8_t i = 0;
        while ((e = mesh_get_routing_entry(i++))) {
            output.printf("| %3i |   %3i    | %3i  |\n", e->dst, e->next_hop, e->num_hops);
        }
        output.printf(line);
    }

    return true;
}

#if MESH_USE_STATISTICS
static CMD_HANDLER_FUNC(wsStatsHandler)
{
    mesh_stats_t stats = mesh_get_stats();
    wirelessHandlerPrintStats(output, &stats, mesh_get_node_address());
    return true;
}
#endif

static CMD_HANDLER_FUNC(wsTxHandler)
{
    char *addr_str = NULL;
    char *data_str = NULL;

    const bool ack = pDataParam;
    const int max_hops_to_use = 2;
    int timeout_ms = 1000;
    mesh_packet_t pkt;
    #if MESH_USE_STATISTICS
    mesh_stats_t stats = { 0 };
    #endif

    if (cmdParams.tokenize(" ", 2, &addr_str, &data_str) < 1) {
        return false;
    }

    /* Data is optional */
    const uint8_t dst_addr = atoi(addr_str);
    const uint8_t len = data_str ? strlen(data_str) : 0;

    // Flush any packets
    while (wireless_get_rx_pkt(&pkt, 0)) {
        output.putline("Discarded a stale wireless packet");
        ;
    }

    if (! wireless_send(dst_addr, ack ? mesh_pkt_ack : mesh_pkt_nack, data_str, len, max_hops_to_use)) {
        output.putline("Error sending packet, check parameters!");
    }
    /* If ack was requested, then we wait for the ack */
    else if (ack)
    {
        if(wireless_get_ack_pkt(&pkt, timeout_ms) && dst_addr == pkt.nwk.src)
        {
            #if MESH_USE_STATISTICS
            if (sizeof(stats) == pkt.info.data_len) {
                mesh_stats_t *p = (mesh_stats_t*) &(pkt.data[0]);
                wirelessHandlerPrintStats(output, p, pkt.nwk.src);
            }
            #endif

            /* Response to ping packet, so print the node name: */
            if (0 == len) {
                output.printf("Remote node name: '");
                for (int i=0; i<pkt.info.data_len; i++) {
                    output.putChar(pkt.data[i]);
                }
                output.printf("'\n");
            }
            else {
                output.putline("Received the acknowledgment!");
            }
        }
        else {
            output.printf("Packet sent to %s but no ACK received", addr_str);
        }
    }

    return true;
}

CMD_HANDLER_FUNC(wirelessHandler)
{
    static CommandProcessor *pCmdProcessor = NULL;
    if (NULL == pCmdProcessor)
    {
        pCmdProcessor = new CommandProcessor(8);
        pCmdProcessor->addHandler(wsStreamHandler,  "stream",   "'stream <addr> <msg>' : Stream a command to another board");
        pCmdProcessor->addHandler(wsFileTxHandler,  "transfer", "'transfer <src filename> <dst filename> <naddr>' : Transfer a file to another board");
        pCmdProcessor->addHandler(wsRxHandler,      "rx",       "'rx <time_ms>' : Poll for a packet");
        pCmdProcessor->addHandler(wsAddrHandler,    "addr",     "'addr <addr>   : Set the wireless address");
        pCmdProcessor->addHandler(wsRteHandler,     "routes",   "'routes' : See the wireless routes");

        void *ack = (void*) 1;
        void *nack = 0;
        pCmdProcessor->addHandler(wsTxHandler,      "ack",  "'ack <addr> <data>'  : Send a packet and wait for acknowledgment", ack);
        pCmdProcessor->addHandler(wsTxHandler,      "nack", "'nack <addr> <data>' : Send a packet", nack);

        #if MESH_USE_STATISTICS
        pCmdProcessor->addHandler(wsStatsHandler,   "stats", "'stats' : See the wireless stats");
        #endif
    }

    /* Display help for empty command */
    if (cmdParams == "") {
        cmdParams = "help";
    }

    return pCmdProcessor->handleCommand(cmdParams, output);
}

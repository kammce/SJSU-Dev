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

#include "nrf_stream.hpp"
#include "wireless.h"



/// The default hops to use unless set by the user
#define NRF_DEFAULT_HOPS        3



NordicStream::NordicStream(void) : mDestAddr(0), mHops(NRF_DEFAULT_HOPS)
{
    memset(&mRxBuffer, 0, sizeof(mRxBuffer));
    memset(&mTxBuffer, 0, sizeof(mTxBuffer));

    /* We just rely on the mesh network algorithm to make sure that it delivers
     * our packet, rather than handle the retry ourselves because if we retry
     * ourselves, the other node may get duplicate packets whereas the mesh
     * algorithm handles duplicates.  This seems to work fairly well in tests.
     */
    mesh_set_retry_count(MESH_RETRY_COUNT_MAX);
}

bool NordicStream::getChar(char* pInputChar, unsigned int timeout)
{
    bool dataAvailable = (mRxBuffer.dataPtr < mRxBuffer.pkt.info.data_len);

    /* If no buffered data, then try to get new packet from nordic wireless */
    if (!dataAvailable) {
        if (wireless_get_rx_pkt(&(mRxBuffer.pkt), timeout)) {
            mRxBuffer.dataPtr = 0;
            dataAvailable = (mRxBuffer.dataPtr < mRxBuffer.pkt.info.data_len);
        }
    }

    if (dataAvailable) {
        *pInputChar = mRxBuffer.pkt.data[mRxBuffer.dataPtr++];
    }

    return dataAvailable;
}

bool NordicStream::putChar(char out, unsigned int timeout)
{
    bool ok = true;

    /* Buffer the data */
    mTxBuffer.pkt.data[mTxBuffer.dataPtr++] = out;

    /* If buffer is full, flush the data */
    if (mTxBuffer.dataPtr >= MESH_DATA_PAYLOAD_SIZE) {
        ok = flush();
    }

    /* Always need to return true, otherwise CharDev class will halt printing further data */
    ok = true;
    return ok;
}

bool NordicStream::flush(void)
{
    bool ok = false;
    mesh_packet_t ackPkt;

    /* If destination address is not set, use the last source as destination */
    const uint8_t dst = (0 == mDestAddr) ? mRxBuffer.pkt.nwk.src : mDestAddr;
    const uint32_t ackTimeoutMs = mesh_get_max_timeout_before_packet_fails(dst);

    void *data = &(mTxBuffer.pkt.data[0]);
    uint8_t len = mTxBuffer.dataPtr;
    mTxBuffer.dataPtr = 0;

    /* Send the packet and wait for the ACK */
    if ((ok = wireless_send(dst, mesh_pkt_ack, data, len, mHops)))
    {
        if ((ok = wireless_get_ack_pkt(&ackPkt, ackTimeoutMs)))
        {
            ok = mesh_is_ack_ok(&ackPkt, dst);
        }
    }

    return ok;
}

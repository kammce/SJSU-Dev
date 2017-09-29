/// DBC file: /var/www/html/SJSU-DEV-Linux/firmware/lib/_can_dbc/243.dbc    Self node: 'DBG'  (ALL = 0)
/// This file can be included by a source file, for example: #include "generated.h"
#ifndef __GENEARTED_DBC_PARSER
#define __GENERATED_DBC_PARSER
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>



/// Extern function needed for dbc_encode_and_send()
extern bool dbc_app_send_can_msg(uint32_t mid, uint8_t dlc, uint8_t bytes[8]);

/// Missing in Action structure
typedef struct {
    uint32_t is_mia : 1;          ///< Missing in action flag
    uint32_t mia_counter_ms : 31; ///< Missing in action counter
} dbc_mia_info_t;

/// CAN message header structure
typedef struct { 
    uint32_t mid; ///< Message ID of the message
    uint8_t  dlc; ///< Data length of the message
} dbc_msg_hdr_t; 

static const dbc_msg_hdr_t COMMAND_HDR =                          {  100, 1 };




/// Message: COMMAND from 'DBG', DLC: 1 byte(s), MID: 100
typedef struct {
    uint8_t ENABLE : 1;                       ///< B0:0  Min: 0 Max: 1   Destination: DBG

    // No dbc_mia_info_t for a message that we will send
} COMMAND_t;


/// @{ These 'externs' need to be defined in a source file of your project
extern const uint32_t                             COMMAND__MIA_MS;
extern const COMMAND_t                            COMMAND__MIA_MSG;
/// @}


/// Encode DBG's 'COMMAND' message
/// @returns the message header of this message
static inline dbc_msg_hdr_t dbc_encode_COMMAND(uint8_t bytes[8], COMMAND_t *from)
{
    uint32_t raw;
    bytes[0]=bytes[1]=bytes[2]=bytes[3]=bytes[4]=bytes[5]=bytes[6]=bytes[7]=0;

    // Not doing min value check since the signal is unsigned already
    if(from->ENABLE > 1) { from->ENABLE = 1; } // Max value: 1
    raw = ((uint32_t)(((from->ENABLE)))) & 0x01;
    bytes[0] |= (((uint8_t)(raw) & 0x01)); ///< 1 bit(s) starting from B0

    return COMMAND_HDR;
}

/// Encode and send for dbc_encode_COMMAND() message
static inline bool dbc_encode_and_send_COMMAND(COMMAND_t *from)
{
    uint8_t bytes[8];
    const dbc_msg_hdr_t hdr = dbc_encode_COMMAND(bytes, from);
    return dbc_app_send_can_msg(hdr.mid, hdr.dlc, bytes);
}



/// Decode DBG's 'COMMAND' message
/// @param hdr  The header of the message to validate its DLC and MID; this can be NULL to skip this check
static inline bool dbc_decode_COMMAND(COMMAND_t *to, const uint8_t bytes[8], const dbc_msg_hdr_t *hdr)
{
    const bool success = true;
    // If msg header is provided, check if the DLC and the MID match
    if (NULL != hdr && (hdr->dlc != COMMAND_HDR.dlc || hdr->mid != COMMAND_HDR.mid)) {
        return !success;
    }

    uint32_t raw;
    raw  = ((uint32_t)((bytes[0]) & 0x01)); ///< 1 bit(s) from B0
    to->ENABLE = ((raw));

    to->mia_info.mia_counter_ms = 0; ///< Reset the MIA counter

    return success;
}


/// Handle the MIA for DBG's COMMAND message
/// @param   time_incr_ms  The time to increment the MIA counter with
/// @returns true if the MIA just occurred
/// @post    If the MIA counter reaches the MIA threshold, MIA struct will be copied to *msg
static inline bool dbc_handle_mia_COMMAND(COMMAND_t *msg, uint32_t time_incr_ms)
{
    bool mia_occurred = false;
    const dbc_mia_info_t old_mia = msg->mia_info;
    msg->mia_info.is_mia = (msg->mia_info.mia_counter_ms >= COMMAND__MIA_MS);

    if (!msg->mia_info.is_mia) { // Not MIA yet, so keep incrementing the MIA counter
        msg->mia_info.mia_counter_ms += time_incr_ms;
    }
    else if(!old_mia.is_mia)   { // Previously not MIA, but it is MIA now
        // Copy MIA struct, then re-write the MIA counter and is_mia that is overwriten
        *msg = COMMAND__MIA_MSG;
        msg->mia_info.mia_counter_ms = COMMAND__MIA_MS;
        msg->mia_info.is_mia = true;
        mia_occurred = true;
    }

    return mia_occurred;
}

#endif

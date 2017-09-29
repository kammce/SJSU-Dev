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
* @brief    Nordic driver
* @ingroup  WIRELESS
*
*	\par Example Code for a Nordic:
*	@code
*	// Initialize and send a packet:
*	nordic_init(32, 2450, 250);
*	nordic_standby1_to_tx_mode1();
*	nordic_queue_tx_fifo(arrayData, 32);
*	nordic_mode1_send_single_packet();
*
*	// Wait for packet to be sent
*	while(!nordic_intr_signal());
*	while(!nordic_is_tx_fifo_empty());
*	nordic_clear_packet_sent_flag();
*
*	// Optionally switch to RX Mode to receive data:
*	nordic_standby1_to_rx();
*
*	while(!nordic_intr_signal());
*	if(nordic_is_packet_available()) {
*		nordic_read_rx_fifo(arrayData, 32);
*		nordic_clear_packet_available_flag();
*	}
*	nordic_rx_to_Stanby1();
*
*	@endcode
*/

#ifndef NRF24L01PLUS_H_
#define NRF24L01PLUS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "utilities.h"   // delay_us()
#include "bio.h"         // board_io* functions
#include "ssp0.h"        // SPI Bus functions



#define NORDIC_EXCHANGE_SPI(byte)	            ssp0_exchange_byte(byte)
#define NORDIC_EXCHANGE_MULTI_BYTE(ptr, len)    ssp0_exchange_data(ptr, len)

#define NORDIC_LOCK_SPI()
#define NORDIC_UNLOCK_SPI()
#define NORDIC_DELAY_US(us)         delay_us(us)
#define NORDIC_CS_ENABLE()          board_io_nordic_cs()
#define NORDIC_CS_DISABLE()         board_io_nordic_ds()
#define NORDIC_CE_HIGH()            board_io_nordic_ce_high()
#define NORDIC_CE_LOW()             board_io_nordic_ce_low()
#define NORDIC_INT_SIGNAL()         (!board_io_nordic_irq_sig()) /* Signal is active low */



/// Initializes the chip to begin data exchange.
/// Enables only Pipe0, sets default address for the Pipe and disables Enhanced Shockburst.
/// @param payload   The payload per packet 1-32
/// @param mhz       The channel number from 2402-2525
/// @param bitrate_kbps   Can only be 250, 1000, or 2000
/// @post Rx interrupt is reflected on interrupt signal, but not Tx
void nordic_init(unsigned char payload, unsigned short mhz, unsigned short bitrate_kbps);

/// @returns TRUE if the nordic finds that the air is free to send data over the wireless medium
/// @warning This hasn't been fully tested.
bool nordic_is_air_free();

/// @returns TRUE if nordic interrupt signal is asserted
static inline bool nordic_intr_signal() { return NORDIC_INT_SIGNAL(); }

/* ******************************************************************************************************* */
/* ******************************         Nordic Modes of Operations       ******************************* */
/* ******************************************************************************************************* */

/// Puts Nordic from Rx Mode to Standby-1 Mode.
/// Standby-1 Mode can be used to send single Tx Packets and return to Rx easily.
void nordic_rx_to_Stanby1();

/// Puts Nordic from Standby-1 Mode to Rx Mode.
/// @note To change from Rx to Tx mode, you must use nordic_RxTo_Stanby1() first!
void nordic_standby1_to_rx();

/// Puts Nordic from Standby-1 Mode to Tx Mode-1.
/// @note Tx Mode-1 is recommended for low throughput data transfers with Software ACKs
///       since this mode can more easily transition to Rx Mode.
void nordic_standby1_to_tx_mode1();

/// Puts Nordic from Standby-1 Mode to Tx Mode-2
/// @note Tx Mode-2 is recommended for high throughput data transfers since this mode
///       can send multiple packets in "Tx Mode" without going to Standby.
///       The only compromise is that you must use nordic_TxMode2_toStandby1_throughPowerDown()
///       to transition back to Rx.  Also, Nordic uses more power in this mode compared to Mode-1.
void nordic_standby1_to_tx_mode2();

/// This transitions from Tx Mode-2 to Standby-1 without sending a packet.
/// This powers-down nordic, then powers it up, which takes about 150uS
void nordic_tx_mode2_to_standby1_through_power_down();





/* ******************************************************************************************************* */
/* *****************************        Nordic Transmission functions       ****************************** */
/* ******************************************************************************************************* */

/// @return		True if TX FIFO is Full
bool nordic_is_tx_fifo_full();

/// @return		True if TX FIFO is fully empty.
bool nordic_is_tx_fifo_empty();

/// Clears all interrupt flags to de-assert interrupt signal.
void nordic_clear_all_intr_flags();

/// Queues Data on the TX Fifo (FIFO can hold 3 Packets)
/// @pre			TX Fifo must not be FULL
/// @param data		The char array data pointer
/// @param length	The length of the data
/// @note In Tx Mode-2 data is sent automatically.
///       In Tx Mode-1 you must use nordic_mode1_send_single_packet()
void nordic_queue_tx_fifo(char *data, unsigned short length);

/// Sends the data in the Tx FIFO.
/// @post	Returns back to Standby-1 after sending the data.
void nordic_mode1_send_single_packet(char *data, unsigned short length);

/// @return			True if a TX Packet was sent.
bool nordic_is_packet_sent();

/// Clears the Tx Packet sent flag
void nordic_clear_packet_sent_flag();

/// @return			True if max retries reached while transmitting packet in enhanced shockburst mode.
bool nordic_is_max_retries_reached();

/// Clears the Max retries reached flag
void nordic_clear_max_retries_flag();

/// Flushes(clears) the TX FIFO Data
void nordic_flush_tx_fifo();



/* ******************************************************************************************************* */
/* ******************************         Nordic Receive functions         ******************************* */
/* ******************************************************************************************************* */

/// @return		True if an RX Packet is available.
bool nordic_is_packet_available();

/// Clears the RX Packet Received Flag
void nordic_clear_packet_available_flag();

/// Dequeues Data from the RX Fifo
/// @pre			RX Fifo must contain data
/// @param data		The char array data pointer
/// @param length	The length of the data
/// @return			The data pipe data was received on (0-5)
char nordic_read_rx_fifo(char *data, unsigned short length);

/// Flushes(clears) the RX FIFO Data
void nordic_flush_rx_fifo();







/* ******************************************************************************************************* */
/* *****************************       Nordic Configuration Functions       ****************************** */
/* ******************************************************************************************************* */

/// Sets the triggers that enable the Interrupt Pin
/// @param rx, tx, maxTransmissions		If true, interrupt pin will trigger upon these detections.
/// @note	These are all enabled by default at power-up
void nordic_set_intr_signals(bool rx, bool tx, bool maxTransmissions);

/// @returns the interrupt status register (not the interrupt signal).
char nordic_get_intr_reg_status(void);

/// Sets the length of the CRC of over-the-air packets
/// @param length		Must be 0, 1, or 2.  Use 0 to disable CRC checking
void nordic_set_crc(unsigned char length);

/// Powers up the Nordic Chip
void nordic_power_up();

/// Powers down the Nordic chip (uses few nano-amps in power-down)
void nordic_power_down();

/// Sets the Channel Number in Mhz
/// @warning	Be careful with FCC restricted range!
/// @param MHz  Must be between 2400 - 2525
void nordic_set_channel(unsigned short MHz);

/// Enables or disables continuous carrier transmission
void nordic_set_continous_carrier_transmit(bool enable);

/// Sets the over-the-air data-rate
/// @param kbps 	Must be 250, 1000, or 2000
/// @note	Lower data rate yields longer range, but potentially decreases delivery rate.
void nordic_set_air_data_rate(unsigned short kbps);

/// Sets the power level of the chip
/// @param powerLevel	Must be 0-3 with 3 being the highest
void nordic_set_power_level(unsigned char powerLevel);





/* ******************************************************************************************************* */
/* *************************        Nordic Enhanced Shockburst functions        ************************** */
/* ******************************************************************************************************* */

/// Sets the Enhanced Shockburst auto-transmit options
/// @param txDelayUs	The delay in uS from 250 - 4000 for retransmission
/// @param retries		The number of retries 1-15
void nordic_set_auto_transmit_options(unsigned short txDelayUs, unsigned char retries);

/// Gets the total number of lost packets.
/// @param clear	If true, clears the count
/// @return			The total lost packets 0-15
char nordic_get_lost_packet_cnt(bool clear);

/// Gets the total number of retransmissions.
/// @note			The count is cleared when new transmission starts.
char nordic_get_retransmit_count();




/* ******************************************************************************************************* */
/* *************************         Nordic Pipes & Addresses functions         ************************** */
/* ******************************************************************************************************* */

/// Sets the payload for the pipes.
/// @param pipeNumber	The pipe number from 0-5
/// @param payload		The payload 0-32 (0 means pipe not used)
void nordic_set_payload_for_pipe(unsigned char pipeNumber, unsigned char payload);

/// Sets the width of the address.
/// @param width	The address width, must be 3-5
/// @note			Lower increases data throughput, higher might catch other wireless traffic.
void nordic_set_addr_width(unsigned short width);

/// Sets the address of the transmitter.
/// @param address	The char array of address.
/// @param length	The length of address, must be same as what was set at nordic_setAddressWidth()
void nordic_set_tx_address(char* address, unsigned short length);

/// Sets the reciver's Pipe 0 Address.
/// @param address	The char array of address.
/// @param length	The length of address, must be same as what was set at nordic_setAddressWidth()
void nordic_set_rx_pipe0_addr(char* address, unsigned short length);

/// Sets the reciver's Pipe 1 Address.
/// @param address	The char array of address.
/// @param length	The length of address, must be same as what was set at nordic_setAddressWidth()
void nordic_set_rx_pipe1_addr(char* address, unsigned short length);

/// Sets the last byte of the address of Pipe2-Pipe5
/// @note	The top bytes are the same as Pipe1's address.
/// @param 	address	The last byte of this pipe's address.
//@{
void nordic_set_rx_pipe2_lsb_addr(char address);
void nordic_set_rx_pipe3_lsb_addr(char address);
void nordic_set_rx_pipe4_lsb_addr(char address);
void nordic_set_rx_pipe5_lsb_addr(char address);
//@}

/// Sets the auto-ack (enhanced shockburst for the pipes)
/// @param pipe0, pipe1, pipe2, pipe3, pipe4, pipe5		Set to true, to enable auto-ack on the pipe.
void nordic_set_auto_ack_for_pipes(bool pipe0, bool pipe1, bool pipe2, bool pipe3, bool pipe4, bool pipe5);

/// Enables or disables the pipes
/// @param pipe0, pipe1, pipe2, pipe3, pipe4, pipe5		Set to true, to enable the pipe.
void nordic_enable_pipes(bool pipe0, bool pipe1, bool pipe2, bool pipe3, bool pipe4, bool pipe5);





#ifdef __cplusplus
}
#endif

#endif /* NRF24L01PLUS_H_ */

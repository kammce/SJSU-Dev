from __future__ import division

APPLICATION_VERSION = '1.2'

import argparse
import functools
import logging
import math
import struct
import sys
import time

import serial
import serial.serialutil
import serial.tools.list_ports as port_list
from intelhex import IntelHex

TOOL_NAME = 'pyFLASH - HYPERLOAD'
TOOL_INFO = 'Flashing Tool for devices running the HYPERLOAD protocol'
INITIAL_DEVICE_BAUD = 38400

parser = argparse.ArgumentParser()

parser.add_argument(
    '-d',
    '--device',
    type=str,
    default="",
    help='Path to serial device file. In linux the name should be '
    'something similar to "/dev/ttyUSB0", WSL "/dev/ttyS0", and '
    'Max OSX "/dev/tty-usbserial-AJ20A5".')

parser.add_argument(
    '-b',
    '--baud',
    type=int,
    help='bitrate/speed to send program over.',
    default=38400,
    choices=[
        4800, 9600, 19200, 38400, 57600, 115200, 230400, 576000, 921600,
        1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000
    ])

parser.add_argument(
    '-c',
    '--clockspeed',
    type=int,
    help='clock speed in Hz of processor during programming.',
    default=48000000)

parser.add_argument(
    '-v',
    '--verbose',
    help='Enable version debug message output.',
    action='store_true')

parser.add_argument(
    '-a',
    '--animation',
    type=str,
    help='Choose which animation you would like to see when programming :).',
    choices=[
        "clocks",
        "circles",
        "quadrants",
        "trigrams",
        "squarefills",
        "spaces",
        "braille",
    ],
    default="clocks")

parser.add_argument(
    'binary',
    help='path to the firmware.bin file you want to program the board with.')

args = parser.parse_args()

if args.verbose:
  logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
else:
  logging.basicConfig(stream=sys.stdout, level=logging.INFO)

# Global Defines
SPECIAL_CHAR = {'Dollar': b'$', 'OK': b'!', 'NextLine': b'\n', 'STAR': b'*'}
HANDSHAKE_SEQUENCE = [
    0xFF,  # Signature of Hyperload
    0x55,  # Host -> Hyperload Request to flash device
    0xAA  # Hyperload -> Host accept request to flash
]

## Animation stuff
ANIMATIONS = {
    "circles": [0x25D0, 0x25D3, 0x25D1, 0x25D2],
    "quadrants": [0x259F, 0x2599, 0x259B, 0x259C],
    "trigrams": [0x2630, 0x2631, 0x2632, 0x2634],
    "squarefills": [0x25E7, 0x25E9, 0x25E8, 0x25EA],
    "spaces": [0x2008, 0x2008, 0x2008, 0x2008],
    "clocks": [
        0x1F55B, 0x1F550, 0x1F551, 0x1F552, 0x1F553, 0x1F554, 0x1F555, 0x1F556,
        0x1F557, 0x1F558, 0x1F559, 0x1F55A
    ],
    "braille":
    [0x2840, 0x2844, 0x2846, 0x2847, 0x2840, 0x28c7, 0x28e7, 0x28f7, 0x28fF],
}

VALID_SERIAL_PORT_DESCRIPTIONS = [
    "CP2102N USB to UART Bridge Controller",  # SJ2 Rev 1.x
    "FT232R USB UART",  # SJOne Rev 4 (and probably the rest too)
    "FT231X USB UART",  # SJ2-Mini Rev 1
    "n/a",  # Description of all serial ports on WSL
    "USER SUPPLIED PORT",  # Description of a device port passed by --device
]


def getBoardParameters(description_string):
  # Parsing String to obtain required Board Parameters
  board_parameters_list = description_string.replace("\n", "").split(b':')
  board_parameters_dict = {
      'Board': board_parameters_list[0],
      'BlockSize': board_parameters_list[1],
      'BootloaderSize': (int(board_parameters_list[2]) * 2),
      'FlashSize': board_parameters_list[3]
  }
  print("\n******* Board Information *******")

  board = (str)(board_parameters_dict['Board'])
  print("Board              = " + board)

  block_chunk = (str)(board_parameters_dict['BlockSize']) + " bytes"
  print("Block (Chunk) Size = " + block_chunk)

  bootloader_size = (str)(board_parameters_dict['BootloaderSize']) + " bytes"
  print("Bootloader Size    = " + bootloader_size)

  flash_size = (str)(board_parameters_dict['FlashSize']) + " KB"
  print("Flash Size         = " + flash_size)

  print("***********************************\n")
  return board_parameters_dict


def getControlWord(baud_rate, cpu_speed):
  logging.debug("Retrieving Control Word")
  controlWord = ((cpu_speed / (baud_rate * 16)) - 1)
  return controlWord


def getPageContent(binary, current_block, page_size):
  start_offset = current_block * page_size
  end_offset = (start_offset + page_size - 1)
  page_content = bytearray(page_size)

  for x in range(0, page_size):
    page_content[x] = binary[x + (current_block * page_size)]

  return page_content


def getChecksum(blocks):
  return functools.reduce(lambda a, b: (a + b) % 256, blocks)


def unichar(i):
  try:
    return chr(i)
  except ValueError:
    return struct.pack('i', i).decode('utf-32')


def reset_device(port):
  # Put device into reset state
  port.rts = True
  port.dtr = True
  # Hold in reset state for 50 milliseconds
  time.sleep(0.05)
  # Clear all port buffers
  port.reset_input_buffer()
  port.reset_output_buffer()
  port.flush()
  # Remove reset signal to allow device to boot up
  port.rts = False
  port.dtr = False


def port_read(port, number_of_bytes):
  return bytearray(port.read(number_of_bytes))


def port_read_byte(port):
  try:
    return bytearray(port.read(1))[0]
  except IndexError:
    return False


def port_write_and_verify(port, payload, error_message="", debug_message=""):
  bytes_sent = port.write(bytearray(payload))
  if bytes_sent != len(payload):
    logging.error(error_message)
    return False
  else:
    logging.debug(debug_message)
    return True


def proress_bar(bar_length, current_block, total_blocks):
  bar_len = bar_length
  filled_len = int(round(bar_len * (current_block + 1) / float(total_blocks)))

  percents = round(100.0 * (current_block + 1) / float(total_blocks), 1)

  bar = ' ' * (filled_len - 1)
  bar = bar + unichar(0x15E7)
  bar = bar + unichar(0x2219) * (bar_len - filled_len)

  suffix = "Block # {0}/{1} flashed!".format(current_block + 1,
                                             int(total_blocks))

  sys.stdout.write(
      '[%s] %s%% %s  ... %s\r' %
      (bar, percents,
       unichar(selected_animation[current_block % len(selected_animation)]),
       suffix))

  sys.stdout.flush()


class HyperloadStates:
  FindPorts = 1
  FlashRequest = 2
  SetBaudRates = 3
  GetSystemInfo = 4
  PrepareBinaryForFlashing = 5
  TransmitApplicationToBoard = 6
  DetermineIfFlashWasSuccessful = 7
  BailOut = 8


board_parameters = None


class dotdict(dict):
  """dot.notation access to dictionary attributes"""
  __getattr__ = dict.get
  __setattr__ = dict.__setitem__
  __delattr__ = dict.__delitem__


def Hyperload2(binary_file_path, clockspeed, baud, selected_animation, device):
  with open(binary_file_path, mode='rb') as file:
    application_binary = file.read()
  # Initialize HyperloadStates variable
  state = HyperloadStates.FindPorts
  port = None
  while True:
    if state == HyperloadStates.FindPorts:
      if device:
        list_of_serial_devices = [
            dotdict({
                "device": device,
                "description": "USER SUPPLIED PORT"
            })
        ]
      else:
        list_of_serial_devices = list(port_list.comports())
      # Find all serial ports and attempt to reset and connect to them
      for port_info in list_of_serial_devices:
        try:
          try:
            logging.debug(port_info.device)
            logging.debug(port_info.description)
            VALID_SERIAL_PORT_DESCRIPTIONS.index(port_info.description)
          except ValueError:
            continue
          port = serial.Serial(
              port=port_info.device,
              baudrate=INITIAL_DEVICE_BAUD,
              parity=serial.PARITY_NONE,
              stopbits=serial.STOPBITS_ONE,
              bytesize=serial.EIGHTBITS,
              timeout=3)

          # Reset the device.
          logging.info("Resetting Device: %s - %s" % (port_info.device,
                                                      port_info.description))
          reset_device(port)
          # Check if device emits a Hyperload signature after reset
          logging.info("Querying device: %s" % port_info.device)
          hyperload_signature = port_read_byte(port)
          logging.info("hyperload_signature: %d" % hyperload_signature)
          # If it does, immediately break the loop. This will stop at the first
          # device with a hyperload response
          if hyperload_signature == HANDSHAKE_SEQUENCE[0]:
            logging.info("Found Hyperload Device %s", port_info.device)
            state = HyperloadStates.FlashRequest
            break
          else:
            port.close()
        # Skip any devices that result in a serial exception.
        # This is mostly  meant for users of WSL, where /dev/ttySx serial
        # devices that are not backed by a COM port will throw an exception
        # when attempting to open them.
        except serial.serialutil.SerialException:
          logging.debug("Exception on port %s, desc: %s" %
                        (port_info.name, port_info.description))
          continue
      # If the state hasn't changed then none of the ports found emitted a
      # Hyperload Signature, thus we should bail out.
      if state == HyperloadStates.FindPorts:
        logging.error("Couldn't Find any Hyperload Devices")
        state = HyperloadStates.BailOut

    if state == HyperloadStates.FlashRequest:
      port_write_and_verify(port, [HANDSHAKE_SEQUENCE[1]],
                            "Failed to send Hyperload flash request",
                            "Sending Request to flash!")

      sj2_device_discovered = port_read_byte(port)
      if sj2_device_discovered == HANDSHAKE_SEQUENCE[2]:
        logging.debug("Received " + (str)(repr(sj2_device_discovered)) +
                      ", Sending Control Word..")
        state = HyperloadStates.SetBaudRates

    if state == HyperloadStates.SetBaudRates:
      baud_rate_control_integer = int(getControlWord(baud, clockspeed))
      logging.debug(type(baud_rate_control_integer))

      control_word = bytearray(struct.pack('<i', baud_rate_control_integer))

      port_write_and_verify(port, control_word, "Sending control word failed",
                            "Sending Control Word Successful!")

      ackknowledge_byte = port_read_byte(port)

      if ackknowledge_byte != control_word[0]:
        logging.debug(control_word[0])
        logging.debug(ackknowledge_byte)
        logging.error("Failed to receive Control Word Ack")
        state = HyperloadStates.BailOut
      else:
        logging.debug("Ack from Hyperload received!")
        port.baudrate = baud
        state = HyperloadStates.GetSystemInfo

    if state == HyperloadStates.GetSystemInfo:
      # Read the CPU Desc String
      start_of_cpu_description = port_read_byte(port)
      if chr(start_of_cpu_description) != SPECIAL_CHAR['Dollar']:
        logging.error("Failed to read CPU Description String")
        state = HyperloadStates.BailOut
      else:
        logging.debug("Reading CPU Desc String...")

        board_description = SPECIAL_CHAR['Dollar'] + port.read_until(b'\n')
        logging.debug("CPU Description String = %s", board_description)

        board_parameters = getBoardParameters(board_description)

        # Receive OK from Hyperload
        if chr(port_read_byte(port)) != SPECIAL_CHAR['OK']:
          logging.error("Failed to Receive OK")
          state = HyperloadStates.BailOut
        else:
          logging.debug("OK Received! Sending Block")
          state = HyperloadStates.PrepareBinaryForFlashing

    if state == HyperloadStates.PrepareBinaryForFlashing:
      # Sending Blocks of Binary File
      total_blocks = (
          len(application_binary) * 1.0 / int(board_parameters['BlockSize']))
      logging.debug("Total Blocks = %f", total_blocks)

      paddingCount = len(application_binary) - (
          (len(application_binary)) % int(board_parameters['BlockSize']))
      logging.debug("Total Padding Count = %d", paddingCount)

      total_blocks = math.ceil(total_blocks)
      logging.info("Total # of Blocks to be Flashed = %d", total_blocks)

      # Pad 0's to application_binary if required.
      application_binary = bytearray(application_binary)
      application_binary += (b'\x00' * paddingCount)
      state = HyperloadStates.TransmitApplicationToBoard

    if state == HyperloadStates.TransmitApplicationToBoard:
      current_block = 0

      while current_block < total_blocks:
        # Send current block number to Hyperload
        port_write_and_verify(port, struct.pack('>H', current_block),
                              "Error in Sending BlockCount")

        logging.debug("Number of Blocks = %d", current_block)

        block_content = getPageContent(application_binary, current_block,
                                       int(board_parameters['BlockSize']))

        port_write_and_verify(port, block_content,
                              "Failed to sending Data Block Content")

        logging.debug("Size of Block Written = %d", len(block_content))

        checksum = getChecksum(block_content)
        logging.debug("Checksum = %d [0x%x]", checksum, checksum)

        port_write_and_verify(port, [checksum],
                              "Failed to send Entire Data Block")

        if chr(port_read_byte(port)) != SPECIAL_CHAR['OK']:
          logging.error(
              "Failed to Receive Ack.. Retrying #%d\n" % int(current_block))
        else:
          proress_bar(25, current_block, total_blocks)
          current_block = current_block + 1

      if current_block != total_blocks:
        logging.error("Not all blocks were flashed")
        logging.error("Total = " + str(total_blocks))
        logging.error("# of Blocks Flashed = " + str(current_block))
        state = HyperloadStates.BailOut
      else:
        state = HyperloadStates.DetermineIfFlashWasSuccessful

    if state == HyperloadStates.DetermineIfFlashWasSuccessful:
      end_transfer = bytearray([0xFF, 0xFF])
      port_write_and_verify(port, end_transfer,
                            "Could not send end of transfer")
      final_acknowledge = port_read_byte(port)

      if chr(final_acknowledge) != SPECIAL_CHAR['STAR']:
        logging.debug(final_acknowledge)
        logging.error("Final Ack Not Received")
      else:
        port.baudrate = INITIAL_DEVICE_BAUD
        logging.debug("Received Ack")
        logging.info("\n\nFlashing Successful!")

      break

    if state == HyperloadStates.BailOut:
      logging.error("Bailing out of Hyperload")
      break

  if port:
    port.baudrate = INITIAL_DEVICE_BAUD
    port.close()


### Main Program ###
if __name__ == "__main__":
  args = parser.parse_args()

  hex_path_length = (len(args.binary) + 20)
  single_dashes = str('-' * hex_path_length)

  selected_animation = ANIMATIONS[args.animation]

  print('#######################')
  print('{}'.format(TOOL_NAME))
  print('{}'.format(TOOL_INFO))
  print('#######################')
  print('Version    :  {}'.format(APPLICATION_VERSION))
  print('#######################')

  print(single_dashes)
  print('Hex File Path = "' + args.binary + '"')
  print(single_dashes)

  Hyperload2(args.binary, args.clockspeed, args.baud, selected_animation,
             args.device)

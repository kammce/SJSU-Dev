#!/usr/bin/python

import sys, getopt
import re
from collections import OrderedDict

"""
@Author: Preet
This parses the Vector DBC file to generate code to marshal and unmarshal DBC defined messages

Use Python (I used Python 3.5)
python dbc_parse.py -i 243.dbc -s MOTOR
Generate all code: dbc_parse.py -i 243.dbc -s MOTOR -a all > generated.h
"""

LINE_BEG = '%'


def is_empty(s):
    if s:
        return False
    else:
        return True


def MIN(x, y):
    if (x < y):
        return x
    else:
        return y


class Signal(object):
    def __init__(self, name, bit_start, bit_size, endian_and_sign, scale, offset, min_val, max_val, recipients, mux, signal_min, signal_max):
        self.has_field_type = False
        self.name = name
        self.bit_start = int(bit_start)
        self.bit_size = int(bit_size)
        self.endian_and_sign = endian_and_sign

        self.offset = float(offset)
        self.offset_str = offset
        self.scale = float(scale)
        self.scale_str = scale
        self.min_val = float(min_val)
        self.min_val_str = min_val
        self.max_val = float(max_val)
        self.max_val_str = max_val
        self.signal_min = signal_min
        self.signal_max = signal_max

        self.recipients = recipients
        self.enum_info = {}
        self.mux = mux
        if self.mux == '':
            self.mux = '__NO__MUX__'

    # Returns true if the signal uses an enumeration type
    def is_enum_type(self):
        return not is_empty(self.enum_info)

    # Returns true if the signal is part of MUX'd data
    def is_muxed(self):
        return '__NO__MUX__' != self.mux

    # Returns true if the signal should be an unsigned type
    def is_unsigned_var(self):
        t = self.get_code_var_type()
        return t.find("uint") == 0

    # Returns true if the signal is defined in the DBC as a signed type
    def is_real_signed(self):
        return '-' == self.endian_and_sign[1]

    # Returns the variable type (float, int, or enum) based ont he signal data range
    def get_code_var_type(self):
        if '.' in self.scale_str:
            return "float"
        else:
            if not is_empty(self.enum_info):
                return self.name + "_E"

            _max = (2 ** self.bit_size) * self.scale
            if self.is_real_signed():
                _max *= 2

            t = "uint32_t"
            if _max <= 256:
                t = "uint8_t"
            elif _max <= 65536:
                t = "uint16_t"

            # If the signal is signed, or the offset is negative, remove "u" to use "int" type.
            if self.is_real_signed() or self.offset < 0:
                t = t[1:]

            return t

    # Get the signal declaration with the variable type and bit size
    def get_signal_code(self):
        code = ""
        code += "    " + self.get_code_var_type() + " " + self.name
        if self.bit_size <= 4:
            code += " : " + str(self.bit_size) + ";"
        else:
            code += ";"

        # Align the start of the comments
        for i in range(len(code), 45):
            code += " "

        # Comment with Min/Max
        code += " ///< B" + str(self.bit_start + self.bit_size - 1) + ":" + str(self.bit_start)
        if self.min_val != 0 or self.max_val != 0:
            code += "  Min: " + self.min_val_str + " Max: " + self.max_val_str

        # Comment with destination nodes:
        code += "   Destination: "
        for r in self.recipients:
            if r == self.recipients[0]:
                code += r
            else:
                code += "," + r

        return code + "\n"

    # Get the encode code of the signal
    def get_encode_code(self, raw_sig_name, var_name):
        code = ''

        # Min/Max check
        if self.min_val != 0 or self.max_val != 0:
            # If signal is unsigned, and min value is zero, then do not check for '< 0'
            if not (self.is_unsigned_var() and self.min_val == 0):
                code += ("    if(" + var_name + " < " + self.min_val_str + ") { " + var_name + " = " + self.min_val_str + "; } // Min value: " + self.min_val_str + "\n")
            else:
                code += "    // Not doing min value check since the signal is unsigned already\n"
            code += ("    if(" + var_name + " > " + self.max_val_str + ") { " + var_name + " = " + self.max_val_str + "; } // Max value: " + self.max_val_str + "\n")

        # Compute binary value
        # Encode should subtract offset then divide
        # TODO: Might have to add -0.5 for a negative signal
        raw_sig_code = "    " + raw_sig_name + " = "
        raw_sig_code += "((uint32_t)(((" + var_name + " - (" + self.offset_str + ")) / " + str(self.scale) + ") + 0.5))"
        if self.is_real_signed():
            s = "    // Stuff a real signed number into the DBC " + str(self.bit_size) + "-bit signal\n"
            s += raw_sig_code + (" & 0x" + format(2 ** self.bit_size - 1, '02x') + ";\n")
        else:
            s = raw_sig_code + (" & 0x" + format(2 ** self.bit_size - 1, '02x') + ";\n")

        # Optimize
        s = s.replace(" - (0)", "")
        s = s.replace(" / 1.0)", ")")
        if self.scale == 1:
            s = s.replace(" + 0.5", "")

        # Add the code
        code += s

        # Stuff the raw data into individual bytes
        bit_pos = self.bit_start
        remaining = self.bit_size
        byte_num = int(self.bit_start / 8)
        while remaining > 0:
            bits_in_this_byte = MIN(8 - (bit_pos % 8), remaining)

            s = ""
            s += ("    bytes[" + str(byte_num) + "] |= (((uint8_t)(" + raw_sig_name + " >> " + str(
                bit_pos - self.bit_start) + ")")
            s += (" & 0x" + format(2 ** bits_in_this_byte - 1, '02x') + ") << " + str(bit_pos % 8) + ")")
            s += ("; ///< " + str(bits_in_this_byte) + " bit(s) starting from B" + str(bit_pos) + "\n")

            # Optimize
            s = s.replace(" >> 0", "")
            s = s.replace(" << 0", "")
            # Cannot optimize by removing 0xff just for code safety
            #s = s.replace(" & 0xff", "")

            code += s
            byte_num += 1

            bit_pos += bits_in_this_byte
            remaining -= bits_in_this_byte
        return code

    # Get the decode code of the signal
    def get_decode_code(self, raw_sig_name, prefix=''):
        # Little and Big Endian:
        bit_pos = self.bit_start
        remaining = self.bit_size
        byte_num = int(self.bit_start / 8)
        bit_count = 0
        code = ''

        while remaining > 0:
            bits_in_this_byte = MIN(8 - (bit_pos % 8), remaining)

            s = ""
            s += (
            LINE_BEG + raw_sig_name + " |= ((uint32_t)((bytes[" + str(byte_num) + "] >> " + str(bit_pos % 8) + ")")
            s += (" & 0x" + format(2 ** bits_in_this_byte - 1, '02x') + ")) << " + str(bit_count) + ";")
            s += (" ///< " + str(bits_in_this_byte) + " bit(s) from B" + str(bit_pos) + "\n")

            # Optimize
            s = s.replace(" >> 0", "")
            s = s.replace(" << 0", "")
            s = s.replace(" & 0xff", "")

            code += s
            if bit_count == 0:
                code = code.replace("|=", " =")

            byte_num += 1
            bit_pos += bits_in_this_byte
            remaining -= bits_in_this_byte
            bit_count += bits_in_this_byte

        # Decode/get should multiply then add the offset
        enum_cast = ''
        if self.is_enum_type():
            enum_cast = "(" + self.get_code_var_type() + ")"

        # If the signal is not defined as a signed, then we will use this code
        unsigned_code = (prefix + self.name + " = " + enum_cast + "((" + raw_sig_name + " * " + str(self.scale) + ") + (" + self.offset_str + "));\n")

        if self.is_real_signed():
            mask = "(1 << " + str(self.bit_size - 1) + ")"
            s = LINE_BEG + "if (" + raw_sig_name + " & " + mask + ") { // Check signed bit\n"
            s += LINE_BEG + prefix + self.name + " = " + enum_cast
            s += "((((0xFFFFFFFF << " + str(self.bit_size - 1) + ") | " + raw_sig_name + ") * " + str(self.scale) + ") + (" + self.offset_str + "));\n"
            s += LINE_BEG + "} " + "else {\n"
            s += LINE_BEG + unsigned_code
            s += LINE_BEG + "}\n"
        else:
            s = unsigned_code

        # Optimize
        s = s.replace(" + (0)", "")
        s = s.replace(" * 1.0)", ")")
        code += s

        return code


class Message(object):
    """
    Message Object that contains the list of signals inside
    """

    def __init__(self, mid, name, dlc, sender):
        self.mid = mid
        self.name = name
        self.dlc = dlc
        self.sender = sender
        self.signals = OrderedDict()

    # Adds the signal to the dictionary of signals of this message
    def add_signal(self, s):
        self.signals[s.name] = s

    # Returns the struct name derived from the message name
    def get_struct_name(self):
        return "%s_t" % (self.name)
        # return "%s_TX_%s_t" % (self.sender, self.name)

    # Returns true if the node is a recipient of at least one signal contained in the message
    def is_recipient_of_at_least_one_sig(self, node):
        for key in self.signals:
            if node in self.signals[key].recipients:
                return True
        return False

    # Returns true if at least one message signal is a MUX'd type
    def contains_muxed_signals(self):
        for key in self.signals:
            if self.signals[key].is_muxed():
                return True
        return False

    # Returne true if one or more of the message signal is an enumeration type
    def contains_enums(self):
        for key in self.signals:
            if not is_empty(self.signals[key].enum_info):
                return True
        return False

    def get_muxes(self):
        muxes = []
        for key in self.signals:
            if self.signals[key].is_muxed() and self.signals[key].mux not in muxes:
                muxes.append(self.signals[key].mux)
        return muxes

    # Returns the message signal that defines the MUX value
    def get_mux_index_signal(self):
        for key in self.signals:
            if self.signals[key].is_muxed() and self.signals[key].mux == "M":
                return self.signals[key]
        return ""

    # TODO: Do not generate this struct if we are not the recipient of any of the signals of this MUX
    def get_struct_for_mux(self, mux, non_muxed_signals, gen_mia_struct):
        code = '\n'
        code += ("/// Struct for MUX: " + mux + " (used for transmitting)\n")
        code += ("typedef struct {\n")
        code += non_muxed_signals

        for key in self.signals:
            if self.signals[key].mux == mux:
                code += (self.signals[key].get_signal_code())
        if gen_mia_struct:
            code += ("\n    dbc_mia_info_t mia_info;")
        else:
            code += ("\n    // No dbc_mia_info_t for a message that we will send")
        code += ("\n} " + self.get_struct_name()[:-2] + "_" + str(mux) + "_t;\n")
        return code

    def gen_converted_struct(self, self_node, gen_all):
        code = ''
        if self.contains_muxed_signals():
            # Non Muxed signals in this struct, exclude the MUXED index
            non_muxed_signals = ''
            for key in self.signals:
                if not self.signals[key].is_muxed() and not self.signals[key].mux == "M":
                    non_muxed_signals += (self.signals[key].get_signal_code())

            # MUX'd data structures
            code = ("/// @{ MUX'd message: " + self.name + "\n")
            muxes = self.get_muxes()
            gen_mia_struct = gen_all or self_node != self.sender
            for m in muxes[1:]:
                code += self.get_struct_for_mux(m, non_muxed_signals, gen_mia_struct)

            # Parent data structure
            code += "\n/// Struct with all the child MUX'd signals (Used for receiving)\n"
            code += "typedef struct {\n"

            # Child struct instances of the Mux'd signals
            for m in muxes[1:]:
                code += ("    " + self.get_struct_name()[:-2] + "_" + str(m) + "_t " + str(m) + "; ///< MUX'd structure\n")
            code += ("} " + self.get_struct_name() + ";\n")

            code += ("/// @} MUX'd message\n")
        else:
            code += (
            "\n/// Message: " + self.name + " from '" + self.sender + "', DLC: " + self.dlc + " byte(s), MID: " + self.mid + "\n")
            code += ("typedef struct {\n")
            for key in self.signals:
                if gen_all or self_node in self.signals[key].recipients or self.sender == self_node:
                    code += (self.signals[key].get_signal_code())

            if gen_all or self_node != self.sender:
                code += ("\n    dbc_mia_info_t mia_info;")
            else:
                code += ("\n    // No dbc_mia_info_t for a message that we will send")
            code += ("\n} " + self.get_struct_name() + ";\n")

        return code

    def get_encode_and_send(self, name):
        code = ''
        code += ("\n/// Encode and send for dbc_encode_" + name + "() message\n")
        code += ("static inline bool dbc_encode_and_send_" + name + "(" + name + "_t *from)\n")
        code += "{\n"
        code += ("    uint8_t bytes[8];\n")
        code += ("    const dbc_msg_hdr_t hdr = dbc_encode_" + name + "(bytes, from);\n")
        code += ("    return dbc_app_send_can_msg(hdr.mid, hdr.dlc, bytes);\n")
        code += "}\n"
        code += "\n"
        return code

    def get_encode_code(self):
        code = ''
        if self.contains_muxed_signals():
            muxes = self.get_muxes()
            for mux in muxes:
                if "M" == mux:
                    continue

                name = self.get_struct_name()
                name_with_mux = name[:-2] + "_" + str(mux)
                code += ("\n/// Encode " + self.sender + "'s '" + self.name + "' MUX(" + str(mux) + ") message\n")
                code += ("/// @returns the message header of this message\n")
                code += ("static inline dbc_msg_hdr_t dbc_encode_" + name_with_mux)
                code += ("(uint8_t bytes[8], " + name_with_mux + "_t *from)\n")
                code += ("{\n")
                code += ("    uint32_t raw;\n")
                code += ("    bytes[0]=bytes[1]=bytes[2]=bytes[3]=bytes[4]=bytes[5]=bytes[6]=bytes[7]=0;\n\n")
                code += ("    // Set the MUX index value\n")
                muxed_idx = self.get_mux_index_signal()
                code += muxed_idx.get_encode_code("raw", str(mux)[1:])
                code += ("\n")

                # Non Muxed signals in this struct, exclude the MUXED index
                code += "    // Set non MUX'd signals that need to go out with this MUX'd message\n"
                for key in self.signals:
                    if not self.signals[key].is_muxed() and not self.signals[key].mux == "M":
                        code += self.signals[key].get_encode_code("raw", "from->" + key)

                # Rest of the signals that are part of this MUX
                code += ("\n")
                code += ("    // Set the rest of the signals within this MUX (" + mux + ")\n")
                for key in self.signals:
                    if mux == self.signals[key].mux:
                        code += self.signals[key].get_encode_code("raw", "from->" + key)

                code += ("\n")
                code += ("    return " + name[:-2] + "_HDR;\n")
                code += ("}\n")

                # Encode and send function
                code += self.get_encode_and_send(name_with_mux)

        else:
            name = self.get_struct_name()
            code += ("\n/// Encode " + self.sender + "'s '" + self.name + "' message\n")
            code += ("/// @returns the message header of this message\n")
            code += ("static inline dbc_msg_hdr_t dbc_encode_" + name[:-2] + "(uint8_t bytes[8], " + name + " *from)\n")
            code += ("{\n")
            code += ("    uint32_t raw;\n")
            code += ("    bytes[0]=bytes[1]=bytes[2]=bytes[3]=bytes[4]=bytes[5]=bytes[6]=bytes[7]=0;\n")
            code += ("\n")

            for key in self.signals:
                code += self.signals[key].get_encode_code("raw", "from->" + key) + "\n"

            code += ("    return " + self.get_struct_name()[:-2] + "_HDR;\n")
            code += ("}\n")

            # Encode and send function
            code += self.get_encode_and_send(name[:-2])

        return code

    def get_non_mux_signal_decode_code(self, raw_sig_name, prefix=''):
        code = ''
        for key in self.signals:
            if not self.signals[key].is_muxed():
                code += self.signals[key].get_decode_code(raw_sig_name, prefix)
        return code

    def get_signal_decode_code_for_mux(self, mux, raw_sig_name, prefix=''):
        code = ''
        for key in self.signals:
            if self.signals[key].mux == mux:
                code += self.signals[key].get_decode_code(raw_sig_name, prefix)
        return code

    def get_decode_code(self):
        raw_sig_name = "raw"
        code = ''
        code += ("\n/// Decode " + self.sender + "'s '" + self.name + "' message\n")
        code += (
        "/// @param hdr  The header of the message to validate its DLC and MID; this can be NULL to skip this check\n")
        code += ("static inline bool dbc_decode_" + self.get_struct_name()[
                                                    :-2] + "(" + self.get_struct_name() + " *to, const uint8_t bytes[8], const dbc_msg_hdr_t *hdr)\n")
        code += ("{\n")
        code += ("    const bool success = true;\n")
        code += ("    // If msg header is provided, check if the DLC and the MID match\n")
        code += ("    if (NULL != hdr && (hdr->dlc != " + self.get_struct_name()[:-2] + "_HDR.dlc || hdr->mid != " + self.get_struct_name()[:-2] + "_HDR.mid)) {\n")
        code += ("        return !success;\n")
        code += ("    }\n\n")
        code += ("    uint32_t " + raw_sig_name + ";\n")

        if self.contains_muxed_signals():
            # Decode the Mux and store it into it own variable type
            muxed_sig = self.get_mux_index_signal()
            code += ("    // Decode the MUX\n")
            code += (muxed_sig.get_decode_code(raw_sig_name).replace(LINE_BEG, "    ")).replace(muxed_sig.name,
                                                                                                "    const " + muxed_sig.get_code_var_type() + " MUX")
            code += ("\n")

            # Decode the Mux'd signal(s)
            muxes = self.get_muxes()
            for mux in muxes[1:]:
                prefix = "%to->" + mux + "."

                # Each MUX'd message may also have non muxed signals:
                non_mux_code = self.get_non_mux_signal_decode_code(raw_sig_name, prefix)
                mux_code = self.get_signal_decode_code_for_mux(mux, raw_sig_name, prefix)

                if mux == muxes[1]:
                    code += ("    if (" + str(mux)[1:] + " == MUX) {\n")
                else:
                    code += ("    else if (" + str(mux)[1:] + " == MUX) {\n")

                if non_mux_code != '':
                    code += "        // Non Muxed signals (part of all MUX'd structures)\n"
                    code += non_mux_code.replace(LINE_BEG, "        ")
                    code += "\n"
                code += mux_code.replace(LINE_BEG, "        ")

                code += ("\n        to->" + str(mux) + ".mia_info.mia_counter_ms = 0; ///< Reset the MIA counter\n")
                code += ("    }\n")
            code += "    else {\n        return !success;\n    }\n"
        else:
            code += self.get_non_mux_signal_decode_code(raw_sig_name, "    to->").replace(LINE_BEG, "    ")
            code += ("\n")
            code += ("    to->mia_info.mia_counter_ms = 0; ///< Reset the MIA counter\n")

        code += ("\n    return success;\n")
        code += ("}\n")
        return code


class DBC(object):
    def __init__(self, name, self_node, gen_all):
        self.name = name
        self.self_node = self_node
        self.gen_all = gen_all

        # Dictionary of messages with the MSG-ID as the key
        self.messages = OrderedDict()
        self.nodes = []

    def gen_file_header(self):
        code = ''
        code += ("/// DBC file: %s    Self node: '%s'  (ALL = %u)\n" % (self.name, self.self_node, self.gen_all))
        code += ("/// This file can be included by a source file, for example: #include \"generated.h\"\n")
        code += ("#ifndef __GENEARTED_DBC_PARSER\n")
        code += ("#define __GENERATED_DBC_PARSER\n")
        code += ("#include <stdbool.h>\n")
        code += ("#include <stdint.h>\n")
        code += ("#include <stdlib.h>\n")
        return code

    def gen_msg_hdr_struct(self):
        code = ("/// CAN message header structure\n")
        code += ("typedef struct { \n")
        code += ("    uint32_t mid; ///< Message ID of the message\n")
        code += ("    uint8_t  dlc; ///< Data length of the message\n")
        code += ("} dbc_msg_hdr_t; \n")
        return code

    def gen_enum_types(self):
        code = ''
        for mkey in self.messages:
            m = self.messages[mkey]
            if not m.contains_enums():
                continue
            if self.gen_all or m.is_recipient_of_at_least_one_sig(self.self_node) or self.self_node == m.sender:
                code += ("/// Enumeration(s) for Message: '" + m.name + "' from '" + m.sender + "'\n")
                for key in m.signals:
                    if m.signals[key].is_enum_type():
                        code += "typedef enum {\n"
                        for enum_key in m.signals[key].enum_info:
                            code += "    " + enum_key + " = " + m.signals[key].enum_info[enum_key] + ",\n"
                        code += "} " + m.signals[key].name + "_E ;\n\n"
        code += "\n"
        return code

    def gen_msg_hdr_instances(self):
        code = ''
        for mkey in self.messages:
            m = self.messages[mkey]
            if not self.gen_all and not m.is_recipient_of_at_least_one_sig(
                    self.self_node) and self.self_node != m.sender:
                code += "// "
            code += ("static const dbc_msg_hdr_t " + (m.get_struct_name()[:-2] + "_HDR = ").ljust(32 + 7))
            code += ("{ " + str(m.mid).rjust(4) + ", " + m.dlc + " };\n")
        return code

    def gen_mia_struct(self):
        code = ("/// Missing in Action structure\n")
        code += ("typedef struct {\n")
        code += ("    uint32_t is_mia : 1;          ///< Missing in action flag\n")
        code += ("    uint32_t mia_counter_ms : 31; ///< Missing in action counter\n")
        code += ("} dbc_mia_info_t;\n")
        return code

    def _gen_mia_func_header(self, sender, msg_name):
        code = ''
        code += ("\n/// Handle the MIA for " + sender + "'s " + msg_name + " message\n")
        code += ("/// @param   time_incr_ms  The time to increment the MIA counter with\n")
        code += ("/// @returns true if the MIA just occurred\n")
        code += ("/// @post    If the MIA counter reaches the MIA threshold, MIA struct will be copied to *msg\n")
        return code

    def _get_mia_func_body(self, msg_name):
        code = ''
        code += ("{\n")
        code += ("    bool mia_occurred = false;\n")
        code += ("    const dbc_mia_info_t old_mia = msg->mia_info;\n")
        code += ("    msg->mia_info.is_mia = (msg->mia_info.mia_counter_ms >= " + msg_name + "__MIA_MS);\n")
        code += ("\n")
        code += ("    if (!msg->mia_info.is_mia) { // Not MIA yet, so keep incrementing the MIA counter\n")
        code += ("        msg->mia_info.mia_counter_ms += time_incr_ms;\n")
        code += ("    }\n")
        code += ("    else if(!old_mia.is_mia)   { // Previously not MIA, but it is MIA now\n")
        code += ("        // Copy MIA struct, then re-write the MIA counter and is_mia that is overwriten\n")
        code += ("        *msg = " + msg_name + "__MIA_MSG;\n")
        code += ("        msg->mia_info.mia_counter_ms = " + msg_name + "__MIA_MS;\n")
        code += ("        msg->mia_info.is_mia = true;\n")
        code += ("        mia_occurred = true;\n")
        code += ("    }\n")
        code += ("\n    return mia_occurred;\n")
        code += ("}\n")
        return code

    def gen_mia_funcs(self):
        code = ''

        # Generate MIA handler for the dbc.messages we are a recipient of
        for mkey in self.messages:
            m = self.messages[mkey]
            if not self.gen_all and not m.is_recipient_of_at_least_one_sig(self.self_node):
                continue
            if m.contains_muxed_signals():
                muxes = m.get_muxes()
                for mux in muxes[1:]:
                    code += self._gen_mia_func_header(m.sender, m.name + " for MUX \"" + mux + '"')
                    code += ("static inline bool dbc_handle_mia_" + m.get_struct_name()[:-2] + "_" + mux + "(")
                    code += (m.get_struct_name()[:-2] + "_" + mux + "_t *msg, uint32_t time_incr_ms)\n")
                    code += self._get_mia_func_body(m.name + "_" + mux)
            else:
                code += self._gen_mia_func_header(m.sender, m.name)
                code += ("static inline bool dbc_handle_mia_" + m.get_struct_name()[
                                                               :-2] + "(" + m.get_struct_name() + " *msg, uint32_t time_incr_ms)\n")
                code += self._get_mia_func_body(m.name)

        return code


def main(argv):
    dbcfile = '243.dbc'  # Default value unless overriden
    self_node = 'DRIVER'  # Default value unless overriden
    gen_all = False
    muxed_signal = False
    mux_bit_width = 0
    msg_ids_used = []
    try:
        opts, args = getopt.getopt(argv, "i:s:a", ["ifile=", "self=", "all"])
    except getopt.GetoptError:
        print('dbc_parse.py -i <dbcfile> -s <self_node> <-a>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('dbc_parse.py -i <dbcfile> -s <self_node> <-a> <-b>')
            sys.exit()
        elif opt in ("-i", "--ifile"):
            dbcfile = arg
        elif opt in ("-s", "--self"):
            self_node = arg
        elif opt in ("-a", "--all"):
            gen_all = True

    # Parse the DBC file
    dbc = DBC(dbcfile, self_node, gen_all)
    f = open(dbcfile, "r")
    last_mid = -1
    validFile = True
    while 1:
        line = f.readline()
        if not line:
            break

        # Nodes in the DBC file
        if line.startswith("BU_:"):
            nodes = line.strip("\n").split(' ')
            dbc.nodes = (nodes[1:])
            if self_node not in dbc.nodes:
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error "Self node: ' + self_node + ' not found in _BU nodes in the DBC file"')
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error "Self node: ' + self_node + ' not found in _BU nodes in the DBC file"')

        # Start of a message
        # BO_ 100 DRIVER_HEARTBEAT: 1 DRIVER
        if line.startswith("BO_ "):
            muxed_signal = False
            mux_bit_width = 0
            tokens = line.split(' ')
            msg_id = tokens[1]
            msg_name = tokens[2].strip(":")
            dbc.messages[msg_id] = Message(msg_id, msg_name, tokens[3], tokens[4].strip("\n"))
            msg_length = tokens[3]
            last_mid = msg_id
            fixed_mux_signal = False
            fixed_signal_end = 0
            prev_signal_end = 0
            prev_mux_index = 0

            if (int(msg_id) < 0) or (int(msg_id) > 2047):
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error msg id '+ tokens[1] + ' is out of bounds')
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error msg id '+ tokens[1] + ' is out of bounds for 11-bit msgID')

            if msg_id not in msg_ids_used:
                msg_id = msg_ids_used.append(msg_id)
            else:
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error '+ tokens[1] + ' has already been used')
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error msg id '+ msg_id + ' has already been used')

            if (int(msg_length) > 8) or (int(msg_length) < 0):
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error ' + str(tokens[1]) + ' has an incorrect number of bytes. It must be between 0 and 8 bytes.')
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error msg id ' + str(tokens[1]) + ' has an incorrect number of bytes. It must be between 0 and 8 bytes.')

        # Signals: SG_ IO_DEBUG_test_signed : 16|8@1+ (1,-128) [0|0] "" DBG
        if line.startswith(" SG_ "):
            t = line[1:].split(' ')

            # If this is a MUX'd symbol
            mux = ''
            if t[3] == ":":
                mux = t[2]
                line = line.replace(mux + " ", '')
                t = line[1:].split(' ')

            # Split the bit start and the bit size
            s = re.split('[|@]', t[3])
            bit_start = s[0]
            bit_size = s[1]

            if mux == 'M':
                muxed_signal = True
                mux_bit_width = int(bit_size)

            if not muxed_signal:
                if (int(bit_start) < prev_signal_end):
                    print('/////////////////////////////// ERROR /////////////////////////////////////')
                    print('#error ' + t[1] + ' start bit overwrites previous signal') 
                    print('/////////////////////////////// ERROR /////////////////////////////////////')
                    print('')
                    raise ValueError('#error ' + t[1] + ' start bit overwrites previous signal')
                prev_signal_end = int(bit_start) + int(bit_size)
            # Ensure a mux index 
            if muxed_signal:
                if mux == '':
                    fixed_mux_signal = True
                    fixed_signal_end = mux_bit_width + int(bit_size)
                elif mux[0] == 'm':
                    fixed_mux_signal = False
                    if int(mux[1:]) != prev_mux_index:
                        prev_signal_end = fixed_signal_end

                if fixed_mux_signal:
                    if int(bit_start) < mux_bit_width:
                        print('/////////////////////////////// ERROR /////////////////////////////////////')
                        print('#error ' + t[1] + ' start bit overwrites mux index') 
                        print('/////////////////////////////// ERROR /////////////////////////////////////')
                        print('')
                        raise ValueError('#error ' + t[1] + ' start bit overwrites mux index')
                else:
                    if mux != 'M':
                        # Do not allow the signal to use the indexing bits
                        if int(bit_start) < fixed_signal_end:
                            print('/////////////////////////////// ERROR /////////////////////////////////////')
                            print('#error ' + t[1] + ' start bit overwrites mux index') 
                            print('/////////////////////////////// ERROR /////////////////////////////////////')
                            print('')
                            raise ValueError('#error ' + t[1] + ' start bit overwrites previous fixed signal')
                        if mux[0] == 'm':
                        # Check for mux index out of bounds
                            if (int(mux[1:]) >= pow(2,mux_bit_width)) or (int(mux[1:]) < 0):
                                print('/////////////////////////////// ERROR /////////////////////////////////////')
                                print('#error ' + t[1] + ' mux index out of bounds.') 
                                print('/////////////////////////////// ERROR /////////////////////////////////////')
                                print('')
                                raise ValueError('#error ' + t[1] + ' mux index out of bounds.')

                            if int(bit_start) < prev_signal_end:
                                print('/////////////////////////////// ERROR /////////////////////////////////////')
                                print('#error ' + t[1] + ' start bit overwrites previous signal') 
                                print('/////////////////////////////// ERROR /////////////////////////////////////')
                                print('')
                                raise ValueError('#error ' + t[1] + ' start bit overwrites previous signal')
                            prev_signal_end = int(bit_start) + int(bit_size)
                        prev_mux_index = int(mux[1:])

            # If we have an invalid message length then invalidate the DBC and print the offending signal
            # Signal bit width is <= 0
            if (int(bit_size) <= 0):
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error ' + t[1] + ' has invalid size. Signal bit width is: ' + str(int(bit_size))) 
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error ' + t[1] + ' has invalid size. Signal bit width is: ' + str(int(bit_size)))
            
            # Signal is too wide for message
            if (int(bit_start) + int(bit_size)) > (int(msg_length) * 8):
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error ' + t[1] + ' too large. Message needs ' + str(int(bit_start) + int(bit_size)) + ' bits.') 
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error ' + t[1] + ' too large. Message needs ' + str(int(bit_start) + int(bit_size)) + ' bits.')

            endian_and_sign = s[2]
            # Split (0.1,1) to two tokens by removing the ( and the )
            s = t[4][1:-1].split(',')
            scale = s[0]
            offset = s[1]

            # Split the [0|0] to min and max
            s = t[5][1:-1].split('|')
            min_val = s[0]
            max_val = s[1]

            signal_min = 0
            signal_max = (float(scale) * pow(2,int(bit_size)))
            if '-' in t[3]:
                signal_min = -(float(scale) * pow(2,int(bit_size))) / 2
                signal_max = (float(scale) * pow(2,int(bit_size)) / 2)
            # If our min / max values are incorrect then clamping will not work correctly. 
            # Invalidate the DBC and print out the offending signal.
            signal_min = signal_min + float(offset)
            signal_max = signal_max + float(offset) - float(scale)

            # Min for signal is too low.
            if (float(min_val) != 0) and (float(min_val) < float(signal_min)):
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error ' + t[1] + ' min value too low. Min value is: ' + str(signal_min))
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error ' + t[1] + ' min value too low. Min value is: ' + str(signal_min))

            # Max for signal is too high
            if (float(max_val) != 0) and (float(max_val)) > (float(signal_max)):
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('#error ' + t[1] + ' max value too high. Max value is: ' + str(signal_max))
                print('/////////////////////////////// ERROR /////////////////////////////////////')
                print('')
                raise ValueError('#error ' + t[1] + ' max value too high. Max value is: ' + str(signal_max))

            recipients = t[7].strip('\n').split(',')

            # Add the signal the last message object
            sig = Signal(t[1], bit_start, bit_size, endian_and_sign, scale, offset, min_val, max_val, recipients, mux, signal_min, signal_max)
            dbc.messages[last_mid].add_signal(sig)

        # Parse the "FieldType" which is the trigger to use enumeration type for certain signals
        if line.startswith('BA_ "FieldType"'):
            t = line[1:].split(' ')  # BA_ "FieldType" SG_ 123 Some_sig "Some_sig";
            sig_mid = t[3]
            sig_name = t[4]

            # Locate the message and the signal whom this "FieldType" type belongs to
            if sig_mid in dbc.messages:
                if sig_name in dbc.messages[sig_mid].signals:
                    dbc.messages[sig_mid].signals[sig_name].has_field_type = True

        # Enumeration types
        # VAL_ 100 DRIVER_HEARTBEAT_cmd 2 "DRIVER_HEARTBEAT_cmd_REBOOT" 1 "DRIVER_HEARTBEAT_cmd_SYNC" ;
        if line.startswith("VAL_ "):
            t = line[1:].split(' ')
            sig_mid = t[1]
            enum_name = t[2]
            pairs = {}
            t = t[3:]
            for i in range(0, int(len(t) / 2)):
                pairs[t[i * 2 + 1].replace('"', '').replace(';\n', '')] = t[i * 2]

            # Locate the message and the signal whom this enumeration type belongs to
            if sig_mid in dbc.messages:
                if enum_name in dbc.messages[sig_mid].signals:
                    if dbc.messages[sig_mid].signals[enum_name].has_field_type:
                        dbc.messages[sig_mid].signals[enum_name].enum_info = pairs
    
    # If there were errors in parsing the DBC file then do not continue with generation.
    if not validFile:
        sys.exit(-1)

    print(dbc.gen_file_header())
    print("\n")

    # Generate the application send extern function
    print("/// Extern function needed for dbc_encode_and_send()")
    print("extern bool dbc_app_send_can_msg(uint32_t mid, uint8_t dlc, uint8_t bytes[8]);")
    print("")

    # Generate header structs and MIA struct
    print(dbc.gen_mia_struct())
    print(dbc.gen_msg_hdr_struct())
    print(dbc.gen_msg_hdr_instances())
    print(dbc.gen_enum_types())

    # Generate converted struct types for each message
    for mid in dbc.messages:
        m = dbc.messages[mid]
        if not gen_all and not m.is_recipient_of_at_least_one_sig(self_node) and m.sender != self_node:
            code = ("\n// Not generating '" + m.get_struct_name() + "' since we are not the sender or a recipient of any of its signals")
        else:
            print(m.gen_converted_struct(self_node, gen_all))

    # Generate MIA handler "externs"
    print("\n/// @{ These 'externs' need to be defined in a source file of your project")
    for mid in dbc.messages:
        m = dbc.messages[mid]
        if gen_all or m.is_recipient_of_at_least_one_sig(self_node):
            if m.contains_muxed_signals():
                muxes = m.get_muxes()
                for mux in muxes[1:]:
                    print(str("extern const uint32_t ").ljust(50) + (m.name + "_" + mux + "__MIA_MS;"))
                    print(str("extern const " + m.get_struct_name()[:-2] + "_" + mux + "_t").ljust(49) + " " + (
                    m.name + "_" + mux + "__MIA_MSG;"))
            else:
                print(str("extern const uint32_t ").ljust(50) + (m.name + "__MIA_MS;"))
                print(str("extern const " + m.get_struct_name()).ljust(49) + " " + (m.name + "__MIA_MSG;"))
    print("/// @}\n")

    # Generate encode methods
    for mid in dbc.messages:
        m = dbc.messages[mid]
        if not gen_all and m.sender != self_node:
            print ("\n/// Not generating code for dbc_encode_" + m.get_struct_name()[:-2] + "() since the sender is " + m.sender + " and we are " + self_node)
        else:
            print(m.get_encode_code())

    # Generate decode methods
    for mid in dbc.messages:
        m = dbc.messages[mid]
        if not gen_all and not m.is_recipient_of_at_least_one_sig(self_node):
            print ("\n/// Not generating code for dbc_decode_" + m.get_struct_name()[:-2] + "() since '" + self_node + "' is not the recipient of any of the signals")
        else:
            print(m.get_decode_code())

    print(dbc.gen_mia_funcs())
    print("#endif")


if __name__ == "__main__":
    main(sys.argv[1:])

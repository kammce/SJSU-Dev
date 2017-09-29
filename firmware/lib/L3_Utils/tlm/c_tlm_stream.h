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

#ifndef TLM_STREAM_H__
#define TLM_STREAM_H__
#include "c_tlm_comp.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif



/**
 * @file
 * @brief Telemetry stream and decode.
 *
 * Telemetry stream sends encoded ASCII data stream to the provided stream function.
 * Example stream :
 * @{
 *      START:my_component:3
 *      a:4:1:1:DE,AD,BE,EF
 *      b:2:1:1:02,00
 *      c:1:1:1:11
 *      END:my_component
 * @}
 *
 * Each stream starts with the tag START and ends with END
 * After the colon is the name of the component and then the number of variables
 * registered in this component.  Then the variables' data follows :
 *  - The name of the variable
 *  - Number of bytes per variable
 *  - Number of elements per array (1 for a single variable)
 *  - Variable type.  @see tlm_type at c_tlm_var.h
 *  - HEX bytes
 *
 * Telemetry decode will do the opposite.  It will decode an incoming stream, and
 * based on the stream, it will find the component and the registered variable and
 * set the value of the variable based on the stream.
 * This functionality can be used to take a saved data stream, and set the variables'
 * data value based on the stream.  In particular, the registered variables' values
 * can be restored based on a previously saved telemetry stream from a disk etc.
 *
 * Here is an example of saving the stream into a C++ string :
 * @code
 *      void string_stream(const char* s, void *arg) {
 *          std::string *str = (std::string*)arg;
 *          (*str) += s;
 *      }
 *
 *      std::string str = "";
 *      tlm_stream_one(my_comp, string_stream, &str);
 * @endcode
 */


/**
 * Typedef of the stream callback function
 * @param str  The ASCII string containing partial stream
 */
typedef void (*stream_callback_type)(const char* str, void *arg);

/**
 * Streams the telemetry for one component and its variables
 * @param stream        The callback stream function that will receive the strings to print
 * @param print_ascii   If non-null, the data will be printed as ASCII values rather than hex values
 * @param arg           This argument will be passed to your stream function as its argument
 */
void tlm_stream_one(tlm_component *comp, stream_callback_type stream, void *print_ascii, void *arg);

/**
 * Streams the telemetry for ALL registered components and their variables
 * @param stream_func The callback stream function that will receive the strings to print
 * @param arg    This argument will be passed to your stream function as its argument
 * @param ascii  If true, ASCII values are printed, rather than hex value of the data
 */
void tlm_stream_all(stream_callback_type stream_func, void *arg, bool ascii);

/**
 * Streams your provided component telemetry into a file pointer
 * @param comp_ptr  The telemetry component pointer
 * @param file  Could be stdout, stderr, or an opened file handle
 */
void tlm_stream_one_file(tlm_component *comp_ptr, FILE *file);

/**
 * Streams all components to a file
 * @param file  Could be stdout, stderr, or an opened file handle
 * Example: To printf the telemetry, just use "tlm_stream_all_file(stdio);"
 */
void tlm_stream_all_file(FILE *file);

/**
 * This is similar to tlm_stream_decode(char*) except that it decodes stream
 * from an opened file handle.  The file will be read until fgets() fails.
 * @returns true when telemetry decode finds correct stream header.
 */
bool tlm_stream_decode_file(FILE *file);



#ifdef __cplusplus
}
#endif
#endif /* TLM_STREAM_H__ */

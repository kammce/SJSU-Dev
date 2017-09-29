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



#include "c_tlm_stream.h"
#include "c_tlm_var.h"
#include <string.h>     /* strlen() etc. */
#include <stdlib.h>     /* atoi() */
#include <ctype.h>      /* tolower() isdigit() etc. */
#include <inttypes.h>



static uint8_t hex_to_byte(const char *two_digit_hex)
{
    uint8_t d1 = *(two_digit_hex + 0);
    uint8_t d2 = *(two_digit_hex + 1);
    uint8_t value = 0;

    if (isdigit(d1)) value += d1 - '0';
    else value += tolower(d1) - 'a' + 10;

    value <<= 4;

    if (isdigit(d2)) value += d2 - '0';
    else value += tolower(d2) - 'a' + 10;

    return value;
}

static void tlm_stream_file_ptr(const char *str, void *fptr)
{
    fwrite(str, sizeof(char), strlen(str), (FILE*)fptr);
}

/**
 * Callback function for each component's variables
 */
static bool tlm_stream_for_each_component_var(void *elm_ptr, void *arg1, void *arg2, void *print_ascii)
{
    char buff[256];
    tlm_reg_var_type *var = elm_ptr;
    stream_callback_type stream = arg1;
    void *stream_arg = arg2;
    char *p = (char*)(var->data_ptr);
    uint32_t i = 0;

    if (NULL == stream || NULL == var) {
        return false;
    }

    stream((var->name), stream_arg);
    stream(":", stream_arg);
    sprintf(buff, "%" PRIi32 ":", var->elm_size_bytes);
    stream(buff, stream_arg);
    sprintf(buff, "%" PRIi32 ":", var->elm_arr_size);
    stream(buff, stream_arg);
    sprintf(buff, "%i:", var->elm_type);
    stream(buff, stream_arg);

    if (print_ascii)
    {
        tlm_variable_print_value(var, buff, sizeof(buff));
        stream(buff, stream_arg);
    }
    else
    {
        /* A variable has at least one data byte */
        sprintf(buff, "%02X", ((*p++) & 0xFF));
        stream(buff, stream_arg);

        /* Stream rest of the data bytes */
        for(i=(var->elm_size_bytes) * (var->elm_arr_size); /* Total bytes */
            i > 1; i--) { /* note: 1 byte already streamed above */
            sprintf(buff, ",%02X", ((*p++) & 0xFF) );
            stream(buff, stream_arg);
        }
    }

    stream("\n", stream_arg);

    return true;
}

static bool tlm_stream_decode(FILE *file, tlm_component *p_comp)
{
    /* Stream is: <Name>:<Var Size in Bytes>:<Array size>:<Type>:<HEX Bytes>\n */
    uint32_t exp_byte_cnt = 0; /* expected byte count */

    /* Get the header information prior to the hex data */
    char header[128] = { 0 };
    int i = 0, colon_count = 0;
    const int colon_count_of_hex_data = 4;
    for ( ; i < sizeof(header)-1; i++) {
        int c = fgetc(file);
        if (c == EOF) {
            goto decode_error;
        }
        if (c == ':' && (++colon_count) >= colon_count_of_hex_data) {
            break;
        }
        header[i] = c;
    }

    /* Break the header into pieces */
    char *tok_itr = 0;
    char *tok_name     = strtok_r(header, ":", &tok_itr);
    char *tok_var_size = strtok_r(NULL, ":", &tok_itr);
    char *tok_arr_size = strtok_r(NULL, ":", &tok_itr);
    if (NULL != tok_name && NULL != tok_var_size && NULL != tok_arr_size) {
        exp_byte_cnt = atoi(tok_var_size) * atoi(tok_arr_size);
    }
    else {
        goto decode_error;
    }

    /* Decode hex bytes after the header */
    const tlm_reg_var_type *reg_var = tlm_variable_get_by_name(p_comp, tok_name);
    if (NULL != reg_var &&
        (reg_var->elm_size_bytes) * (reg_var->elm_arr_size) == exp_byte_cnt)
    {
        char *dst = (char*)(reg_var->data_ptr);
        int terminator = EOF;

        for ( ; exp_byte_cnt != 0; --exp_byte_cnt) {
            char byte1 = fgetc(file);
            char byte2 = fgetc(file);
            terminator = fgetc(file); /* could be comma, \n or EOF */
            if (EOF == terminator) {
                goto decode_error;
            }

            char hex_bytes[] = { byte1, byte2, 0};
            char byte = hex_to_byte(hex_bytes);
            memcpy(dst, &byte, sizeof(byte));
            dst += sizeof(byte);
        }

        /* After we decode all hex bytes, the last char should be the
         * newline character because we still should have the last line
         * of telemetry stream containing "END:<comp name>"
         */
        if ('\n' != terminator) {
            goto decode_error;
        }
    } else {
        /* Handle unregistered variable */
    }

    return true;
    decode_error :
        return false;
}

/**
 * Callback function for each component
 * @param sca  Stream callback argument
 */
void tlm_stream_one(tlm_component *comp, stream_callback_type stream, void *print_ascii, void *sca)
{
    if (NULL == comp || NULL == stream) {
        return;
    }

    /* sca : stream callback argument */
    char buff[16] = { 0 };
    sprintf(buff, "%u\n", (unsigned int)c_list_node_count((comp->var_list)));

    /* Send: "START:<name>:<#>\n" */
    stream("START:", sca);
    stream(comp->name, sca);
    stream(":", sca);
    stream(buff, sca);

    /* Now for each variable list of this component, make a call-back to our
     * component for each function that will stream data of each variable
     */
    c_list_for_each_elm((comp->var_list), tlm_stream_for_each_component_var,
                        stream,     /* arg1 */
                        sca,        /* arg2 */
                        print_ascii /* arg3 */
                        );

    /* Send: "END:<name>\n" */
    stream("END:", sca);
    stream((comp->name), sca);
    stream("\n", sca);
}

static void tlm_stream_all_args(tlm_component *comp_ptr, void *arg1, void *arg2)
{
    /* Recall the arguments set by tlm_stream_all() */
    void **args = (void**) arg1;
    stream_callback_type stream_func = args[0];
    void *user_arg = args[1];
    void *print_ascii_arg = args[2];

    /* Now pass the arguments to tlm_stream_one() */
    tlm_stream_one(comp_ptr, stream_func, print_ascii_arg, user_arg);
}

void tlm_stream_all(stream_callback_type stream_func, void *arg, bool ascii)
{
    /* We need to pass 3 args, but only have 2 possible arguments that we can pass to
     * tlm_component_for_each() API, so use a double pointer here.
     */
    void *print_ascii_arg = ascii ? (void*) 1 : (void*) NULL;
    void *args[] = { (void*) stream_func, arg, print_ascii_arg };

    /*
     * Simply tell the telemetry component to call our call-back with our
     * stream function as the argument.  We will get the callback function
     * called with pointer to each telemetry component, along with our
     * argument which is the stream function itself
     */
    tlm_component_for_each((tlm_comp_callback)tlm_stream_all_args, args, NULL);
}

void tlm_stream_one_file(tlm_component *comp_ptr, FILE *file)
{
    void * print_ascii = NULL; /* Do not print ASCII (print hex instead) */
    if(file) {
        tlm_stream_one(comp_ptr, tlm_stream_file_ptr, print_ascii, file);
    }
}

/* We need this "wrapper" function because tlm_component_for_each() callback is
 * called with just two arguments, and we cannot just pass tlm_stream_one() function
 * as callback because it takes three arguments.
 */
static void tlm_stream_all_file_args(tlm_component *comp_ptr, void *arg1, void *arg2)
{
    stream_callback_type sc = arg1;
    void *file = arg2;
    void *print_ascii = NULL;

    tlm_stream_one(comp_ptr, sc, print_ascii, file);
}

void tlm_stream_all_file(FILE *file)
{
    if(file) {
        tlm_component_for_each((tlm_comp_callback)tlm_stream_all_file_args,
                               (void*)tlm_stream_file_ptr, /* arg1 at tlm_stream_all_file_args() */
                               file                        /* arg2 at tlm_stream_all_file_args() */
                               );
    }
}

bool tlm_stream_decode_file(FILE *file)
{
    uint32_t num_vars_in_stream = 0;
    tlm_component *component = NULL;
    bool success = false;

    /* Telemetry begins with: "START:<name>:<#>\n"
     * A file can contain telemetry of multiple components so it may have
     * START ... END
     * START ... END
     * We will continue to decode till end of file.
     */
    char line[128] = { 0 };
    while (fgets(line, sizeof(line)-1, file)) {
        if (line == strstr(line, "START:")) {
            char *name = strstr(line, ":") + 1;
            char *count = strstr(name, ":");
            if (NULL != count) {
                *count++ = '\0';
                component = tlm_component_get_by_name(name);
                if (NULL != component) {
                    num_vars_in_stream = atoi(count);
                    success = true;
                }
            }
        }

        /* Number of variables in stream will be greater than zero only
         * when we have correctly decoded STARTing header.  This loop
         * will decode data until end of this components' stream.
         */
        while (num_vars_in_stream > 0) {
            if(tlm_stream_decode(file, component)) {
                --num_vars_in_stream;
            }
            else {
                success = false;
                return success;
            }
        }
    } // fgets()

    /* success only changed to true if we got atleast one "START" in the file */
    return success;
}

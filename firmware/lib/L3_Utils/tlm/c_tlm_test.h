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

#ifndef C_TLM_TEST_H_
#define C_TLM_TEST_H_
#ifdef __cplusplus
extern "C" {
#endif
#if TESTING

#include "c_tlm_stream.h"
#include "c_tlm_comp.h"
#include "c_tlm_var.h"
#include <assert.h>
#include <string.h>

static void string_stream(const char* s, void *arg) {
    strcat((char*)arg, s);
}

static bool test_tlm(void)
{
    long long int a = 0xAA00DEADBEEF0055;
    int b = 1;
    short c = 2;
    char d = 3;
    float f = 0;

    puts("Test: Add 1 component");
    tlm_component *first_comp = NULL;
    assert(0 != (first_comp = tlm_component_add("first")));

    char str_stream[128] = { 0 };
    tlm_stream_one(first_comp, string_stream, str_stream);
    assert(0 == strcmp(str_stream, "START:first:0\nEND:first\n"));

    puts("Test: Add 2 components");
    tlm_component* second_comp = tlm_component_add("second");
    assert(0 != second_comp);
    assert(0 == tlm_component_add("second"));

    assert(0 == tlm_component_get_by_name("null"));
    assert(second_comp == tlm_component_get_by_name("second"));


    puts("Test: Add 3rd component with large data");
    tlm_component* third_comp = tlm_component_add("third");
    assert(0 != third_comp);
    assert(0 == tlm_component_add("third"));
    char large_array1[512] = { 0 };
    char large_array2[512] = { 0 };
    char large_array_org[512] = { 0 };
    for (int i = 0; i < sizeof(large_array1); i++) {
        large_array1[i] = i;
        large_array2[i] = i;
        large_array_org[i] = i;
    }
    assert(tlm_variable_register(third_comp, "large_array1", large_array1, sizeof(large_array1), 1, tlm_char));
    assert(tlm_variable_register(third_comp, "large_array2", large_array2, 1, sizeof(large_array2), tlm_char));


    puts("Test: Add variable to component");
    assert(tlm_variable_register(second_comp, "a", &a, sizeof(a), 1, tlm_int));
    assert(!tlm_variable_register(second_comp, "a", &a, sizeof(a), 1, tlm_int));
    assert(!tlm_variable_register(second_comp, "a", (void*)1 , sizeof(a), 1, tlm_int));
    assert(!tlm_variable_register(second_comp, "b", &a, sizeof(a), 1, tlm_int));

    puts("Test: Register variables to component");
    assert(TLM_REG_VAR(second_comp, b, tlm_int));
    assert(TLM_REG_VAR(second_comp, c, tlm_int));
    assert(TLM_REG_VAR(second_comp, d, tlm_int));
    assert(TLM_REG_VAR(second_comp, f, tlm_float));

    puts("Test: Get pointer by name");
    assert(&b == tlm_variable_get_by_name(second_comp, "b")->data_ptr);
    assert(NULL == tlm_variable_get_by_name(second_comp, "#"));

    puts("Test: TLM Stream decode");
    b = 0;
    d = 0;
    char stream_start[100] = "START:second:2\n";
    char stream_var_a[100] = "b:4:1:0:01,01,00,00";
    char stream_var_d[100] = "d:1:1:0:11";
    assert(tlm_stream_decode(stream_start));
    assert(!tlm_stream_decode(stream_var_a));
    assert(!tlm_stream_decode(stream_var_d));
    assert(257 == b);
    assert(17 == d);

    puts("Test: TLM Stream to stdio");
    tlm_stream_all_file(stdout);

    /* Stream to file, then change values of a and b, and decode
     * back to the stream and verify old values of a and b.
     */
    b = 0x12345;
    d = 0x15;
    puts("Test: TLM Stream to file");
    FILE *file = fopen("test.txt", "w");
    assert(file);
    tlm_stream_all_file(file);
    fclose(file);

    b = 0;
    d = 0;
    memset(large_array1, 0, sizeof(large_array1));
    memset(large_array2, 0, sizeof(large_array2));
    file = fopen("test.txt", "r");
    assert(file);
    assert(tlm_stream_decode_file(file));
    assert(0x12345 == b);
    assert(0x15 == d);
    assert(0 == memcmp(large_array_org, large_array1, sizeof(large_array_org)));
    assert(0 == memcmp(large_array_org, large_array2, sizeof(large_array_org)));
    fclose(file);


    char s1[42] = "0123456789012345678901234567890123456789";
    char s2[42] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMN";
    tlm_component* disk = tlm_component_add("disk");
    TLM_REG_VAR(disk, s1, tlm_string);
    TLM_REG_VAR(disk, s2, tlm_string);

    file = fopen("test.txt", "w");
    assert(file);
    tlm_stream_one_file(disk, file);
    fclose(file);

    memset(s1, 0, sizeof(s1));
    memset(s2, 0, sizeof(s2));

    file = fopen("test.txt", "r");
    assert(file);
    assert(tlm_stream_decode_file(file));
    fclose(file);

    printf("s1 = |%s|\n", s1);
    printf("s2 = |%s|\n", s2);

    assert(0 == strcmp(s1, "0123456789012345678901234567890123456789"));
    assert(0 == strcmp(s2, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMN"));

    return true;
}


#ifdef __cplusplus
}
#endif
#endif /* TESTING */
#endif /* C_TLM_TEST_H_ */

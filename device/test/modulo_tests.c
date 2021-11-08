// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file modulo_tests.c
*/

#include <stdbool.h>

#include "defines.h"
#include "inttypes.h"  // PRIu64
#include "modulo.h"
#include "modulus.h"
#include "test_common.h"
#include "util_print.h"

/**
Helper function to test barret wide reduction. Note that this tests the '%' operator too, which may
not always be desirable.

@param input
@param modulus
@param res_exp
 */
void test_barrett_reduce_wide_helper(ZZ *input, const Modulus *modulus, ZZ res_exp)
{
    ZZ q = modulus->value;
    printf("-------------------------------------\n");
    ZZ res = barrett_reduce_wide(input, modulus);

    printf("{ %" PRIuZZ " , %" PRIuZZ "} mod %" PRIuZZ "\n", input[1], input[0], q);

    uint64_t actual_input = (uint64_t)(((uint64_t)input[1] << 32) | ((input[0]) & 0xFFFFFFFF));
    printf("--> input = %" PRIu64 "\n", actual_input);

    // -- If you don't want to test '%', must comment this out
    ZZ res_default = (ZZ)(actual_input % q);
    print_zz("Result default", res_default);
    se_assert(res == res_default);

    print_zz("Result expected", res_exp);
    print_zz("Result barrett", res);
    se_assert(res == res_exp);
}

/**
Helper function to test barret reduction. Note that this tests the '%' operator too, which may not
always be desirable.

@param input
@param modulus
@param res_exp
 */
void test_barrett_reduce_helper(ZZ input, const Modulus *modulus, ZZ res_exp)
{
    ZZ q = modulus->value;
    printf("-------------------------------------\n");
    ZZ res = barrett_reduce(input, modulus);

    printf("%" PRIuZZ " mod %" PRIuZZ "\n", input, q);

    // -- If you don't want to test '%', comment this out
    ZZ res_default = input % q;
    print_zz("Result default", res_default);
    se_assert(res == res_default);

    print_zz("Result expected", res_exp);
    print_zz("Result barrett", res);
    se_assert(res == res_exp);
}

void test_barrett_reduce(void)
{
    printf("\n**************************************\n");
    printf("Beginning tests for barrett_reduce...\n\n");
    Modulus modulus;  // value = q, const_ratio = floor(2^64, q)

    set_modulus(134012929, &modulus);  // 27 bits

    test_barrett_reduce_helper(0, &modulus, 0);
    test_barrett_reduce_helper(1, &modulus, 1);
    test_barrett_reduce_helper(134012928, &modulus, 134012928);
    test_barrett_reduce_helper(134012929, &modulus, 0);
    test_barrett_reduce_helper(134012930, &modulus, 1);
    test_barrett_reduce_helper((ZZ)(134012929) << 1, &modulus, 0);  // 28 bits
    test_barrett_reduce_helper((ZZ)(134012929) << 2, &modulus, 0);  // 29 bits
    test_barrett_reduce_helper(0x36934613, &modulus, 111543821);    // random
    test_barrett_reduce_helper(MAX_ZZ, &modulus, 6553567);

    set_modulus(1053818881, &modulus);  // 30 bits

    test_barrett_reduce_helper(0, &modulus, 0);
    test_barrett_reduce_helper(1, &modulus, 1);
    test_barrett_reduce_helper(1053818880, &modulus, 1053818880);
    test_barrett_reduce_helper(1053818881, &modulus, 0);
    test_barrett_reduce_helper(1053818882, &modulus, 1);
    test_barrett_reduce_helper((ZZ)(1053818881) << 1, &modulus, 0);  // 31 bits
    test_barrett_reduce_helper((ZZ)(1053818881) << 2, &modulus, 0);  // 32 bits
    test_barrett_reduce_helper(0x36934613, &modulus, 915621395);     // random
    test_barrett_reduce_helper(MAX_ZZ, &modulus, 79691771);

    printf("\n... all tests for barrett_reduce passed.\n");
    printf("**************************************\n");
}

void test_barrett_reduce_wide(void)
{
    printf("\n***************************************\n");
    printf("Beginning tests for barrett_reduce_wide...\n\n");

    ZZ input[2];
    Modulus modulus;  // value = q, const_ratio = floor(2^64, q)

    set_modulus(134012929, &modulus);  // 27 bits

    input[1] = 0;
    input[0] = 0;
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = 1;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 0;
    input[0] = 134012928;
    test_barrett_reduce_wide_helper(input, &modulus, 134012928);
    input[1] = 0;
    input[1] = 0;
    input[0] = 134012929;
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = 134012930;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 0;
    input[0] = 134012929 << 1;  // 28 bits
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = 134012929 << 2;  // 29 bits
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 6553567);
    input[1] = 0x33345624;  // random
    input[0] = 0x47193658;  // random
    test_barrett_reduce_wide_helper(input, &modulus, 77416961);
    input[1] = MAX_ZZ;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 119980058);

    set_modulus(1053818881, &modulus);  // 30 bits

    input[1] = 0;
    input[0] = 0;
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = 1;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 0;
    input[0] = 1053818880;
    test_barrett_reduce_wide_helper(input, &modulus, 1053818880);
    input[1] = 0;
    input[0] = 1053818881;
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = 1053818882;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 0;
    input[0] = (ZZ)(1053818881) << 1;  // 31 bits
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = (ZZ)(1053818881) << 2;  // 32 bits
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 79691771);
    input[1] = 0x33345624;  // random
    input[0] = 0x47193658;  // random
    test_barrett_reduce_wide_helper(input, &modulus, 569939669);
    input[1] = MAX_ZZ;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 159648581);
}

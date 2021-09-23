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
Helper function to test barret wide reduction. Note that this tests the '%' operator
too, which may not always be desirable.

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
Helper function to test barret reduction. Note that this tests the '%' operator too,
which may not always be desirable.

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

    // If you don't want to test '%', must comment this out
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
    ZZ input;
    Modulus modulus;

    // modulus.value        = q
    // modulus.const_ratio  = floor(2^128/q)

    // ------------------------------
    // Set 1: q = 2 (min value of q)
    // ------------------------------
    set_modulus(2, &modulus);

    test_barrett_reduce_helper(1, &modulus, 1);           //        1 = 1 (mod 2)
    test_barrett_reduce_helper(2, &modulus, 0);           //        2 = 0 (mod 2)
    test_barrett_reduce_helper(3, &modulus, 1);           //        3 = 1 (mod 2)
    test_barrett_reduce_helper(MAX_ZZ, &modulus, 1);      // max odd  = 1 (mod 2)
    test_barrett_reduce_helper(MAX_ZZ - 1, &modulus, 0);  // max even = 0 (mod 2)

    input = 0xda9b246b;

    test_barrett_reduce_helper(input, &modulus, 1);  // random odd  = 1 (mod 2)
    input--;
    test_barrett_reduce_helper(input, &modulus, 0);  // random even = 0 (mod 2)

    // ------------------
    // Set 2: q = max q
    // ------------------
    set_modulus(MAX_Q, &modulus);
    test_barrett_reduce_helper(1, &modulus, 1);  // 1 % MAXQ = 1
    test_barrett_reduce_helper(2, &modulus, 2);  // 2 % MAXQ = 2

    // -- Since MAX_ZZ = MAX_Q*2 + 1, MAXZZ % MAX_q = 1
    test_barrett_reduce_helper(MAX_ZZ, &modulus, 1);

    // -- random x < q ---> x = x (mod q), 32 bit
    input = 0x76774403;
    test_barrett_reduce_helper(input, &modulus, input);
    input = 0x958b2d91;  // random > q, 32 bit
    test_barrett_reduce_helper(input, &modulus, 361442706);

    // --------------------------------------------
    // Set 3: q = 1559578058 (random 31-bit number)
    // --------------------------------------------
    set_modulus(0x5cf545ca, &modulus);
    test_barrett_reduce_helper(1, &modulus, 1);  // 1 = 1 (mod q)
    test_barrett_reduce_helper(2, &modulus, 2);  // 2 = 2 (mod q)
    test_barrett_reduce_helper(MAX_ZZ, &modulus, 1175811179);

    // random x < q ---> x = x (mod q)
    input = 0x16774403;
    test_barrett_reduce_helper(input, &modulus, input);
    // random > q, 32 bit
    test_barrett_reduce_helper(0x958b2d91, &modulus, 949348295);
    printf("\n... all tests for barrett_reduce passed.\n");
    printf("**************************************\n");
}

void test_barrett_reduce_wide(void)
{
    printf("\n***************************************\n");
    printf("Beginning tests for barrett_reduce_wide...\n\n");

    ZZ input[2];
    Modulus modulus;
    // -- q must also be > 1 ----> where is this enforced??

    // ------------------------------
    // Set 1: q = 2 (min value of q)
    // ------------------------------
    set_modulus(2, &modulus);

    input[1] = 0;  //          1 = 1 (mod 2)
    input[0] = 1;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 0;  //          2 = 0 (mod 2)
    input[0] = 2;
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;  //          3 = 1 (mod 2)
    input[0] = 3;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = MAX_ZZ;  // max (odd)  = 1 (mod 2)
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = MAX_ZZ;  // max (even) = 0 (mod 2)
    input[0] = MAX_ZZ - 1;
    test_barrett_reduce_wide_helper(input, &modulus, 0);

    input[1] = random_zz();
    input[0] = 0xda9b246b;  // random (odd) = 1 (mod 2)
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[0]--;  // random (even) = 0 (mod 2)
    test_barrett_reduce_wide_helper(input, &modulus, 0);

    // -------------
    // Set 2: q = 3
    // -------------
    set_modulus(3, &modulus);

    input[1] = 0;
    input[0] = 0;
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = 0;
    input[0] = 1;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 456;
    input[0] = 123;
    test_barrett_reduce_wide_helper(input, &modulus, 0);
    input[1] = MAX_ZZ;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 0);

    // -------------
    // Set 3: max q
    // -------------
    set_modulus(MAX_Q, &modulus);

    input[1] = 0;  // 1 = 1 (mod q)
    input[0] = 1;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 0;  // 2 = 2 (mod q)
    input[0] = 2;
    test_barrett_reduce_wide_helper(input, &modulus, 2);

    // -- Since MAX_ZZ = MAX_Q*2 + 1, MAXZZ % MAX_q = 1
    input[1] = 0;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 1);

    input[1] = MAX_ZZ;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 3);

    // -- random x < q ---> x = x (mod q), 32 bit
    input[1] = 0;
    input[0] = 0x76774403;
    test_barrett_reduce_wide_helper(input, &modulus, input[0]);

    input[1] = 0x958b2d91;  // random > q, 32 bit
    input[0] = 0x654b2d21;  // random > q, 32 bit
    test_barrett_reduce_wide_helper(input, &modulus, 274827334);

    // --------------------------------------------
    // Set 3: random q, 32 bit (q = 1559578058)
    // --------------------------------------------
    set_modulus(0x5cf545ca, &modulus);

    input[1] = 0;  // 1 = 1 (mod q)
    input[0] = 1;
    test_barrett_reduce_wide_helper(input, &modulus, 1);
    input[1] = 0;  // 2 = 2 (mod q)
    input[0] = 2;
    test_barrett_reduce_wide_helper(input, &modulus, 2);
    input[1] = 0;  // random x < q ---> x = x (mod q)
    input[0] = 0x16774403;
    test_barrett_reduce_wide_helper(input, &modulus, input[0]);

    input[1] = MAX_ZZ;
    input[0] = MAX_ZZ;
    test_barrett_reduce_wide_helper(input, &modulus, 112593495);
    input[1] = 0xd9582d91;  // random > q
    input[0] = 0x7b813c5b;
    test_barrett_reduce_wide_helper(input, &modulus, 25773121);
    printf("\n... all tests for barrett_reduce_wide passed.\n");
    printf("***************************************\n");
}

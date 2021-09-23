// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file uintmodarith_tests.c
*/

#include "defines.h"
#include "modulo.h"
#include "modulus.h"
#include "test_common.h"
#include "uintmodarith.h"
#include "util_print.h"

void test_add_mod_helper(ZZ val1, ZZ val2, Modulus *modulus, ZZ res_exp)
{
    // -- Recall correctness requirement: val1 + val2 < 2q-1
    ZZ q = modulus->value;
    printf("---------------------------------\n");
    for (int i = 0; i < 2; i++)
    {
        printf("( %" PRIuZZ " + %" PRIuZZ " ) %% %" PRIuZZ "\n", val1, val2, q);
        ZZ res = add_mod(val1, val2, modulus);
        print_zz("Result         ", res);
        print_zz("Result expected", res_exp);
        se_assert(res == res_exp);

        ZZ res_default = (val1 + val2) % q;
        print_zz("Result default ", res_default);
        se_assert(res == res_default);

        // -- Swap val1 and val2 and try again
        ZZ temp = val1;
        val1    = val2;
        val2    = temp;
        printf("(After swap)\n");
    }
}

void test_mul_mod_helper(ZZ val1, ZZ val2, Modulus *modulus, ZZ res_exp)
{
    ZZ q = modulus->value;
    printf("---------------------------------\n");
    for (int i = 0; i < 2; i++)
    {
        printf("( %" PRIuZZ " * %" PRIuZZ " ) %% %" PRIuZZ "\n", val1, val2, q);
        ZZ res = mul_mod(val1, val2, modulus);
        print_zz("Result expected", res_exp);
        print_zz("Result         ", res);
        se_assert(res == res_exp);

        ZZ res_default = (val1 * val2) % q;
        print_zz("Result default ", res_default);
        se_assert(res == res_default);

        // -- Swap val1 and val2 and try again
        ZZ temp = val1;
        val1    = val2;
        val2    = temp;
        printf("(After swap)\n");
    }
}

void test_neg_mod_helper(ZZ input, const Modulus *modulus, ZZ res_exp)
{
    ZZ q = modulus->value;
    printf("---------------------------------\n");
    ZZ res = neg_mod(input, modulus);

    printf("( -%" PRIuZZ ") %% %" PRIuZZ "\n", input, q);
    print_zz("Result         ", res);
    print_zz("Result expected", res_exp);
    se_assert(res == res_exp);

    // -- Don't use the remainder operator, it does not work well for modular negation
    // ZZ res_default = (ZZ)((int64_t)(-input) % q);
    // print_zz("Result default  ", res_default);
    // se_assert(res == res_default);
}

void test_add_mod_basic(Modulus *modulus)
{
    // -- Recall correctness requirement: val1 + val2 < 2q-1
    ZZ q = modulus->value;

    test_add_mod_helper(0, 0, modulus, 0);  // 0+0 = 0 (mod q)
    test_add_mod_helper(0, 1, modulus, 1);  // 0+1 = 1 (mod q)
    test_add_mod_helper(0, q, modulus, 0);  // 0+q = 0 (mod q)
    test_add_mod_helper(1, q, modulus, 1);  // 1+q = 1 (mod q)

    test_add_mod_helper(1, q - 1, modulus, 0);      // 1+q-1 = 0 (mod q)
    test_add_mod_helper(q, q - 1, modulus, q - 1);  // q+q-1 = q-1 (mod q)

    test_add_mod_helper(q - 1, q - 1, modulus, q - 2);  // q-1+q-1 = q-2 (mod q)
    test_add_mod_helper(0, 2 * q - 1, modulus, q - 1);  // 0+2q-1 = q-1 (mod q)
}

void test_mul_mod_basic(Modulus *modulus)
{
    ZZ q = modulus->value;
    test_mul_mod_helper(0, 0, modulus, 0);  // 0*0 % q = 0
    test_mul_mod_helper(1, 1, modulus, 1);  // 1*1 % q = 1

    test_mul_mod_helper(1, q, modulus, 0);          // 1*q % q = 0
    test_mul_mod_helper(q + 1, 1, modulus, 1);      // (q + 1)*1 % q = 1
    test_mul_mod_helper(q - 1, 1, modulus, q - 1);  // (q - 1)*1 % q = q - 1
    test_mul_mod_helper(0, 12345, modulus, 0);      // 0*x % q = 0

    // Can't really calculate expected for these...
    test_mul_mod_helper(1, MAX_ZZ, modulus,
                        MAX_ZZ % q);                    // 1*MAX64 % q = MAX64 % q
    test_mul_mod_helper(1, 12345, modulus, 12345 % q);  // 1*x % q = x % q
}

void test_neg_mod_basic(Modulus *modulus)
{
    ZZ q = modulus->value;
    test_neg_mod_helper(0, modulus, 0);      // -0 % q = 0
    test_neg_mod_helper(1, modulus, q - 1);  // -1 % q = q-1
    test_neg_mod_helper(q - 1, modulus, 1);  // -(q-1) % q = 1
    test_neg_mod_helper(q, modulus, 0);      // -q % q = 0
}

void test_add_mod(void)
{
    printf("\n*******************************************\n");
    printf("Beginning tests for add_mod...\n\n");
    Modulus modulus;

    // -------------------------------
    //  Set 1: q = 2 (min value of q)
    // -------------------------------
    set_modulus(2, &modulus);
    test_add_mod_basic(&modulus);

    // ----------------
    //  Set 2: q = 10
    // ----------------
    set_modulus(10, &modulus);
    test_add_mod_basic(&modulus);
    test_add_mod_helper(7, 7, &modulus, 4);  // 7+7 % 10 = 4
    test_add_mod_helper(6, 7, &modulus, 3);  // 6+7 % 10 = 3

    // -----------------------
    // Set 3: max q
    // -----------------------
    set_modulus(MAX_Q, &modulus);
    test_add_mod_basic(&modulus);
    test_add_mod_basic(&modulus);

    printf("\n...all tests for add_mod passed.\n");
    printf("*******************************************\n");
}

void test_neg_mod(void)
{
    printf("\n*******************************************\n");
    printf("Beginning tests for neg_mod...\n\n");
    Modulus modulus;

    set_modulus(2, &modulus);
    test_neg_mod_basic(&modulus);

    set_modulus(0xFFFF, &modulus);
    test_neg_mod_basic(&modulus);
    test_neg_mod_helper(1234, &modulus, 64301);

    set_modulus(0x10000, &modulus);
    test_neg_mod_basic(&modulus);
    test_neg_mod_helper(1234, &modulus, 64302);

    printf("\n...all tests for neg_mod passed.\n");
    printf("*******************************************\n");
}

void test_mul_mod(void)
{
    printf("\n*******************************************\n");
    printf("Beginning tests for mul_mod...\n\n");
    Modulus modulus;

    // -------------------------------
    //  Set 1: q = 2 (min value of q)
    // -------------------------------
    set_modulus(2, &modulus);
    test_mul_mod_basic(&modulus);

    // ----------------
    //  Set 2: q = 10
    // ----------------
    set_modulus(10, &modulus);
    test_mul_mod_basic(&modulus);
    test_mul_mod_helper(7, 7, &modulus, 9);
    test_mul_mod_helper(6, 7, &modulus, 2);

    // -----------------------
    // Set 3: max q
    // -----------------------
    set_modulus(MAX_Q, &modulus);
    test_mul_mod_basic(&modulus);

    printf("\n...all tests for mul_mod passed.\n");
    printf("*******************************************\n");
}

// TODO: Add more test cases for above functions

// TODO: Add this test
// void test_mod_mumo_helper();

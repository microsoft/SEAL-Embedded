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

// -- Note: This uses % to check
void test_add_mod_helper(ZZ val1, ZZ val2, Modulus *modulus, ZZ res_exp)
{
    // -- Recall correctness requirement: val1 + val2 < 2q-1
    se_assert(modulus && modulus->value);
    ZZ q = modulus->value;
    printf("---------------------------------\n");
    for (int i = 0; i < 2; i++)
    {
        printf("( %" PRIuZZ " + %" PRIuZZ " ) %% %" PRIuZZ "\n", val1, val2, q);
        se_assert((val1 + val2) < (((modulus->value) * 2) - 1));
        ZZ res         = add_mod(val1, val2, modulus);
        ZZ res_default = (val1 + val2) % q;

        print_zz("Result         ", res);
        print_zz("Result expected", res_exp);
        print_zz("Result default ", res_default);

        se_assert(res == res_exp);
        se_assert(res == res_default);

        // -- Swap val1 and val2 and try again
        ZZ temp = val1;
        val1    = val2;
        val2    = temp;
        printf("(After swap)\n");
    }
}

// -- Note: This uses % to check
void test_mul_mod_helper(ZZ val1, ZZ val2, Modulus *modulus, ZZ res_exp)
{
    ZZ q = modulus->value;
    printf("---------------------------------\n");
    for (int i = 0; i < 2; i++)
    {
        printf("( %" PRIuZZ " * %" PRIuZZ " ) %% %" PRIuZZ "\n", val1, val2, q);
        ZZ res = mul_mod(val1, val2, modulus);

        print_zz("Result         ", res);
        print_zz("Result expected", res_exp);
        se_assert(res == res_exp);

        if (val1 < 0xFFFF && val2 < 0xFFFF)
        {
            ZZ res_default = (val1 * val2) % q;
            print_zz("Result default ", res_default);
            se_assert(res == res_default);
        }

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
    printf("( -%" PRIuZZ ") %% %" PRIuZZ "\n", input, q);
    se_assert(input <= q);

    // -- Recall correctness requirement: input must be < q
    ZZ res = neg_mod(input, modulus);
    print_zz("Result         ", res);
    print_zz("Result expected", res_exp);

    // -- Don't use the remainder operator, it does not work well for modular negation
    // -- Check the slow way
    ZZ temp = input;
    while (temp > q) temp -= q;
    size_t res_basic = (input == 0) ? 0 : q - temp;
    print_zz("Result basic   ", res_basic);

    se_assert(res == res_exp);
    se_assert(res == res_basic);
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
    test_add_mod_helper(q, q - 2, modulus, q - 2);  // q+q-2 = q-2 (mod q)

    test_add_mod_helper(q - 1, q - 1, modulus, q - 2);  // q-1+q-1 = q-2 (mod q)
    test_add_mod_helper(0, 2 * q - 2, modulus, q - 2);  // 0+2q-2 = q-2 (mod q)
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

    // -- Can't really calculate expected for these...
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

    set_modulus(134012929, &modulus);  // 27 bit
    test_add_mod_basic(&modulus);
    test_add_mod_helper(134012929 - 10, 134012929, &modulus, 134012929 - 10);
    test_add_mod_helper(134012929 + 10, 134012929 - 12, &modulus, 134012929 - 2);

    set_modulus(1053818881, &modulus);  // 30 bit
    test_add_mod_basic(&modulus);
    test_add_mod_helper(1053818881 - 10, 1053818881, &modulus, 1053818881 - 10);
    test_add_mod_helper(1053818881 + 10, 1053818881 - 12, &modulus, 1053818881 - 2);

    printf("\n...all tests for add_mod passed.\n");
    printf("*******************************************\n");
}

void test_neg_mod(void)
{
    printf("\n*******************************************\n");
    printf("Beginning tests for neg_mod...\n\n");
    Modulus modulus;

    set_modulus(134012929, &modulus);  // 27 bit
    test_neg_mod_basic(&modulus);
    test_neg_mod_helper(10, &modulus, 134012929 - 10);
    test_neg_mod_helper(134012929 - 10, &modulus, 10);

    set_modulus(1053818881, &modulus);  // 30 bit
    test_neg_mod_basic(&modulus);
    test_neg_mod_helper(10, &modulus, 1053818881 - 10);
    test_neg_mod_helper(1053818881 - 10, &modulus, 10);

    printf("\n...all tests for neg_mod passed.\n");
    printf("*******************************************\n");
}

void test_mul_mod(void)
{
    printf("\n*******************************************\n");
    printf("Beginning tests for mul_mod...\n\n");
    Modulus modulus;

    set_modulus(134012929, &modulus);  // 27 bit
    test_mul_mod_basic(&modulus);
    test_mul_mod_helper(0x38573475, 0x83748563, &modulus, 4025350);  // random

    set_modulus(1053818881, &modulus);  // 30 bit
    test_mul_mod_basic(&modulus);
    test_mul_mod_helper(0x38573475, 0x83748563, &modulus, 65334256);  // random

    printf("\n...all tests for mul_mod passed.\n");
    printf("*******************************************\n");
}

// TODO: Add this test
// void test_mod_mumo_helper();

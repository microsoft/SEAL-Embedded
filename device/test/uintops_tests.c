// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file uintops_tests.c
*/

#include <stdbool.h>
#include <string.h>  // memset

#include "defines.h"
#include "test_common.h"
#include "uint_arith.h"
#include "uintops.h"
#include "util_print.h"

void test_add_uint_helper(ZZ val1, ZZ val2, ZZ sum_exp, uint8_t carry_exp)
{
    for (int i = 0; i < 2; i++)
    {
        ZZ sum;
        uint8_t carry = add_uint(val1, val2, &sum);
        se_assert(carry == carry_exp);
        se_assert(sum == sum_exp);

        if (i == 0)
        {
            // -- Swap val1 and val2 and try again
            ZZ temp = val1;
            val1    = val2;
            val2    = temp;
        }
    }
}

void test_add_uint(void)
{
    printf("\n**********************************\n");
    printf("\nBeginnning tests for add_uint...\n\n");
    ZZ val1, val2, sum_exp;
    uint8_t c_exp;

    test_add_uint_helper(0, 0, 0, 0);
    test_add_uint_helper(1, 1, 2, 0);
    test_add_uint_helper(MAX_ZZ, 0, MAX_ZZ, 0);
    test_add_uint_helper(MAX_ZZ, 1, 0, 1);
    test_add_uint_helper(MAX_ZZ, MAX_ZZ, MAX_ZZ - 1, 1);
    test_add_uint_helper(MAX_ZZ - 1, 1, MAX_ZZ, 0);
    test_add_uint_helper(MAX_ZZ - 1, 2, 0, 1);

    ZZ val = random_zz();
    test_add_uint_helper(val, 0, val, 0);

    // -- Test that regular addition still works
    for (size_t i = 0; i < 5; i++)
    {
        printf("\n-------------\n");
        printf("\nTest number: %zu\n\n", i);
        switch (i)
        {
            case 0:  // max half * 2 = shift everything by 1
                val1    = 0xFFFF;
                val2    = 0xFFFF;
                sum_exp = val1 << 1;
                c_exp   = 0;
                break;
            case 1:
                val1    = 0xFFFFF;
                val2    = 0xFFFFF;
                sum_exp = val1 << 1;
                c_exp   = 0;
                break;
            case 2:
                val1    = 0x0F00F00F;
                val2    = ~val1;
                sum_exp = 0xFFFFFFFF;
                c_exp   = 0;
            case 3:
                val1    = 0x37281295;
                val2    = 0x15720382;
                sum_exp = 0x4c9a1617;
                c_exp   = 0;
            case 4:
                val1    = 0xd7281295;
                val2    = 0xa5720382;
                sum_exp = 0x7c9a1617;
                c_exp   = 1;
        }
        test_add_uint_helper(val1, val2, sum_exp, c_exp);
    }
    printf("**********************************\n");
}

void test_mult_uint_helper(ZZ val1, ZZ val2, ZZ *result_exp)
{
    ZZ result[2];
    for (int i = 0; i < 2; i++)
    {
        mul_uint_wide(val1, val2, result);
        se_assert(result[0] == result_exp[0]);
        se_assert(result[1] == result_exp[1]);

        if (i == 0)
        {
            // -- Swap inputs and try again
            memset(&result, 0, sizeof(result));
            ZZ temp = val1;
            val1    = val2;
            val2    = temp;
        }
    }
}

void test_mult_uint(void)
{
    printf("\n************************************\n");
    printf("\nBeginnning tests for mult_uint...\n\n");
    ZZ val1, val2;
    ZZ result_exp[2];

    memset(result_exp, 0, sizeof(result_exp));
    test_mult_uint_helper(0, 0, result_exp);

    memset(result_exp, 0, sizeof(result_exp));
    test_mult_uint_helper(1, 0, result_exp);

    for (size_t i = 0; i < 3; i++)
    {
        printf("\n-------------\n");
        printf("\nTest number: %zu\n\n", i);
        switch (i)
        {
            case 0:
                val1          = 0x10000;
                val2          = 0x0FABA;
                result_exp[1] = 0;
                result_exp[0] = 0xFABA0000;
                break;
            case 1:
                val1          = 0x100000;
                val2          = 0x00FABA;
                result_exp[1] = 0xF;
                result_exp[0] = 0xABA00000;
                break;

            case 2:
                val1          = 11223344;
                val2          = 55667788;
                result_exp[1] = 0x2383b;
                result_exp[0] = 0xa2879a40;
                break;
        }
        test_mult_uint_helper(val1, val2, result_exp);
    }
    printf("************************************\n");
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/*
@file sample_tests.c

Various tests for sampling a polynomial from a distribution.
*/

#include <string.h>  // memcmp

#include "ckks_common.h"
#include "defines.h"
#include "parameters.h"
#include "sample.h"
#include "test_common.h"
#include "util_print.h"  // printf

#ifdef SE_USE_MALLOC
#define TEST_SAMPLE_DEGREE 4096
#else
#define TEST_SAMPLE_DEGREE SE_DEGREE_N
#endif

void test_ternary_poly_stats(ZZ *s, size_t n)
{
    size_t num_zero = 0, num_one = 0, num_other = 0;

    for (size_t i = 0; i < n; i++)
    {
        if (s[i] == 0)
            num_zero++;
        else if (s[i] == 1)
            num_one++;
        else
            num_other++;
    }

    size_t percent_zero  = 100 * num_zero / n;
    size_t percent_one   = 100 * num_one / n;
    size_t percent_other = 100 * num_other / n;

    size_t threshold_lower = 29;
    size_t threshold_upper = 37;

    printf("Percent \'0\'     values (should be ~33%%) : %0.1f\n",
           ((double)num_zero / (double)n) * 100);
    printf("Percent \'1\'     values (should be ~33%%) : %0.1f\n",
           ((double)num_one / (double)n) * 100);
    printf("Percent \'other\' values (should be ~33%%) : %0.1f\n",
           ((double)num_other / (double)n) * 100);

    if (n > 64)
    {
        se_assert(percent_zero > threshold_lower);
        se_assert(percent_zero < threshold_upper);
        se_assert(percent_one > threshold_lower);
        se_assert(percent_one < threshold_upper);
        se_assert(percent_other > threshold_lower);
        se_assert(percent_other < threshold_upper);
    }
}

// TODO: Create tests for cbd sampling

void test_sample_poly_uniform(void)
{
    printf("\n******************************************\n");
    printf("Beginning test for sample_poly_uniform...\n");
    size_t n = TEST_SAMPLE_DEGREE;
    Parms parms;
    set_parms_ckks(n, 1, &parms);
    ZZ q = parms.curr_modulus->value;
    print_zz("q", q);
    printf("\n");
    SE_PRNG prng;
    prng_randomize_reset(&prng, NULL);

    // -- Sample
#ifdef SE_USE_MALLOC
    ZZ *a = calloc(n, sizeof(ZZ));
#else
    ZZ a[SE_DEGREE_N * sizeof(ZZ)];
#endif
    sample_poly_uniform(&parms, &prng, a);

    // -- Test
    size_t num_above    = 0;
    size_t num_below_eq = 0;
    for (int i = 0; i < n; i++)
    {
        if (a[i] > q / 2)
            num_above++;
        else
            num_below_eq++;
    }
    size_t percent_above    = 100 * num_above / n;
    size_t percent_below_eq = 100 * num_below_eq / n;
    size_t threshold_lower  = 47;
    size_t threshold_upper  = 53;
    se_assert(percent_above > threshold_lower);
    se_assert(percent_below_eq < threshold_upper);

    print_poly_sign("sampled a", (ZZsign *)a, n);
#ifdef SE_USE_MALLOC
    delete_parameters(&parms);
    free(a);
#endif
    printf("... done with tests for sample_poly_uniform.\n");
    printf("******************************************\n");
}

void test_sample_poly_ternary(void)
{
    printf("\n******************************************\n");
    printf("Beginning test for sample_poly_ternary...\n");
    size_t n = TEST_SAMPLE_DEGREE;
    Parms parms;
    set_parms_ckks(n, 1, &parms);
    print_zz("q", parms.curr_modulus->value);
    printf("\n");
    SE_PRNG prng;
    prng_randomize_reset(&prng, NULL);

    // -- Sample
#ifdef SE_USE_MALLOC
    ZZ *s = malloc(n * sizeof(ZZ));
#else
    ZZ s[SE_DEGREE_N * sizeof(ZZ)];
#endif
    sample_poly_ternary(&parms, &prng, s);

    // -- Test
    test_ternary_poly_stats(s, n);

    print_poly("sampled s", s, n);
#ifdef SE_USE_MALLOC
    delete_parameters(&parms);
    free(s);
#endif
    printf("... done with tests for sample_poly_ternary.\n");
    printf("******************************************\n");
}

#ifndef SE_USE_MALLOC
void test_sample_poly_ternary_small(void)
{
    printf("Error. This test is not runnable because SE_USE_MALLOC is not defined.\n");
}
#else
void test_sample_poly_ternary_small(void)
{
    printf("\n******************************************\n");
    printf("Beginning test for sample_poly_ternary_small...\n");

    size_t n = TEST_SAMPLE_DEGREE;
    Parms parms;
    set_parms_ckks(n, 3, &parms);
    print_zz("q", parms.curr_modulus->value);
    printf("\n");

    SE_PRNG prng;
    prng_randomize_reset(&prng, NULL);

    // -- Sample a polynomial in compressed form
    // -- Note: n = # of elements = # of 2-bit slots
    // --> # of bits required  = 2n
    // --> # of bytes required = 2n/8 = n/4
    size_t s_small_nbytes = n / 4;
    ZZ *s_small = calloc(s_small_nbytes, 1);
    sample_small_poly_ternary_prng_96(n, &prng, s_small);
    print_poly_small("s              ", s_small, n);
    ZZ *s_small_save = malloc(s_small_nbytes);
    memcpy(s_small_save, s_small, s_small_nbytes);

    // -- Test expansion
    size_t s_expanded_nbytes = n * sizeof(ZZ);
    ZZ *s_expanded = calloc(s_expanded_nbytes, 1);
    expand_poly_ternary(s_small, &parms, s_expanded);
    print_poly("s_expanded     ", s_expanded, n);
    test_ternary_poly_stats(s_expanded, n);

    // -- Test in-place expansion
    ZZ *s_inplace = realloc(s_small, s_expanded_nbytes);
    expand_poly_ternary_inpl(s_inplace, &parms);
    print_poly("sk expanded inpl", s_inplace, n);
    test_ternary_poly_stats(s_inplace, n);

    se_assert(!memcmp(s_inplace, s_expanded, s_expanded_nbytes));

    // -- Test conversion to next modulus prime
    for (size_t j = 0; j < 2; j++)
    {
        for (size_t nprimes = 0; nprimes < parms.nprimes; nprimes++)
        {
            print_zz("q", parms.curr_modulus->value);
            if (j == 1)
            {
                convert_poly_ternary_inpl(s_inplace, &parms);
                print_poly("s converted     ", s_inplace, n);
            }
            else
            {
                expand_poly_ternary(s_small_save, &parms, s_inplace);
                print_poly("s  expanded     ", s_inplace, n);
            }
            for (size_t i = 0; i < n; i++)
            {
                if (s_inplace[i] == 0 || s_inplace[i] == 1)
                {
                    se_assert(s_inplace[i] == s_expanded[i]);
                }
                else
                {
                    ZZ q = parms.curr_modulus->value;
                    if (s_inplace[i] != q - 1)
                    {
                        printf("s_inplace[%zu]: %" PRIuZZ "\n", i, s_inplace[i]);
                        printf("q: %" PRIuZZ "\n", q);
                    }
                    se_assert(s_inplace[i] == q - 1);
                }
            }
            if ((nprimes + 1) < parms.nprimes) next_modulus(&parms);
        }
        reset_primes(&parms);
    }

    delete_parameters(&parms);
    free(s_inplace);
    free(s_expanded);
    free(s_small_save);
    printf("... done with tests for sample_poly_ternary_small.\n");
    printf("******************************************\n");
}
#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ntt_tests.c
*/

#include "defines.h"
#ifndef SE_NTT_NONE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>  // memset

#include "intt.h"
#include "ntt.h"
#include "parameters.h"
#include "polymodmult.h"
#include "test_common.h"
#include "uintmodarith.h"
#include "util_print.h"

#ifndef SE_USE_MALLOC
#ifdef SE_NTT_FAST
#define NTT_TESTS_ROOTS_MEM 2 * SE_DEGREE_N
#elif defined(SE_NTT_REG) || defined(SE_NTT_ONE_SHOT)
#define NTT_TESTS_ROOTS_MEM SE_DEGREE_N
#else
#define NTT_TESTS_ROOTS_MEM 0
#endif
#ifdef SE_INTT_FAST
#define INTT_TESTS_ROOTS_MEM 2 * SE_DEGREE_N
#elif defined(SE_INTT_REG) || defined(SE_INTT_ONE_SHOT)
#define INTT_TESTS_ROOTS_MEM SE_DEGREE_N
#else
#define INTT_TESTS_ROOTS_MEM 0
#endif
#endif

// sb_res stores schoolbook results and must have 2n space
// ntt_roots either has space for n or 2n roots, depending on ntt option chosen
void test_poly_mult_ntt_only_helper(const Parms *parms, const ZZ *ntt_roots, ZZ *sb_res, ZZ *a,
                                    ZZ *b)
{
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;
    print_poly("a          ", a, n);
    print_poly("b          ", b, n);

    // -- We could get the ntt by the following equation:
    // 			    intt(ntt(a) . ntt(b)) 	= [a * b]_Rq
    // -- However, we do not necessarily have an intt implementation.
    // -- Therefore, can check ntt in the following way, by balancing
    //    the above equation:
    // 		--> ntt(intt(ntt(a) . ntt(b))) 	= ntt([a * b]_Rq)
    // 		-->          ntt(a) . ntt(b) 	= ntt([a * b]_Rq)
    // We can use either the schoolbook alg for a * b and reduce it mod Rq
    const char *left_side_str  = "ntt(a) . ntt(b)";
    const char *right_side_str = "ntt([a * b]_Rq)";

    poly_mult_mod_sb(a, b, n, mod, sb_res);
    print_poly("    [a * b]_Rq ", sb_res, n);

    // -- Calculate right side:  ntt([a * b]_Rq)
    ntt_inpl(parms, ntt_roots, sb_res);
    print_poly(right_side_str, sb_res, n);
    ntt_inpl(parms, ntt_roots, a);
    // print_poly("ntt(a)         ", a, n);
    ntt_inpl(parms, ntt_roots, b);
    // print_poly("ntt(b)         ", b, n);

    // -- Calculate left side:  ntt(a) . ntt(b)
    poly_mult_mod_ntt_form_inpl(a, b, n, mod);
    print_poly(left_side_str, a, n);

    // -- Compare left side with right side
    compare_poly(right_side_str, sb_res, left_side_str, a, n);
}

void test_poly_mult_ntt_intt_helper(const Parms *parms, const ZZ *ntt_roots, const ZZ *intt_roots,
                                    ZZ *sb_res, ZZ *a, ZZ *b)
{
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;
    print_poly("a          ", a, n);
    print_poly("b          ", b, n);

    // -- intt(ntt(a) . ntt(b)) = [a * b]_Rq
    const char *left_side_str  = "ntt(a) . ntt(b)";
    const char *right_side_str = "    [a * b]_Rq ";

    // -- First, make sure we can get back the original vector
    se_assert(sb_res && a);
    if (sb_res && a)
    {
        memcpy(sb_res, a, n * sizeof(a[0]));  // sb_res = a
    }

    ntt_inpl(parms, ntt_roots, sb_res);  // sb_res = ntt(a)
    print_poly("     ntt(a) ", sb_res, n);
    intt_inpl(parms, intt_roots, sb_res);  // sb_res = intt(ntt(a))
    print_poly("a           ", a, n);
    print_poly("intt(ntt(a))", sb_res, n);
    compare_poly("a           ", a, "intt(ntt(a))", sb_res, n);

    // -- Now, test multiplication
    // printf("About to perform schoolbook multiplication\n");
    poly_mult_mod_sb(a, b, n, mod, sb_res);  // This is very slow!
    // printf("Back from performing schoolbook multiplication\n");
    print_poly("    [a * b]_Rq ", sb_res, n);

    ntt_inpl(parms, ntt_roots, a);
    // print_poly("ntt(a)         ", a, n);

    ntt_inpl(parms, ntt_roots, b);
    // print_poly("ntt(b)         ", b, n);

    // -- Left side:  intt(ntt(a) . ntt(b))
    poly_mult_mod_ntt_form_inpl(a, b, n, mod);
    intt_inpl(parms, intt_roots, a);
    print_poly(left_side_str, a, n);

    // -- Finally, compare left side with right side
    compare_poly(right_side_str, sb_res, left_side_str, a, n);
}

/**
@param[in] n        Polynomial ring degree (ignored if SE_USE_MALLOC is defined)
@param[in] nprimes  # of modulus primes    (ignored if SE_USE_MALLOC is defined)
*/
void test_poly_mult_ntt(size_t n, size_t nprimes)
{
#ifndef SE_USE_MALLOC
    se_assert(n == SE_DEGREE_N && nprimes == SE_NPRIMES);  // sanity check
    if (n != SE_DEGREE_N) n = SE_DEGREE_N;
    if (nprimes != SE_NPRIMES) nprimes = SE_NPRIMES;
#endif

    printf("**********************************\n\n");
    printf("Beginning tests for poly_mult_mod_ntt");
    printf("....\n\n");

    bool intt_mult_test = true;
    Parms parms;
    set_parms_ckks(n, nprimes, &parms);
    print_test_banner("Ntt", &parms);

#ifdef SE_NTT_OTF
    size_t ntt_roots_size = 0;
#elif defined(SE_NTT_REG) || defined(SE_NTT_ONE_SHOT)
    size_t ntt_roots_size  = n;
#else  // defined(SE_NTT_FAST)
    size_t ntt_roots_size  = 2 * n;
#endif

#ifdef SE_INTT_OTF
    size_t intt_roots_size = 0;
#elif defined(SE_INTT_REG) || defined(SE_INTT_ONE_SHOT)
    size_t intt_roots_size = n;
#else  // defined(SE_INTT_FAST)
    size_t intt_roots_size = 2 * n;
#endif

    // ------------------
    //	Initialize memory
    // ------------------
    size_t mempool_size = 4 * n + ntt_roots_size + intt_roots_size;
#ifdef SE_USE_MALLOC
    ZZ *mempool = calloc(mempool_size, sizeof(ZZ));
#else
    se_assert(n == SE_DEGREE_N && nprimes == SE_NPRIMES);
    ZZ mempool_local[4 * SE_DEGREE_N + NTT_TESTS_ROOTS_MEM + INTT_TESTS_ROOTS_MEM];
    se_assert(mempool_size == (4 * SE_DEGREE_N + NTT_TESTS_ROOTS_MEM + INTT_TESTS_ROOTS_MEM));
    ZZ *mempool = &(mempool_local[0]);
    memset(mempool, 0, mempool_size * sizeof(ZZ));
#endif

    // clang-format off
    size_t idx = 0;  // start index
    ZZ *a          = &(mempool[idx]);                       idx += n;
    ZZ *b          = &(mempool[idx]);                       idx += n;
    ZZ *sb_res     = &(mempool[idx]);                       idx += 2*n;
    ZZ *ntt_roots  =  ntt_roots_size ? &(mempool[idx]) : 0; idx += ntt_roots_size;
    ZZ *intt_roots = intt_roots_size ? &(mempool[idx]) : 0; idx += intt_roots_size;
    se_assert(idx == mempool_size);
    // clang-format on

    while (1)
    {
        // ----------------------
        //	Initialize ntt roots
        // ----------------------
        ntt_roots_initialize(&parms, ntt_roots);
#ifdef SE_NTT_FAST
        for (size_t i = 0; i < 3; i++)
        {
            printf("\nNtt Mumo[%zu]: ", i);
            print_zz("operand", ((MUMO *)ntt_roots)[i].operand);
            print_zz("quotient", ((MUMO *)ntt_roots)[i].quotient);
        }
#elif !defined(SE_NTT_OTF)
        print_poly("ntt_roots", ntt_roots, n);
#endif
        if (intt_mult_test)
        {
            intt_roots_initialize(&parms, intt_roots);
#ifdef SE_INTT_FAST
            for (size_t i = 0; i < 3; i++)
            {
                printf("\nIntt Mumo[%zu]: ", i);
                print_zz("operand", ((MUMO *)intt_roots)[i].operand);
                print_zz("quotient", ((MUMO *)intt_roots)[i].quotient);
            }
#elif !defined(SE_INTT_OTF)
            print_poly("intt_roots", intt_roots, n);
#endif
        }

        // -------------------
        //	Run through tests
        // -------------------
        print_zz("Modulus", parms.curr_modulus->value);
        for (int testnum = 1; testnum < 14; testnum++)
        {
            printf("--------------- Test %d ------------------\n", testnum);
            clear(mempool, mempool_size - ntt_roots_size - intt_roots_size);  // Reset for each test
            switch (testnum)
            {
                case 0: break;                   // 0 * 0 = 0
                case 1: a[0] = b[0] = 1; break;  // 1 * 1 = 1
                case 2: a[1] = b[0] = 1; break;  // x * 1 = 1
                case 3:
                    a[n / 4] = 2;
                    b[0]     = 1;
                    break;  // x * 1 = x

                case 4:
                    set(a, n, 2);  // {2, 2, 2, ...}
                    b[0] = 1;      // * 1
                    break;

                case 5:
                    set(a, n, 1);  // {1, 1, 1, ...}
                    b[0] = 2;      // * 2
                    break;

                case 6: {
                    ZZ v[] = {1, 1, 0};
                    memcpy(a, v, sizeof(v));
                }
                    {
                        ZZ v[] = {1, 0, 0};
                        memcpy(b, v, sizeof(v));
                    }
                    break;

                case 7: {
                    ZZ v[] = {1, 1, 0};
                    memcpy(a, v, sizeof(v));
                    memcpy(b, v, sizeof(v));
                }
                break;

                case 8: {
                    ZZ v[] = {1, 1, 1, 0, 0};
                    memcpy(a, v, sizeof(v));
                    memcpy(b, v, sizeof(v));
                }
                break;

                case 9:
                    a[1] = 1;  //  {0, 1}
                    b[0] = 1;  // * 1
                    break;

                case 10:
                    a[n - 1] = 1;  //  {0, ..., 0, 1}
                    b[0]     = 1;  // * 1
                    break;

                case 11:
                    a[n - 1] = 1;  //   {0, ..., 0, 1}
                    b[1]     = 1;  // * {0, 1}
                    break;

                case 12:
                    set(a, n, 1);  // 	{1, 1, 1, ...}
                    set(b, n, 1);  // * {1, 1, 1, ...}
                    break;

                case 13:
                    random_zzq_poly(a, n, parms.curr_modulus);  //   rand_x
                    random_zzq_poly(b, n, parms.curr_modulus);  // * rand_y
                    break;
                default: break;
            }
            if (intt_mult_test)
                test_poly_mult_ntt_intt_helper(&parms, ntt_roots, intt_roots, sb_res, a, b);
            else
                test_poly_mult_ntt_only_helper(&parms, ntt_roots, sb_res, a, b);
        }
        if ((parms.curr_modulus_idx + 1) < parms.nprimes)
        {
            bool ret = next_modulus(&parms);
            se_assert(ret);
        }
        else
            break;
    }
#ifdef SE_USE_MALLOC
    if (mempool)
    {
        free(mempool);
        mempool = 0;
    }
#endif
    delete_parameters(&parms);
}
#endif

#ifdef SE_USE_MALLOC
#ifdef NTT_TESTS_ROOTS_MEM
#undef NTT_TESTS_ROOTS_MEM
#endif
#ifdef INTT_TESTS_ROOTS_MEM
#undef INTT_TESTS_ROOTS_MEM
#endif
#endif

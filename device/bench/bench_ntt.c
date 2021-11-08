// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file bench_ntt.c
*/

#include "defines.h"

#if defined(SE_ENABLE_TIMERS)
#include <stdbool.h>
#include <stdio.h>

#include "bench_common.h"
#include "ckks_common.h"
#include "fileops.h"
#include "ntt.h"
#include "parameters.h"
#include "timer.h"
#include "util_print.h"

// -- Configuration
// #define SE_BENCH_NTT_ROOTS
// #define SE_BENCH_NTT_COMP
#define SE_BENCH_NTT_FULL

// -- Sanity check
#if !defined(SE_BENCH_NTT_ROOTS) && !defined(SE_BENCH_NTT_COMP) && !defined(SE_BENCH_NTT_FULL)
#define SE_BENCH_NTT_ROOTS
#endif

#ifndef SE_USE_MALLOC
#ifdef SE_NTT_FAST
#define NTT_TESTS_ROOTS_MEM 2 * SE_DEGREE_N
#elif defined(SE_NTT_REG) || defined(SE_NTT_ONE_SHOT)
#define NTT_TESTS_ROOTS_MEM SE_DEGREE_N
#else
#define NTT_TESTS_ROOTS_MEM 0
#endif
#endif

void bench_ntt(void)
{
#ifdef SE_USE_MALLOC
    const PolySizeType n = 4096;
#else
    const PolySizeType n = SE_DEGREE_N;
#endif

    const size_t vec_size = n;

#ifdef SE_USE_MALLOC

#ifdef SE_NTT_FAST
    size_t ntt_roots_size = 2 * n;
#elif defined(SE_NTT_OTF)
    size_t ntt_roots_size = 0;
#else
    size_t ntt_roots_size = n;
#endif

    size_t mempool_size = vec_size + ntt_roots_size;
    ZZ *mempool         = calloc(mempool_size, sizeof(ZZ));
#else
    ZZ mempool[SE_DEGREE_N + NTT_TESTS_ROOTS_MEM];
    memset(&mempool, 0, (SE_DEGREE_N + NTT_TEST_ROOTS_MEM) * sizeof(ZZ));
#endif

    ZZ *vec = mempool;

#ifdef SE_NTT_OTF
    ZZ *ntt_roots = 0;
#else
    ZZ *ntt_roots          = &(mempool[vec_size]);
#endif

    Parms parms;
    parms.nprimes = 1;
    ckks_setup(n, 1, NULL, &parms);

#ifdef SE_BENCH_NTT_ROOTS
    const char *bench_name = "ntt (timing roots load/gen)";
#elif defined(SE_BENCH_NTT_COMP)
    const char *bench_name = "ntt (timing computation)";
#else
    const char *bench_name = "ntt (timing roots + computation)";
#endif
    print_bench_banner(bench_name, &parms);

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        t_curr = 0;
        random_zzq_poly(vec, n, parms.curr_modulus);

#ifdef SE_NTT_FAST
        se_secure_zero_memset(ntt_roots, n * sizeof(ZZ) * 2);
#elif defined(SE_NTT_REG) || defined(SE_NTT_ONE_SHOT)
        se_secure_zero_memset(ntt_roots, n * sizeof(ZZ));
#endif

#if defined(SE_BENCH_NTT_ROOTS) || defined(SE_BENCH_NTT_FULL)
        printf("curr runtime (us) = %0.2f\n", t_curr);
        reset_start_timer(&timer);
#endif

        // -- Load or generate the roots
        ntt_roots_initialize(&parms, ntt_roots);

#ifdef SE_BENCH_NTT_ROOTS
        stop_timer(&timer);
        t_curr += read_timer(timer, MICRO_SEC);
#elif defined(SE_BENCH_NTT_COMP)
        reset_start_timer(&timer);
#endif

        // -- Ntt computation
        ntt_inpl(&parms, ntt_roots, vec);

#if defined(SE_BENCH_NTT_COMP) || defined(SE_BENCH_NTT_FULL)
        stop_timer(&timer);
        t_curr += read_timer(timer, MICRO_SEC);
#endif
        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_full("ntt(vec)", vec, n);
    }

    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
    print_bench_banner(bench_name, &parms);

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
#endif

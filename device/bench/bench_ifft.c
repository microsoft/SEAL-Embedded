// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file bench_ifft.h
*/

#include "defines.h"

#ifdef SE_ENABLE_TIMERS
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "bench_common.h"
#include "ckks_common.h"
#include "fft.h"
#include "fileops.h"
#include "parameters.h"
#include "timer.h"
#include "util_print.h"

// -- Configuration
// #define SE_BENCH_IFFT_ROOTS
// #define SE_BENCH_IFFT_COMP
#define SE_BENCH_IFFT_FULL

// -- Sanity check
#if !defined(SE_BENCH_IFFT_ROOTS) && !defined(SE_BENCH_IFFT_COMP) && !defined(SE_BENCH_IFFT_FULL)
#define SE_BENCH_IFFT_ROOTS
#endif

#ifndef SE_USE_MALLOC
#if defined(SE_IFFT_LOAD_FULL) || defined(SE_IFFT_ONE_SHOT)
#define IFFT_TEST_ROOTS_MEM SE_DEGREE_N
#else
#define IFFT_TEST_ROOTS_MEM 0
#endif
#endif

void bench_ifft(void)
{
#ifdef SE_USE_MALLOC
    const PolySizeType n = 4096;
#else
    const PolySizeType n   = SE_DEGREE_N;
#endif
    size_t logn = get_log2(n);

    size_t size_mult = sizeof(double complex) / sizeof(ZZ);
    size_t vec_size  = n * size_mult;
#ifdef SE_IFFT_OTF
    size_t ifft_roots_size = 0;
#else
    size_t ifft_roots_size = n * size_mult;
#endif

    size_t mempool_size = vec_size + ifft_roots_size;
#ifdef SE_USE_MALLOC
    ZZ *mempool = calloc(mempool_size, sizeof(ZZ));
#else
    ZZ mempool[SE_DEGREE_N + IFFT_TEST_ROOTS_MEM];
    memset(&mempool, 0, (SE_DEGREE_N + IFFT_TEST_ROOTS_MEM) * sizeof(ZZ));
#endif

    double complex *vec = (double complex *)mempool;

#ifdef SE_IFFT_OTF
    double complex *ifft_roots = 0;
#else
    double complex *ifft_roots = (double complex *)&(mempool[vec_size]);
#endif

    Parms parms;
    ckks_setup(n, 1, NULL, &parms);

#ifdef SE_BENCH_IFFT_ROOTS
    const char *bench_name = "inverse fft (timing roots load/gen)";
#elif defined(SE_BENCH_IFFT_COMP)
    const char *bench_name     = "inverse fft (timing computation)";
#else
    const char *bench_name = "inverse fft (timing roots + computation)";
#endif
    print_bench_banner(bench_name, &parms);

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0;
    volatile float t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        t_curr = 0;
        gen_double_complex_half_vec(vec, pow(10, 6), n);

#if defined(SE_IFFT_LOAD_FULL)
        se_secure_zero_memset(ifft_roots, n * sizeof(double complex));
#endif

#if defined(SE_BENCH_IFFT_ROOTS) || defined(SE_BENCH_IFFT_FULL)
        reset_start_timer(&timer);
#endif
        // -- Load or generate the roots --
#ifdef SE_IFFT_LOAD_FULL
        se_assert(ifft_roots);
        load_ifft_roots(n, ifft_roots);
#endif

#ifdef SE_BENCH_IFFT_ROOTS
        stop_timer(&timer);
        t_curr += read_timer(timer, MICRO_SEC);
#elif defined(SE_BENCH_IFFT_COMP)
        reset_start_timer(&timer);
#endif

        // -- Ifft computation --
        ifft_inpl(vec, n, logn, ifft_roots);

#if defined(SE_BENCH_IFFT_COMP) || defined(SE_BENCH_IFFT_FULL)
        stop_timer(&timer);
        t_curr += read_timer(timer, MICRO_SEC);
#endif

        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_double_complex_full("ifft(vec)", vec, n);
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

#ifndef SE_USE_MALLOC
#ifdef IFFT_TEST_ROOTS_MEM
#undef IFFT_TEST_ROOTS_MEM
#endif
#endif

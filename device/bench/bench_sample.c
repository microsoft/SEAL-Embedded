// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file bench_sample.c
*/

#include "defines.h"
#ifdef SE_ENABLE_TIMERS
#include "bench_common.h"
#include "sample.h"
#include "timer.h"
#include "util_print.h"

void bench_sample_poly_cbd(void)
{
#ifdef SE_USE_MALLOC
    const size_t n = 4096;
    int8_t *vec    = calloc(n, sizeof(int8_t));
#else
    const size_t n = SE_DEGREE_N;
    int8_t vec[SE_DEGREE_N];
#endif

    int8_t *poly           = &(vec[0]);
    const char *bench_name = "sample poly cbd";
    print_bench_banner(bench_name, 0);

    SE_PRNG prng;
    prng_randomize_reset(&prng, NULL);

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        reset_start_timer(&timer);

        sample_poly_cbd_generic_prng_16(n, &prng, poly);  // now stores [e1]

        stop_timer(&timer);
        t_curr = read_timer(timer, MICRO_SEC);
        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_int8_full("cbd poly", poly, n);
    }
    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
#ifdef SE_USE_MALLOC
    if (vec)
    {
        free(vec);
        vec = 0;
    }
#endif
}

void bench_sample_ternary_small(void)
{
#ifdef SE_USE_MALLOC
    const size_t n = 4096;
    ZZ *vec        = malloc(n / 4);
#else
    const size_t n = SE_DEGREE_N;
    ZZ vec[SE_DEGREE_N / 4];
#endif

    Parms parms;
    parms.small_u = 1;
    set_parms_ckks(n, 1, &parms);

    ZZ *poly               = &(vec[0]);
    const char *bench_name = "sample poly ternary (small)";
    print_bench_banner(bench_name, &parms);
    SE_PRNG prng;
    prng_randomize_reset(&prng, NULL);
    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        reset_start_timer(&timer);

        sample_small_poly_ternary_prng_96(n, &prng, poly);

        stop_timer(&timer);
        t_curr = read_timer(timer, MICRO_SEC);
        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_ternary_full("ternary (small) poly", poly, n, 1);
    }
    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
#ifdef SE_USE_MALLOC
    if (vec)
    {
        free(vec);
        vec = 0;
    }
    delete_parameters(&parms);
#endif
}

void bench_sample_uniform(void)
{
#ifdef SE_USE_MALLOC
    const size_t n = 4096;
    ZZ *vec        = calloc(n, sizeof(ZZ));
#else
    const size_t n = SE_DEGREE_N;
    ZZ vec[SE_DEGREE_N];
#endif

    Parms parms;
    set_parms_ckks(n, 1, &parms);

    ZZ *poly               = &(vec[0]);
    const char *bench_name = "sample poly uniform";
    print_bench_banner(bench_name, &parms);

    SE_PRNG prng;
    prng_randomize_reset(&prng, NULL);

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        reset_start_timer(&timer);

        sample_poly_uniform(&parms, &prng, poly);

        stop_timer(&timer);
        t_curr = read_timer(timer, MICRO_SEC);
        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_full("uniform poly", poly, n);
    }
    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
#ifdef SE_USE_MALLOC
    if (vec)
    {
        free(vec);
        vec = 0;
    }
    delete_parameters(&parms);
#endif
}

void bench_prng_randomize_seed(void)
{
    const char *bench_name = "prng randomize seed";
    print_bench_banner(bench_name, 0);

    SE_PRNG prng;
    Timer timer;

    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        reset_start_timer(&timer);

        prng_randomize_reset(&prng, NULL);

        stop_timer(&timer);
        t_curr = read_timer(timer, MICRO_SEC);
        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_uint8_full("random seed", prng.seed, SE_PRNG_SEED_BYTE_COUNT);
    }
    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
}

void bench_prng_fill_buffer(void)
{
    const char *bench_name = "prng fill buffer";
    print_bench_banner(bench_name, 0);

#ifdef SE_USE_MALLOC
    const size_t n = 4096;
    ZZ *vec        = calloc(n, sizeof(ZZ));
#else
    const size_t n = SE_DEGREE_N;
    ZZ vec[SE_DEGREE_N];
#endif
    ZZ *poly = &(vec[0]);

    SE_PRNG prng;

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        prng_randomize_reset(&prng, NULL);
        reset_start_timer(&timer);

        prng_fill_buffer(n * sizeof(ZZ), &prng, (void *)poly);

        stop_timer(&timer);
        t_curr = read_timer(timer, MICRO_SEC);
        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_full("random buffer", vec, n);
    }
    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
#ifdef SE_USE_MALLOC
    if (vec)
    {
        free(vec);
        vec = 0;
    }
#endif
}

void bench_prng_randomize_seed_fill_buffer(void)
{
    const char *bench_name = "prng randomize + fill buffer";
    print_bench_banner(bench_name, 0);

#ifdef SE_USE_MALLOC
    const size_t n = 4096;
    ZZ *vec        = calloc(n, sizeof(ZZ));
#else
    const size_t n = SE_DEGREE_N;
    ZZ vec[SE_DEGREE_N];
#endif
    ZZ *poly = &(vec[0]);

    SE_PRNG prng;

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        reset_start_timer(&timer);

        prng_randomize_reset(&prng, NULL);
        prng_fill_buffer(n * sizeof(ZZ), &prng, (void *)poly);

        stop_timer(&timer);
        t_curr = read_timer(timer, MICRO_SEC);
        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);
        print_poly_full("random buffer", vec, n);
    }
    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
#ifdef SE_USE_MALLOC
    if (vec)
    {
        free(vec);
        vec = 0;
    }
#endif
}
#endif

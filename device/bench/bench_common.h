// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file bench_common.h
*/

#pragma once

#include "defines.h"
#include "modulo.h"      // barrett_reduce
#include "sample.h"      // random_zz
#include "util_print.h"  // print functions, printf

/*
Generate "random" uint
Note: random_zz is already defined in sample.h
*/
static inline ZZ random_zzq(Modulus *q)
{
    return barrett_reduce(random_zz(), q);
}

static inline ZZ random_zz_half(void)
{
    // -- Don't do this. It doesn't work.
    // ZZ result;
    // getrandom((void*)&result, sizeof(result)/2, 0);
    return random_zz() & 0xFFFF;
}

static inline ZZ random_zz_quarter(void)
{
    return random_zz() & 0xFF;
}

// Generate "random" double
static inline double gen_double_half(int64_t div)
{
    return (double)random_zz_half() / (double)div;
}

// "Random" uint poly
static inline void random_zz_poly(ZZ *poly, size_t n)
{
    for (size_t i = 0; i < n; i++) { poly[i] = random_zz(); }
}

static inline void random_zzq_poly(ZZ *poly, size_t n, Modulus *q)
{
    for (size_t i = 0; i < n; i++) { poly[i] = random_zzq(q); }
}

// "Random" double complex
static inline void gen_double_complex_half_vec(double complex *vec, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++)
    { vec[i] = (double complex)_complex(gen_double_half(div), gen_double_half(div)); }
}

static inline double gen_double_quarter(int64_t div)
{
    return (double)random_zz_quarter() / (double)div;
}

static inline flpt gen_flpt_quarter(int64_t div)
{
    return (flpt)gen_double_quarter(div);
}

// "Random" flpt
static inline void gen_flpt_quarter_poly(flpt *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_flpt_quarter(div);
}

static inline void print_bench_banner(const char *benchmark_name, const Parms *parms)
{
#if defined(SE_ON_NRF5) && defined(SE_NRF5_UART_PRINTF_ENABLED)
    return;  // Printing banner on the NRF5 via serial doesn't work well. Just don't do it.
#endif
    printf("***************************************************\n");
    printf("Running Benchmark: %s\n", benchmark_name);
    if (parms)
    {
        printf("n: %zu, nprimes: %zu, scale: %0.2f\n", parms->coeff_count, parms->nprimes,
               parms->scale);
        print_config(!parms->is_asymmetric);
    }
    printf("***************************************************\n");
}

static inline void set_time_vals(float time_curr, float *time_total, float *time_min,
                                 float *time_max)
{
    se_assert(time_total);
    time_total[0] += time_curr;
    if (time_min && ((time_curr < time_min[0]) || (time_min[0] == 0))) time_min[0] = time_curr;
    if (time_max && (time_curr > time_max[0])) time_max[0] = time_curr;
}

static inline void print_time_vals(const char *name, float time_curr, size_t num_runs,
                                   float *time_total, float *time_min, float *time_max)
{
    printf("\n\n");
    for (size_t k = 0; k < 3; k++)
    {
        printf("-- Runtimes out of %zu runs (%s) --\n", num_runs, name);
        printf("curr runtime (us) = %0.2f\n", time_curr);
        if (num_runs) printf("avg  runtime (us) = %0.2f\n", time_total[0] / (float)num_runs);
        printf("max  runtime (us) = %0.2f\n", time_max[0]);
        printf("min  runtime (us) = %0.2f\n", time_min[0]);
    }
}

static inline void set_print_time_vals(const char *name, float time_curr, size_t num_runs,
                                       float *time_total, float *time_min, float *time_max)
{
    set_time_vals(time_curr, time_total, time_min, time_max);
    print_time_vals(name, time_curr, num_runs, time_total, time_min, time_max);
}

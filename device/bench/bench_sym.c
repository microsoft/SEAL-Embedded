// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file bench_sym.c
*/

#include "defines.h"
#ifdef SE_ENABLE_TIMERS
#include <stdbool.h>

#include "bench_common.h"
#include "ckks_common.h"
#include "ckks_sym.h"
#include "parameters.h"
#include "timer.h"

// -- Configuration
// #define SE_BENCH_ENCODE
// #define SE_BENCH_SAMPLE
// #define SE_BENCH_ENCRYPT
#define SE_BENCH_FULL

// -- Sanity check
#if !defined(SE_BENCH_ENCODE) && !defined(SE_BENCH_SAMPLE) && !defined(SE_BENCH_ENCRYPT) && \
    !defined(SE_BENCH_FULL)
#define SE_BENCH_ENCODE
#endif

void bench_sym(void)
{
#ifdef SE_USE_MALLOC
    const size_t n       = 4096;
    const size_t nprimes = 3;
#else
    const size_t n       = SE_DEGREE_N;
    const size_t nprimes = SE_NPRIMES;
#endif
    Parms parms;
    parms.is_asymmetric = false;
    parms.small_s       = true;
    parms.sample_s      = false;

    // -- Make sure we didn't set this accidentally
    if (!parms.sample_s) se_assert(parms.small_s);

#ifdef SE_USE_MALLOC
    print_ckks_mempool_size(n, 1);
    ZZ *mempool = ckks_mempool_setup_sym(n);
#else
    print_ckks_mempool_size();
    ZZ mempool_local[MEMPOOL_SIZE];
    memset(&mempool_local, 0, MEMPOOL_SIZE * sizeof(ZZ));
    ZZ *mempool = &(mempool_local[0]);
#endif

    // -- Get pointers
    SE_PTRS se_ptrs_local;
    ckks_set_ptrs_sym(n, mempool, &se_ptrs_local);
    double complex *conj_vals  = se_ptrs_local.conj_vals;
    int64_t *conj_vals_int     = se_ptrs_local.conj_vals_int_ptr;
    double complex *ifft_roots = se_ptrs_local.ifft_roots;
    ZZ *c0                     = se_ptrs_local.c0_ptr;
    ZZ *c1                     = se_ptrs_local.c1_ptr;
    uint16_t *index_map        = se_ptrs_local.index_map_ptr;
    ZZ *ntt_roots              = se_ptrs_local.ntt_roots_ptr;
    ZZ *ntt_pte                = se_ptrs_local.ntt_pte_ptr;
    ZZ *s                      = se_ptrs_local.ternary;
    flpt *v                    = se_ptrs_local.values;
    size_t vlen                = n / 2;

    SE_PRNG prng;
    SE_PRNG shareable_prng;

    // -- Set up parameters and index_map if applicable
    ckks_setup(n, nprimes, index_map, &parms);

    // -- If s is allocated space ahead of time, can load ahead of time too
    // -- If we are testing and sample s is set, this will also sample s
    ckks_setup_s(&parms, NULL, &prng, s);

    const char *bench_name = "Symmetric_Encryption";
    print_bench_banner(bench_name, &parms);

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        t_curr = 0;
        se_assert(parms.nprimes >= 1);
        gen_flpt_quarter_poly(v, -10, vlen);
        // print_poly_flpt("v        ", v, vlen);

        // -- Begin encode-encrypt sequence
#if defined(SE_BENCH_ENCODE) || defined(SE_BENCH_FULL)
        reset_start_timer(&timer);
#endif
        // -- First, encode base
        ckks_encode_base(&parms, v, vlen, index_map, ifft_roots, conj_vals);
        // print_poly_int64_full("conj_vals_int      ", conj_vals_int_ptr, n);

#ifdef SE_BENCH_ENCODE
        stop_timer(&timer);
        t_curr += read_timer(timer, MICRO_SEC);
#elif defined(SE_BENCH_SAMPLE)
        reset_start_timer(&timer);
#endif
        // -- First prime, so need to sample error e
        // -- While sampling error e, add it in place to the base message
        ckks_sym_init(&parms, NULL, NULL, &shareable_prng, &prng, conj_vals_int);

#if defined(SE_BENCH_SAMPLE) || defined(SE_BENCH_FULL)
        stop_timer(&timer);
        t_curr += read_timer(timer, MICRO_SEC);
#endif

        for (size_t i = 0; i < parms.nprimes; i++)
        {
#if defined(SE_BENCH_ENCRYPT) || defined(SE_BENCH_FULL)
            reset_start_timer(&timer);
#endif
            // -- Per prime Encode + Encrypt
            ckks_encode_encrypt_sym(&parms, conj_vals_int, 0, &shareable_prng, s, ntt_pte,
                                    ntt_roots, c0, c1, 0, 0);

#if defined(SE_BENCH_ENCRYPT) || defined(SE_BENCH_FULL)
            stop_timer(&timer);
            t_curr += read_timer(timer, MICRO_SEC);
#endif

            if (b_itr && ((i + 1) == parms.nprimes))
                set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);

            // -- We must execute these printf calls to ensure the compiler does not
            //    optimize away generation of c0 and c1 polynomials. (Note that in the
            //    ifft truly on-the-fly case, c1 is overwritten by c0 and their
            //    outputs at this point will therefore be the same. We do not have to
            //    be concerned about c1 not being evaluated in this case though since
            //    c1 == a, which is necessary for the generation of c0.)
            print_poly_full("c0 ", c0, n);
            print_poly_full("c1 ", c1, n);
            if ((i + 1) < parms.nprimes) ckks_next_prime_sym(&parms, s);
        }
    }

    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);
    print_bench_banner(bench_name, &parms);

#ifdef SE_USE_MALLOC
    if (mempool)
    {
        free(mempool);
        mempool = 0;
    }
    delete_parameters(&parms);
#endif
}
#endif

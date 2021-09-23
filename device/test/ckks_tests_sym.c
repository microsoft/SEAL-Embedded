// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_test_sym.c
*/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "ckks_common.h"
#include "ckks_sym.h"
#include "ckks_tests_common.h"
#include "defines.h"
#include "fft.h"
#include "fileops.h"
#include "ntt.h"
#include "polymodarith.h"
#include "polymodmult.h"
#include "sample.h"
#include "test_common.h"
#include "util_print.h"

#if !((defined(SE_ON_SPHERE_M4) || defined(SE_ON_NRF5)) && !defined(SE_ENCRYPT_TYPE_SYMMETRIC))
/**
Symmetric test core. If debugging is enabled, throws an error if a test fails.

@param[in] test_message  If 0, sets the message 0
*/
void test_ckks_sym_base(bool test_message)
{
#ifdef SE_USE_MALLOC
    size_t n       = 4096;
    size_t nprimes = 3;
#else
    size_t n       = SE_DEGREE_N;
    size_t nprimes = SE_NPRIMES;
#endif

    Parms parms;
    parms.scale = (n > 1024) ? pow(2, 25) : pow(2, 20);
    // parms.scale       = pow(2, 40);
    // parms.scale       = n * n;
    bool encode_only = 0;  // this only works if s is not not persistent
    // =========================
    parms.is_asymmetric = 0;
    parms.small_s       = 1;
    parms.sample_s      = 0;

    // -- Make sure we didn't set this accidentally
    if (!parms.sample_s) se_assert(parms.small_s);

#ifdef SE_USE_MALLOC
    print_ckks_mempool_size(n, 1);
    ZZ *mempool = ckks_mempool_setup_sym(n);
#else
    print_ckks_mempool_size();
    ZZ mempool_local[MEMPOOL_SIZE];
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

    if (!test_message)
    {
        memset(v, 0, vlen * sizeof(flpt));
        // v = 0;
    }

// -- Additional pointers required for testing.
#ifdef SE_USE_MALLOC
    ZZ *s_test_save   = calloc(n, sizeof(ZZ));  // ntt(expanded(s)) or expanded(s)
    ZZ *c1_test_save  = calloc(n, sizeof(ZZ));
    ZZ *temp_test_mem = calloc(4 * n, sizeof(ZZ));
#else
    ZZ s_test_save_vec[SE_DEGREE_N];  // ntt(expanded(s)) or expanded(s)
    ZZ c1_test_save_vec[SE_DEGREE_N];
    ZZ temp_test_mem_vec[4 * SE_DEGREE_N];
    ZZ *s_test_save = &(s_test_save_vec[0]);
    // printf("s_save address: %p\n", s_test_save);
    ZZ *c1_test_save  = &(c1_test_save_vec[0]);
    ZZ *temp_test_mem = &(temp_test_mem_vec[0]);
// printf("temp_test_mem address: %p\n", temp_test_mem);
// printf("temp_test_mem address: %p\n", &(temp_test_mem[4*SE_DEGREE_N-1]));
#endif

    SE_PRNG prng;
    SE_PRNG shareable_prng;

    // -- Set up parameters and index_map if applicable
    ckks_setup(n, nprimes, index_map, &parms);

    print_test_banner("Symmetric Encryption", &parms);

    // -- If s is allocated space ahead of time, can load ahead of time too
    // -- If we are testing and sample s is set, this will also sample s
    ckks_setup_s(&parms, NULL, &prng, s);
    size_t s_size = parms.small_s ? n / 16 : n;
    if (encode_only) clear(s, s_size);

    for (size_t testnum = 0; testnum < 9; testnum++)
    {
        printf("-------------------- Test %zu -----------------------\n", testnum);
        ckks_reset_primes(&parms);

        // -- Set test values
        if (test_message)
        {
            set_encode_encrypt_test(testnum, vlen, v);
            print_poly_flpt("v        ", v, vlen);
        }

        // -- Begin encode-encrypt sequence
        // -- First, calculate m + e (not fully reduced, not in ntt form)
        if (test_message)
        {
            bool ret = ckks_encode_base(&parms, v, vlen, index_map, ifft_roots, conj_vals);
            se_assert(ret);
        }
        else
            memset(conj_vals_int, 0, n * sizeof(conj_vals_int[0]));
        // print_poly_int64("conj_vals_int      ", conj_vals_int, n);
        se_assert(v);

        // -- Sample error e. While sampling e, add it in place to the base message.
        if (!encode_only)
        {
            ckks_sym_init(&parms, NULL, NULL, &shareable_prng, &prng, conj_vals_int);
        }

        for (size_t i = 0; i < parms.nprimes; i++)
        {
            print_zz("\n ***** Modulus", parms.curr_modulus->value);

            // -- Per prime Encode + Encrypt
            ckks_encode_encrypt_sym(&parms, conj_vals_int, NULL, &shareable_prng, s, ntt_pte,
                                    ntt_roots, c0, c1, s_test_save, c1_test_save);
            // print_poly_int64("conj_vals_int", conj_vals_int, n);
            // print_poly_ternary("s", s, n, true);
            // print_poly_ternary("s_save", s, n, false);

            // -- Check that decrypt gives back the pt+err and decode gives back v.
            // -- Note: This will only decode if values is non-zero. Otherwise, will
            //    just decrypt.
            // -- Note: sizeof(max(ntt_roots, ifft_roots)) must be passed as temp memory
            //    to undo ifft
            bool s_test_save_small = false;
            check_decode_decrypt_inpl(c0, c1_test_save, v, vlen, s_test_save, s_test_save_small,
                                      ntt_pte, index_map, &parms, temp_test_mem);

            // -- Done checking this prime. Now try next prime if requested
            bool ret = ckks_next_prime_sym(&parms, s);
            se_assert(ret || (!ret && i + 1 == parms.nprimes));
        }

        // -- Can exit now if rlwe testing only
        if (!test_message) break;
    }

#ifdef SE_USE_MALLOC
    free(mempool);
    free(s_test_save);
    free(c1_test_save);
    free(temp_test_mem);
#endif
    delete_parameters(&parms);
}

/**
Full encode + symmetric encrypt test
*/
void test_ckks_encode_encrypt_sym(void)
{
    printf("Beginning tests for ckks encode + symmetric encrypt...\n");
    bool test_message = 1;
    test_ckks_sym_base(test_message);
}

/**
Symmetric rlwe test only (message is the all-zeros vector)
*/
void test_enc_zero_sym(void)
{
    printf("Beginning tests for rlwe symmetric encryption of 0...\n");
    bool test_message = 0;
    test_ckks_sym_base(test_message);
}
#else

void test_ckks_encode_encrypt_sym(void)
{
    printf("Error! Did you choose the wrong configuration settings?\n");
}

void test_enc_zero_sym(void)
{
    printf("Error! Did you choose the wrong configuration settings?\n");
}

#endif

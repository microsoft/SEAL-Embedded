// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_tests_asym.c
*/

#include <math.h>  // pow
#include <stdbool.h>
#include <stdio.h>

#include "ckks_asym.h"
#include "ckks_common.h"
#include "ckks_sym.h"
#include "ckks_tests_common.h"
#include "defines.h"
#include "ntt.h"
#include "polymodarith.h"
#include "sample.h"
#include "test_common.h"
#include "util_print.h"

#if !((defined(SE_ON_SPHERE_M4) || defined(SE_ON_NRF5)) && defined(SE_ENCRYPT_TYPE_SYMMETRIC))
/**
Asymmetric test core. If debugging is enabled, throws an error if a test fails.

@param[in] n             Polynomial ring degree (ignored if SE_USE_MALLOC is defined)
@param[in] nprimes       # of modulus primes    (ignored if SE_USE_MALLOC is defined)
@param[in] test_message  If 0, sets the message 0
*/
void test_ckks_asym_base(size_t n, size_t nprimes, bool test_message)
{
#ifndef SE_USE_MALLOC
    se_assert(n == SE_DEGREE_N && nprimes == SE_NPRIMES);  // sanity check
    if (n != SE_DEGREE_N) n = SE_DEGREE_N;
    if (nprimes != SE_NPRIMES) nprimes = SE_NPRIMES;
#endif

    Parms parms;
    parms.sample_s      = false;
    parms.is_asymmetric = true;
    parms.pk_from_file  = false;  // must be false to check on device
    parms.small_s       = true;   // try this first
    parms.small_u       = true;   // try this first
    bool encode_only    = false;

    // -- Make sure we didn't set this accidentally
    if (!parms.sample_s) se_assert(parms.small_s);

#ifdef SE_USE_MALLOC
    print_ckks_mempool_size(n, 0);
    ZZ *mempool = ckks_mempool_setup_asym(n);
#else
    print_ckks_mempool_size();
    ZZ mempool_local[MEMPOOL_SIZE];
    memset(&mempool_local, 0, MEMPOOL_SIZE * sizeof(ZZ));
    ZZ *mempool = &(mempool_local[0]);
#endif

    // -- Get pointers
    SE_PTRS se_ptrs_local;
    ckks_set_ptrs_asym(n, mempool, &se_ptrs_local);
    double complex *conj_vals  = se_ptrs_local.conj_vals;
    int64_t *conj_vals_int     = se_ptrs_local.conj_vals_int_ptr;
    double complex *ifft_roots = se_ptrs_local.ifft_roots;
    ZZ *pk_c0                  = se_ptrs_local.c0_ptr;
    ZZ *pk_c1                  = se_ptrs_local.c1_ptr;
    uint16_t *index_map        = se_ptrs_local.index_map_ptr;
    ZZ *ntt_roots              = se_ptrs_local.ntt_roots_ptr;
    ZZ *ntt_u_e1_pte           = se_ptrs_local.ntt_pte_ptr;
    ZZ *u                      = se_ptrs_local.ternary;
    flpt *v                    = se_ptrs_local.values;
    int8_t *e1                 = se_ptrs_local.e1_ptr;
    size_t vlen                = n / 2;

    if (!test_message)
    {
        memset(v, 0, vlen * sizeof(flpt));
        // v = 0;
    }

    // -- Additional pointers required for testing.
#ifdef SE_USE_MALLOC
    ZZ *s            = calloc(n / 16, sizeof(ZZ));
    int8_t *ep_small = calloc(n, sizeof(int8_t));
    ZZ *ntt_s_save   = calloc(n, sizeof(ZZ));  // ntt(expanded(s)) or expanded(s)
    printf("            s addr: %p\n", s);
    printf("     ep_small addr: %p\n", ep_small);
    printf("   ntt_s_save addr: %p\n", ntt_s_save);
#else
    ZZ s_vec[SE_DEGREE_N / 16];
    int8_t ep_small_vec[SE_DEGREE_N];
    ZZ ntt_s_save_vec[SE_DEGREE_N];  // ntt(expanded(s)) or expanded(s)
    memset(&ep_small_vec, 0, SE_DEGREE_N * sizeof(ZZ) / 16);
    memset(&ntt_s_save_vec, 0, SE_DEGREE_N * sizeof(ZZ) / 16);
    ZZ *s            = &(s_vec[0]);
    int8_t *ep_small = &(ep_small_vec[0]);
    ZZ *ntt_s_save   = &(ntt_s_save_vec[0]);  // ntt(expanded(s)) or expanded(s)
#endif

#if defined(SE_USE_MALLOC) && !(defined(SE_ON_NRF5) || defined(SE_ON_SPHERE_M4))
    // -- This is too much memory for the NRF5, so just allocate as needed later in that case
    ZZ *ntt_ep_save   = calloc(n, sizeof(ZZ));  // ntt(reduced(ep)) or reduced(ep)
    ZZ *ntt_e1_save   = calloc(n, sizeof(ZZ));
    ZZ *ntt_u_save    = calloc(n, sizeof(ZZ));
    ZZ *temp_test_mem = calloc(4 * n, sizeof(ZZ));
    printf("  ntt_ep_save addr: %p\n", ntt_ep_save);
    printf("  ntt_e1_save addr: %p\n", ntt_e1_save);
    printf("   ntt_u_save addr: %p\n", ntt_u_save);
    printf("temp_test_mem addr: %p\n", temp_test_mem);
#elif !defined(SE_USE_MALLOC)
    ZZ ntt_ep_save_vec[SE_DEGREE_N];          // ntt(reduced(ep)) or reduced(ep)
    ZZ ntt_e1_save_vec[SE_DEGREE_N];
    ZZ ntt_u_save_vec[SE_DEGREE_N];
    ZZ temp_test_mem_vec[4 * SE_DEGREE_N];
    memset(&ntt_ep_save_vec, 0, SE_DEGREE_N * sizeof(ZZ));
    memset(&ntt_e1_save_vec, 0, SE_DEGREE_N * sizeof(ZZ));
    memset(&ntt_u_save_vec, 0, SE_DEGREE_N * sizeof(ZZ));
    memset(&temp_test_mem_vec, 0, 4 * SE_DEGREE_N * sizeof(ZZ));
    ZZ *ntt_ep_save   = &(ntt_ep_save_vec[0]);  // ntt(reduced(ep)) or reduced(ep)
    ZZ *ntt_e1_save   = &(ntt_e1_save_vec[0]);
    ZZ *ntt_u_save    = &(ntt_u_save_vec[0]);
    ZZ *temp_test_mem = &(temp_test_mem_vec[0]);
#endif

    SE_PRNG prng;
    SE_PRNG shareable_prng;

    // -- Set up parameters and index_map if applicable
    ckks_setup(n, nprimes, index_map, &parms);

    print_test_banner("Asymmetric Encryption", &parms);

    // -- If s is allocated space ahead of time, can load ahead of time too.
    // -- (If we are testing and sample s is set, this will also sample s)
    ckks_setup_s(&parms, NULL, &prng, s);
    size_t s_size = parms.small_s ? n / 16 : n;
    if (encode_only) clear(s, s_size);

    for (size_t testnum = 0; testnum < 9; testnum++)
    {
        printf("-------------------- Test %zu -----------------------\n", testnum);
        ckks_reset_primes(&parms);

        if (test_message)
        {
            // -- Set test values
            set_encode_encrypt_test(testnum, vlen, v);
            print_poly_flpt("v        ", v, vlen);
        }

        // ------------------------------------------
        // ----- Begin encode-encrypt sequence ------
        // ------------------------------------------

        // -- First, encode
        if (test_message)
        {
            bool ret = ckks_encode_base(&parms, v, vlen, index_map, ifft_roots, conj_vals);
            se_assert(ret);
        }
        else
            memset(conj_vals_int, 0, n * sizeof(conj_vals_int[0]));
        // print_poly_int64("conj_vals_int      ", conj_vals_int, n);
        se_assert(v);

        // -- Sample u, ep, e0, e1. While sampling e0, add in-place to base message.
        if (!encode_only)
        {
            // -- Sample ep here for generating the public key later
#ifdef SE_DEBUG_NO_ERRORS
            memset(ep_small, 0, n * sizeof(int8_t));
#else
            sample_poly_cbd_generic_prng_16(n, &prng, ep_small);
#endif
            // print_poly_int8_full("ep_small", ep_small, n);
            // printf("About to init\n");
            ckks_asym_init(&parms, NULL, &prng, conj_vals_int, u, e1);
            // printf("Back from init\n");
        }

        // -- Debugging
        // size_t u_size = parms.small_u ? n/16 : n;
        // memset(u, 0, u_size * sizeof(ZZ));
        // if (parms.small_u) set_small_poly_idx(0, 1, u);
        // else				  u[0] = 1;

        print_poly_ternary("u   ", u, n, true);
        print_poly_ternary("s   ", s, n, true);
        // print_poly_int8_full("e1  ", e1_ptr, n);

        for (size_t i = 0; i < parms.nprimes; i++)
        {
            const Modulus *mod = parms.curr_modulus;
            print_zz(" ***** Modulus", mod->value);

#ifdef SE_ON_NRF5
            ZZ *ntt_ep_save = calloc(n, sizeof(ZZ));  // ntt(reduced(ep)) or reduced(ep)
            ZZ *ntt_e1_save = calloc(n, sizeof(ZZ));
            ZZ *ntt_u_save  = calloc(n, sizeof(ZZ));
#endif
            // -- We can't just load pk if we are testing because we need ep to
            //    check the "decryption". Need to generate pk here instead to keep
            //    track of the secret key error term.
            se_assert(!parms.pk_from_file);
            printf("generating pk...\n");
            gen_pk(&parms, s, ntt_roots, NULL, &shareable_prng, ntt_s_save, ep_small, ntt_ep_save,
                   pk_c0, pk_c1);
            printf("...done generating pk.\n");

            // -- Debugging
            print_poly("pk0 ", pk_c0, n);
            print_poly("pk1 ", pk_c1, n);
            print_poly_ternary("u   ", u, n, 1);
            // print_poly_int8_full("e1  ", e1_ptr, n);
            // if (ntt_e1_ptr) print_poly_full("ntt e1  ", ntt_e1_ptr, n);

            // -- Per prime Encode + Encrypt
            ckks_encode_encrypt_asym(&parms, conj_vals_int, u, e1, ntt_roots, ntt_u_e1_pte,
                                     ntt_u_save, ntt_e1_save, pk_c0, pk_c1);
            print_poly_int64("conj_vals_int      ", conj_vals_int, n);

            // -- Debugging
            // print_poly_int8_full("e1  ", e1, n);
            // if (ntt_e1_ptr) print_poly_full("ntt e1  ", ntt_e1, n);
            print_poly_ternary("u   ", u, n, 1);
            // if (ntt_u_save) print_poly("ntt_u_save   ", ntt_u_save, n);

            // -- Decryption does the following:
            //    (c1 = pk1*u + e1)*s + (c0 = pk0*u + e0 + m)  -->
            //    ((pk1 = c1 = a)*u + e1)*s + ((pk0 = c1 = -a*s+ep)*u + e0 + m) -->
            //    a*u*s + e1*s -a*s*u + ep*u + e0 ===> e1*s + ep*u + e0 + m
            print_poly("c0      ", pk_c0, n);
            print_poly("c1      ", pk_c1, n);
            print_poly("ntt(u)  ", ntt_u_save, n);
            print_poly("ntt(ep) ", ntt_ep_save, n);
            poly_mult_mod_ntt_form_inpl(ntt_u_save, ntt_ep_save, n, mod);
            print_poly("ntt(u) * ntt(ep)", ntt_u_save, n);

            print_poly("ntt(s)  ", ntt_s_save, n);
            print_poly("ntt(e1) ", ntt_e1_save, n);
            poly_mult_mod_ntt_form_inpl(ntt_e1_save, ntt_s_save, n, mod);
            print_poly("ntt(s) * ntt(e1)", ntt_e1_save, n);

            print_poly("ntt(u) * ntt(ep)", ntt_u_save, n);
            poly_add_mod_inpl(ntt_u_save, ntt_e1_save, n, mod);
            print_poly("ntt(u) * ntt(ep) + ntt(s) * ntt(e1)", ntt_u_save, n);

            print_poly("ntt(m + e0)", ntt_u_e1_pte, n);
            poly_add_mod_inpl(ntt_u_e1_pte, ntt_u_save, n, mod);
            print_poly("ntt(u) * ntt(ep) + ntt(s) * ntt(e1) + ntt(m + e0)", ntt_u_e1_pte, n);
            ZZ *pterr = ntt_u_e1_pte;

#ifdef SE_ON_NRF5
            if (ntt_ep_save)
            {
                free(ntt_ep_save);
                ntt_ep_save = 0;
            }
            if (ntt_e1_save)
            {
                free(ntt_e1_save);
                ntt_e1_save = 0;
            }
            if (ntt_u_save)
            {
                free(ntt_u_save);
                ntt_u_save = 0;
            }
            ZZ *temp_test_mem = calloc(4 * n, sizeof(ZZ));
#endif

            // -- Check that decrypt gives back the pt+err and decode gives back v.
            // -- Note: This will only decode if values is non-zero. Otherwise, will just decrypt.
            // -- Note: sizeof(max(ntt_roots, ifft_roots)) must be passed as temp memory to undo
            //    ifft.
            bool s_test_save_small = 0;
            check_decode_decrypt_inpl(pk_c0, pk_c1, v, vlen, ntt_s_save, s_test_save_small, pterr,
                                      index_map, &parms, temp_test_mem);

            // -- Done checking this prime, now try next prime if requested
            // -- Note: This does nothing to u if u is in small form
            bool ret = ckks_next_prime_asym(&parms, u);
            se_assert(ret || (!ret && i + 1 == parms.nprimes));

#ifdef SE_ON_NRF5
            if (temp_test_mem)
            {
                free(temp_test_mem);
                temp_test_mem = 0;
            }
#endif
        }

        // -- Can exit now if rlwe testing only
        if (!test_message) break;
    }

#ifdef SE_USE_MALLOC
    //clang-format off
    if (mempool)
    {
        free(mempool);
        mempool = 0;
    }
    if (s)
    {
        free(s);
        s = 0;
    }
    if (ep_small)
    {
        free(ep_small);
        ep_small = 0;
    }
    if (ntt_s_save)
    {
        free(ntt_s_save);
        ntt_s_save = 0;
    }
    //clang-format on
#if !(defined(SE_ON_NRF5) || defined(SE_ON_SPHERE_M4))
    //clang-format off
    if (ntt_ep_save)
    {
        free(ntt_ep_save);
        ntt_ep_save = 0;
    }
    if (ntt_e1_save)
    {
        free(ntt_e1_save);
        ntt_e1_save = 0;
    }
    if (ntt_u_save)
    {
        free(ntt_u_save);
        ntt_u_save = 0;
    }
    if (temp_test_mem)
    {
        free(temp_test_mem);
        temp_test_mem = 0;
    }
    //clang-format on
#endif
#endif
    delete_parameters(&parms);
}

/**
Full encode + asymmetric encrypt test

@param[in] n        Polynomial ring degree (ignored if SE_USE_MALLOC is defined)
@param[in] nprimes  # of modulus primes    (ignored if SE_USE_MALLOC is defined)
*/
void test_ckks_encode_encrypt_asym(size_t n, size_t nprimes)
{
    printf("Beginning tests for ckks encode + asymmetric encrypt...\n");
    bool test_message = 1;
#ifdef SE_USE_MALLOC
    test_ckks_asym_base(n, nprimes, test_message);
#else
    SE_UNUSED(n);
    SE_UNUSED(nprimes);
    test_ckks_asym_base(SE_DEGREE_N, SE_NPRIMES, test_message);
#endif
}

/**
Asymmetric rlwe test only (message is the all-zeros vector)

@param[in] n        Polynomial ring degree (ignored if SE_USE_MALLOC is defined)
@param[in] nprimes  # of modulus primes    (ignored if SE_USE_MALLOC is defined)
*/
void test_enc_zero_asym(size_t n, size_t nprimes)
{
    printf("Beginning tests for rlwe asymmetric encryption of 0...\n");
    bool test_message = 0;
#ifdef SE_USE_MALLOC
    test_ckks_asym_base(n, nprimes, test_message);
#else
    SE_UNUSED(n);
    SE_UNUSED(nprimes);
    test_ckks_asym_base(SE_DEGREE_N, SE_NPRIMES, test_message);
#endif
}
#else
void test_ckks_encode_encrypt_asym(void)
{
    printf("Error! Did you choose the wrong configuration settings?\n");
    se_assert(0);
}

void test_enc_zero_asym(void)
{
    printf("Error! Did you choose the wrong configuration settings?\n");
    se_assert(0);
}
#endif

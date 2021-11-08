// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_tests_encode.c
*/

#include <math.h>  // pow
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

/**
Test CKKS encoding

@param[in] n Polynomial ring degree (ignored if SE_USE_MALLOC is defined)
*/
void test_ckks_encode(size_t n)
{
#ifndef SE_USE_MALLOC
    se_assert(n == SE_DEGREE_N);  // sanity check
    if (n != SE_DEGREE_N) n = SE_DEGREE_N;
#endif

    Parms parms;
#ifdef SE_USE_MALLOC
    print_ckks_mempool_size(n, 1);
    ZZ *mempool = ckks_mempool_setup_sym(n);
#else
    print_ckks_mempool_size();
    ZZ mempool_local[MEMPOOL_SIZE];
    memset(&(mempool_local), 0, MEMPOOL_SIZE * sizeof(ZZ));
    ZZ *mempool = &(mempool_local[0]);
#endif
    // TODO: Make a malloc function to include test memory

    // -- Get pointers
    SE_PTRS se_ptrs_local;
    ckks_set_ptrs_sym(n, mempool, &se_ptrs_local);
    double complex *conj_vals  = se_ptrs_local.conj_vals;
    int64_t *conj_vals_int     = se_ptrs_local.conj_vals_int_ptr;
    double complex *ifft_roots = se_ptrs_local.ifft_roots;
    uint16_t *index_map        = se_ptrs_local.index_map_ptr;
    // -- Note that, since we are not adding the error, the pt will not be in NTT form.
    ZZ *pt      = se_ptrs_local.ntt_pte_ptr;
    flpt *v     = se_ptrs_local.values;
    size_t vlen = n / 2;

    // -- Additional pointers required for testing.
#ifdef SE_USE_MALLOC
    ZZ *temp = calloc(n, sizeof(double complex));
#else
    ZZ temp[SE_DEGREE_N * sizeof(double complex) / sizeof(ZZ)];
    memset(&temp, 0, SE_DEGREE_N * sizeof(double complex));
#endif

    // -- Set up parameters and index_map if applicable
    ckks_setup(n, 1, index_map, &parms);
    print_test_banner("Encode", &parms);

    for (size_t testnum = 0; testnum < 9; testnum++)
    {
        printf("-------------------- Test %zu -----------------------\n", testnum);
        const Modulus *mod = parms.curr_modulus;
        print_zz("\n ***** Modulus", mod->value);

        // -- Get test values
        set_encode_encrypt_test(testnum, vlen, v);
        print_poly_flpt("v        ", v, vlen);

        // -- Begin encode-encrypt sequence
        // -- First, encode base. Afer this, we should only use the pointer values
        bool ret = ckks_encode_base(&parms, v, vlen, index_map, ifft_roots, conj_vals);
        se_assert(ret);
        // print_poly_int64_full("conj_vals_int      ", conj_vals_int, n);

        // -- Now, need to reduce the plaintext
        reduce_set_pte(&parms, conj_vals_int, pt);
        // print_poly_int64_full("conj_vals_int      ", conj_vals_int, n);

        // -- Check that decoding works
        check_decode_inpl(pt, v, vlen, index_map, &parms, temp);
    }
#ifdef SE_USE_MALLOC
    // clang-format off
    if (mempool) { free(mempool); mempool = 0; }
    if (temp)    { free(temp);    temp    = 0; }
    // clang-format on
#endif
    delete_parameters(&parms);
}

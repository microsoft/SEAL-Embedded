// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file api_tests.c
*/

#include <stdbool.h>
#include <stdio.h>

#include "ckks_tests_common.h"
#include "defines.h"
#include "seal_embedded.h"
#include "test_common.h"
#include "util_print.h"

// -- Comment out to run true test
// #define SE_API_TESTS_DEBUG

/**
Function to print ciphertext values with the same function signature as SEND_FNCT_PTR.
Used in place of a networking function for testing.

Size req: v must be at least vlen_bytes

@param[in] v           Input polynomial (ciphertext) to be printed
@param[in] vlen_bytes  Number of bytes of v to print
@returns               Then number of bytes of v that were printed (equal to vlen_bytes on success)
*/
size_t test_print_ciphertexts(void *v, size_t vlen_bytes)
{
    static int idx   = 0;
    size_t vlen      = vlen_bytes / sizeof(ZZ);
    const char *name = idx ? "c1" : "c0";
#ifdef SE_API_TESTS_DEBUG
    print_poly(name, (ZZ *)v, vlen);
#else
    print_poly_full(name, (ZZ *)v, vlen);
#endif
    idx = idx ? 0 : 1;
    return vlen_bytes;
}

/**
Helper function to tests the default SEAL-Embedded API for by printing values of a ciphertext.
If SE_DISABLE_TESTING_CAPABILITY is not defined, throws an error on failure.
On success, resulting output can be verified with the SEAL-Embedded adapter.
*/
void test_ckks_api_base(SE_PARMS *se_parms)
{
    se_assert(se_parms);
    se_assert(se_parms->parms);
    se_assert(se_parms->se_ptrs);

    SEND_FNCT_PTR fake_network_func = (void *)&test_print_ciphertexts;

    size_t vlen = se_parms->parms->coeff_count / 2;
#ifdef SE_USE_MALLOC
    flpt *v = calloc(vlen, sizeof(flpt));
#else
    flpt v[SE_DEGREE_N / 2];
    memset(&v, 0, SE_DEGREE_N * sizeof(flpt) / 2);
    se_assert(SE_DEGREE_N / 2 == se_parms->parms->coeff_count / 2);
#endif

    for (size_t testnum = 0; testnum < 9; testnum++)
    {
        // -- Set up buffer v with testing values
        set_encode_encrypt_test(testnum, vlen, &(v[0]));

        // -- Print the expected output. This will help the adapter with verification.
#ifdef SE_API_TESTS_DEBUG
        print_poly_flpt("v (cleartext)", v, vlen);
#else
        print_poly_flpt_full("v (cleartext)", v, vlen);
#endif

        // -- Call the main encryption function!
        /*
        // -- Seed for testing
        uint8_t share_seed[SE_PRNG_SEED_BYTE_COUNT];
        uint8_t       seed[SE_PRNG_SEED_BYTE_COUNT];
        memset(&(share_seed[0]), 0, SE_PRNG_SEED_BYTE_COUNT);
        memset(&(      seed[0]), 0, SE_PRNG_SEED_BYTE_COUNT);
        share_seed[0] = 1;
              seed[0] = 1;
        bool ret = se_encrypt_seeded(share_seed, seed, fake_network_func, &(v[0]), vlen *
        sizeof(flpt), false, se_parms); se_assert(ret);
        */
        se_encrypt(fake_network_func, &(v[0]), vlen * sizeof(flpt), false, se_parms);
    }
#ifdef SE_USE_MALLOC
    if (v)
    {
        free(v);
        v = 0;
    }
#endif
    delete_parameters(se_parms->parms);
}

/**
Tests the default SEAL-Embedded API for symmetric encryption by printing values of a ciphertext.
If SE_DISABLE_TESTING_CAPABILITY is not defined, throws an error on failure.
On success, resulting output can be verified with the SEAL-Embedded adapter.
*/
void test_ckks_api_sym(void)
{
    printf("Beginning tests for ckks api symmetric encrypt...\n");
#ifndef SE_USE_MALLOC
    static ZZ mempool_local[MEMPOOL_SIZE];
    *mempool_ptr_global = &(mempool_local[0]);
    // -- Debugging
    // printf("Address of mempool_local: %p\n", &(mempool_local[0]));
    // printf("Address of mempool_ptr_global: %p\n", &mempool_ptr_global);
    // printf("Value of mempool_ptr_global (should be address of mempool_local): %p\n",
    // mempool_ptr_global);
#endif
    SE_PARMS *se_parms = se_setup_default(SE_SYM_ENCR);
    print_test_banner("Symmetric Encryption (API)", se_parms->parms);
    test_ckks_api_base(se_parms);
}

/**
Tests the default SEAL-Embedded API for asymmetric encryption by printing values of a ciphertext.
If SE_DISABLE_TESTING_CAPABILITY is not defined, throws an error on failure.
On success, resulting output can be verified with the SEAL-Embedded adapter.
*/
void test_ckks_api_asym(void)
{
    printf("Beginning tests for ckks api asymmetric encrypt...\n");
#ifndef SE_USE_MALLOC
    static ZZ mempool_local[MEMPOOL_SIZE];
    *mempool_ptr_global = &(mempool_local[0]);
    // -- Debugging
    // printf("Address of mempool_local: %p\n", &(mempool_local[0]));
    // printf("Address of mempool_ptr_global: %p\n", &mempool_ptr_global);
    // printf("Value of mempool_ptr_global (should be address of mempool_local): %p\n",
    // mempool_ptr_global);
#endif
    SE_PARMS *se_parms = se_setup_default(SE_ASYM_ENCR);
    print_test_banner("Asymmetric Encryption (API)", se_parms->parms);
    test_ckks_api_base(se_parms);
}

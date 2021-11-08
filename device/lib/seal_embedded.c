// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file seal_embedded.c
*/

#include "seal_embedded.h"

#include "ckks_asym.h"
#include "ckks_common.h"
#include "ckks_sym.h"
#include "defines.h"
#include "fileops.h"
#include "parameters.h"
#include "util_print.h"

static Parms se_encr_params_global;
static SE_PTRS se_ptrs_global;
static SE_PARMS se_parms_global;
static SE_PRNG se_shareable_prng_global;
static SE_PRNG se_prng_global;

SE_PARMS *se_setup_custom(size_t degree, size_t nprimes, const ZZ *modulus_vals, const ZZ *ratios,
                          double scale, EncryptType encrypt_type)
{
    SE_PARMS *se_parms = &se_parms_global;
    Parms *parms       = &se_encr_params_global;
    SE_PTRS *se_ptrs   = &se_ptrs_global;

    se_assert(se_parms && parms && se_ptrs);
    se_parms->parms   = parms;
    se_parms->se_ptrs = se_ptrs;

    size_t n             = degree;
    parms->scale         = scale;
    parms->is_asymmetric = (encrypt_type == SE_ASYM_ENCR);
    parms->pk_from_file  = 1;
    parms->sample_s      = 0;
    parms->small_u       = 1;
    parms->small_s       = 1;
    se_parms->parms      = parms;

#ifdef SE_USE_MALLOC
    ZZ *mempool;
    if (encrypt_type == SE_ASYM_ENCR)
    {
        print_ckks_mempool_size(n, 0);
        mempool = ckks_mempool_setup_asym(n);
    }
    else
    {
        print_ckks_mempool_size(n, 1);
        mempool = ckks_mempool_setup_sym(n);
    }
    se_assert(mempool);
#else
    print_ckks_mempool_size();
    printf("In se_setup_custom, no malloc\n");
    // ZZ *mempool = &(mempool_ptr_global[0]);
    ZZ *mempool = *mempool_ptr_global;
    printf("Address of mempool_ptr_global: %p\n", &mempool_ptr_global);
    printf("Value of mempool_ptr_global (should be address of mempool_local): %p\n",
           mempool_ptr_global);
    printf("Value of mempool (should be equal to value of mempool_ptr_global): %p\n", mempool);
#endif

    if (encrypt_type == SE_ASYM_ENCR) { ckks_set_ptrs_asym(n, mempool, se_ptrs); }
    else
    {
        ckks_set_ptrs_sym(n, mempool, se_ptrs);
    }

    if (!modulus_vals || !ratios) { ckks_setup(n, nprimes, se_ptrs->index_map_ptr, parms); }
    else
    {
        ckks_setup_custom(n, nprimes, modulus_vals, ratios, se_ptrs->index_map_ptr, parms);
    }

    if (encrypt_type == SE_SYM_ENCR) { ckks_setup_s(parms, NULL, NULL, se_ptrs->ternary); }

    return se_parms;
}

SE_PARMS *se_setup(size_t degree, size_t nprimes, double scale, EncryptType encrypt_type)
{
    return se_setup_custom(degree, nprimes, NULL, NULL, scale, encrypt_type);
}

SE_PARMS *se_setup_default(EncryptType encrypt_type)
{
    double scale = pow(2, 25);
    // double scale = pow(2, 35);
    // double scale = pow(2, 40);
    return se_setup(4096, 3, scale, encrypt_type);
}

bool se_encrypt_seeded(uint8_t *shareable_seed, uint8_t *seed, SEND_FNCT_PTR network_send_function,
                       void *v, size_t vlen_bytes, bool print, SE_PARMS *se_parms)
{
    se_assert(se_parms);
    se_assert(se_parms && se_parms->se_ptrs);
    se_assert(se_parms->parms && se_parms->se_ptrs->values);
    Parms *parms     = se_parms->parms;
    SE_PTRS *se_ptrs = se_parms->se_ptrs;
    size_t n         = parms->coeff_count;

    size_t copy_size_bytes = (n / 2) * sizeof(ZZ);
    if (vlen_bytes < copy_size_bytes) copy_size_bytes = vlen_bytes;
    memset(se_ptrs->values, 0, copy_size_bytes);
    memcpy(se_ptrs->values, v, copy_size_bytes);

    ckks_reset_primes(parms);

    bool ret = ckks_encode_base(parms, se_ptrs->values, n / 2, se_ptrs->index_map_ptr,
                                se_ptrs->ifft_roots, se_ptrs->conj_vals);
    se_assert(ret);
    if (!ret) return ret;
    // -- Debugging
    // print_poly_int64("pt, reg", se_ptrs->conj_vals_int_ptr, n);

    // -- Uncomment this line and the similar line below to check against
    //    expected values with adapter
    // print_poly_int64_full("pt, reg", se_ptrs->conj_vals_int_ptr, n);

    if (parms->is_asymmetric)
    {
        ckks_asym_init(parms, seed, &se_prng_global, se_ptrs->conj_vals_int_ptr, se_ptrs->ternary,
                       se_ptrs->e1_ptr);
        load_pki(0, parms, se_ptrs->c0_ptr);
        load_pki(1, parms, se_ptrs->c1_ptr);
    }
    else
    {
        ckks_sym_init(parms, shareable_seed, seed, &se_shareable_prng_global, &se_prng_global,
                      se_ptrs->conj_vals_int_ptr);
    }
    // -- Debugging
    // print_poly_int64("pte, reg", se_ptrs->conj_vals_int_ptr, n);

    // -- Uncomment this line and the similar line above to check against
    //    expected values with adapter
    // print_poly_int64_full("pte, reg", se_ptrs->conj_vals_int_ptr, n);

    for (size_t i = 0; i < parms->nprimes; i++)
    {
        if (parms->is_asymmetric)
        {
            ckks_encode_encrypt_asym(parms, se_ptrs->conj_vals_int_ptr, se_ptrs->ternary,
                                     se_ptrs->e1_ptr, se_ptrs->ntt_roots_ptr, se_ptrs->ntt_pte_ptr,
                                     NULL, NULL, se_ptrs->c0_ptr, se_ptrs->c1_ptr);
        }
        else
        {
            ckks_encode_encrypt_sym(parms, se_ptrs->conj_vals_int_ptr, NULL,
                                    &se_shareable_prng_global, se_ptrs->ternary,
                                    se_ptrs->ntt_pte_ptr, se_ptrs->ntt_roots_ptr, se_ptrs->c0_ptr,
                                    se_ptrs->c1_ptr, NULL, NULL);
        }

        if (print)
        {
            print_poly("c0: ", se_ptrs->c0_ptr, n);
            print_poly("c1: ", se_ptrs->c1_ptr, n);
        }

#ifndef SE_DISABLE_TESTING_CAPABILITY
#ifndef SE_REVERSE_CT_GEN_ENABLED
        // -- Sanity check
        se_assert(se_parms->parms->curr_modulus_idx == i);
#endif
        // -- Sanity checks
        for (size_t i = 0; i < n; i++)
        {
            se_assert((se_ptrs->c0_ptr)[i] < se_parms->parms->curr_modulus->value);
            se_assert((se_ptrs->c1_ptr)[i] < se_parms->parms->curr_modulus->value);
        }
#endif

        if (network_send_function)
        {
            size_t nbytes_send, nbytes_recv;

            // TODO: Finish this feature, add to defines
#ifdef SE_ENABLE_SYM_SEED_CT
            if (!parms->is_asymmetric)
            {
                nbytes_send = SE_PRNG_SEED_BYTE_COUNT;
                nbytes_recv =
                    network_send_function(&(se_shareable_prng_global.seed[0]), nbytes_send);
                se_assert(nbytes_recv == nbytes_send);
            }
            else
#endif
            {
                nbytes_send = n * sizeof(ZZ);
                nbytes_recv = network_send_function(se_ptrs->c0_ptr, nbytes_send);
                se_assert(nbytes_recv == nbytes_send);
            }

            nbytes_send = n * sizeof(ZZ);
            nbytes_recv = network_send_function(se_ptrs->c1_ptr, nbytes_send);
            se_assert(nbytes_recv == nbytes_send);
        }

        if ((i + 1) < parms->nprimes)
        {
            if (parms->is_asymmetric)
                ckks_next_prime_asym(parms, se_ptrs->ternary);
            else
                ckks_next_prime_sym(parms, se_ptrs->ternary);
        }
    }
    return true;
}

bool se_encrypt(SEND_FNCT_PTR network_send_function, void *v, size_t vlen_bytes, bool print,
                SE_PARMS *se_parms)
{
    return se_encrypt_seeded(NULL, NULL, network_send_function, v, vlen_bytes, print, se_parms);
}

void se_cleanup(SE_PARMS *se_parms)
{
    se_assert(se_parms);
#ifdef SE_USE_MALLOC
    se_assert(se_parms->se_ptrs);
    delete_parameters(se_parms->parms);

    // -- Free the memory pool. Note that conj_vals should always point to the start.
    // -- TODO: Make this better
    if (se_parms->se_ptrs->conj_vals) free(se_parms->se_ptrs->conj_vals);
#endif
    se_parms->parms = 0;
}

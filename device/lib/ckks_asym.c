// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_asym.c
*/

#include "ckks_asym.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // memset

#include "ckks_common.h"
#include "ckks_sym.h"
#include "defines.h"
#include "fft.h"
#include "fileops.h"
#include "modulo.h"
#include "ntt.h"
#include "parameters.h"
#include "polymodarith.h"
#include "polymodmult.h"
#include "sample.h"
#include "uintmodarith.h"
#include "util_print.h"

#ifdef SE_USE_MALLOC
size_t ckks_get_mempool_size_asym(size_t degree)
{
    se_assert(degree >= 16);
    if (degree == SE_DEGREE_N) return MEMPOOL_SIZE_Asym;
    size_t n            = degree;
    size_t mempool_size = 4 * n;  // base

#ifdef SE_IFFT_OTF
    mempool_size += n + n / 4 + n / 16;
#if defined(SE_NTT_ONE_SHOT) || defined(SE_NTT_REG)
    mempool_size += n;
#elif defined(SE_NTT_FAST)
    mempool_size += 2 * n;
#endif
#else
    mempool_size += 4 * n;
#endif

#if defined(SE_INDEX_MAP_PERSIST) || defined(SE_INDEX_MAP_LOAD_PERSIST)
    mempool_size += n / 2;
#endif

#ifdef SE_MEMPOOL_ALLOC_VALUES
    mempool_size += n / 2;
#endif

    se_assert(mempool_size);
    return mempool_size;
}

ZZ *ckks_mempool_setup_asym(size_t degree)
{
    size_t mempool_size = ckks_get_mempool_size_asym(degree);
    ZZ *mempool         = calloc(mempool_size, sizeof(ZZ));
    // printf("mempool_size: %zu\n", mempool_size);
    if (!mempool)
    {
        printf("Error! Allocation failed. Exiting...\n");
        exit(1);
    }
    se_assert(mempool_size && mempool);
    return mempool;
}
#endif

void ckks_set_ptrs_asym(size_t degree, ZZ *mempool, SE_PTRS *se_ptrs)
{
    se_assert(mempool && se_ptrs);
    size_t n = degree;

    // -- First, set everything to the set size or 0
    se_ptrs->conj_vals         = (double complex *)mempool;
    se_ptrs->conj_vals_int_ptr = (int64_t *)mempool;
    se_ptrs->c1_ptr            = &(mempool[2 * n]);
    se_ptrs->c0_ptr            = &(mempool[3 * n]);

    // -- In asymmetric mode, ntt_pte_ptr == ntt_u2pte_ptr
    se_ptrs->ternary       = &(mempool[5 * n + n / 4]);  // default: SE_INDEX_MAP_OTF
    se_ptrs->ifft_roots    = 0;                          // default: SE_IFFT_OTF
    se_ptrs->index_map_ptr = 0;                          // default: SE_INDEX_MAP_OTF
    se_ptrs->ntt_roots_ptr = 0;                          // default: SE_NTT_OTF
    se_ptrs->values        = 0;

    // -- Sizes
    size_t ifft_roots_size        = 0;
    size_t ntt_roots_size         = 0;
    size_t index_map_persist_size = 0;

    // -- Set ifft_roots based on IFFT type
#ifndef SE_IFFT_OTF
    ifft_roots_size     = 4 * n;
    se_ptrs->ifft_roots = (double complex *)&(mempool[4 * n]);
#endif

    // -- Set ntt_roots based on NTT type
#if defined(SE_NTT_ONE_SHOT) || defined(SE_NTT_REG)
    ntt_roots_size         = n;
    se_ptrs->ntt_roots_ptr = &(mempool[4 * n]);
#elif defined(SE_NTT_FAST)
    ntt_roots_size         = 2 * n;
    se_ptrs->ntt_roots_ptr = &(mempool[4 * n]);
#endif

    size_t total_block2_size = ifft_roots_size ? ifft_roots_size : (ntt_roots_size + n);

    // -- Set pi inverse based on index map type
#if defined(SE_INDEX_MAP_LOAD) || defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
    se_ptrs->index_map_ptr = (uint16_t *)&(mempool[4 * n]);
#elif defined(SE_INDEX_MAP_PERSIST) || defined(SE_INDEX_MAP_LOAD_PERSIST)
    // -- If ifft, this will be + the ifft_roots size
    //    else, this will be + the ntt size, + ntt(e1) size
    se_ptrs->index_map_ptr = (uint16_t *)&(mempool[4 * n + total_block2_size]);
    index_map_persist_size = n / 2;
#endif
    se_ptrs->ntt_pte_ptr = &(mempool[4 * n + ntt_roots_size]);

#ifdef SE_IFFT_OTF
    se_ptrs->e1_ptr  = (int8_t *)&(mempool[4 * n + ntt_roots_size + n + index_map_persist_size]);
    se_ptrs->ternary = &(mempool[4 * n + ntt_roots_size + n + index_map_persist_size + n / 4]);
#else
    se_ptrs->e1_ptr        = (int8_t *)&(mempool[4 * n + ntt_roots_size + n]);
    se_ptrs->ternary       = &(mempool[4 * n + ntt_roots_size + n + n / 4]);
#endif

#ifdef SE_MEMPOOL_ALLOC_VALUES
#ifdef SE_IFFT_OTF
    se_ptrs->values =
        (flpt *)&(mempool[4 * n + total_block2_size + index_map_persist_size + n / 4 + n / 16]);
#else
    se_ptrs->values = (flpt *)&(mempool[4 * n + total_block2_size + index_map_persist_size]);
#endif
#endif

    size_t address_size = 4;
    se_assert(((ZZ *)se_ptrs->conj_vals) == ((ZZ *)se_ptrs->conj_vals_int_ptr));
    se_assert(se_ptrs->c1_ptr ==
              (ZZ *)se_ptrs->conj_vals_int_ptr + 2 * n * sizeof(ZZ) / address_size);
    se_assert(se_ptrs->c0_ptr == se_ptrs->c1_ptr + n * sizeof(ZZ) / address_size);

    // -- Debugging: print all adresses
#ifdef SE_USE_MALLOC
    se_print_addresses(mempool, se_ptrs, n, 0);
    se_print_relative_positions(mempool, se_ptrs, n, 0);
#else
    se_print_addresses(mempool, se_ptrs);
    se_print_relative_positions(mempool, se_ptrs);
#endif
}

void gen_pk(const Parms *parms, ZZ *s_small, ZZ *ntt_roots, uint8_t *seed, SE_PRNG *shareable_prng,
            ZZ *s_save, int8_t *ep_small, ZZ *ntt_ep, ZZ *pk_c0, ZZ *pk_c1)
{
    se_assert(parms && shareable_prng);
    prng_randomize_reset(shareable_prng, seed);  // Safe to share this prng

    // -- pk_c0 := - [a . ntt(exp(s))] + ntt(ep) or  := - [a * s] + ep
    //    pk_c1 := a
    // -- If pk_c0 and pk_c1 point to the same location, pk0 will overwrite pk1.
    //    However, we need to return pk1. Therefore, must save it in extra buffer.
    ckks_encode_encrypt_sym(parms, 0, ep_small, shareable_prng, s_small, ntt_ep, ntt_roots, pk_c0,
                            pk_c1, s_save, 0);
}

void ckks_asym_init(const Parms *parms, uint8_t *seed, SE_PRNG *prng, int64_t *conj_vals_int, ZZ *u,
                    int8_t *e1)
{
    se_assert(parms && prng);
    size_t n = parms->coeff_count;

    // -- The prng should be reset & re-randomized once per encode-encrypt sequence.
    // -- (This is not strictly necessary, but it is more consistent.)
    // -- This call both resets the prng counter to 0 and the sets seed to a random value.
    // -- Note that this prng's seed value should not be shared.
    // printf("About to randomize reset\n");
    prng_randomize_reset(prng, seed);

    // -- Sample ternary polynomial u
    // printf("About to sample small poly ternary\n");
    if (parms->small_u) { sample_small_poly_ternary_prng_96(n, prng, u); }
    else
    {
        sample_poly_ternary(parms, prng, u);
    }
    // print_poly_ternary("u", u, n, parms->small_u);
    // printf("Back from sample small poly ternary\n");

    // -- Sample error polynomials e0 and e1
#ifdef SE_DEBUG_NO_ERRORS
    memset(e1, 0, n * sizeof(int8_t));
#else
    sample_add_poly_cbd_generic_inpl_prng_16(conj_vals_int, n, prng);  // now stores [pt+e0]
    sample_poly_cbd_generic_prng_16(n, prng, e1);                      // now stores [e1]
#endif
}

void ckks_encode_encrypt_asym(const Parms *parms, const int64_t *conj_vals_int, const ZZ *u,
                              const int8_t *e1, ZZ *ntt_roots, ZZ *ntt_u_e1_pte, ZZ *ntt_u_save,
                              ZZ *ntt_e1_save, ZZ *pk_c0, ZZ *pk_c1)
{
    se_assert(parms);
#ifdef SE_DISABLE_TESTING_CAPABILITY
    SE_UNUSED(ntt_u_save);
    SE_UNUSED(ntt_e1_save);
#endif

    // ===============================================================================
    //   Generate ciphertext: (c[1], c[0]) = ([pk1*u + e1]_Rq, [pk0*u + (m + e0)]_Rq)
    // ===============================================================================
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;

    // -------------------------
    //      Load pk1, pk0
    // -------------------------
    if (parms->pk_from_file)
    {
        load_pki(1, parms, pk_c1);
        load_pki(0, parms, pk_c0);
    }

    // -------------------------
    //  [pk1*u]_Rq, [pk0*u]_Rq
    // -------------------------
    // -- If u is in small form, expand so we can calculate ntt(u)
    // print_poly_ternary_full("u", u, n, 1);
    if (parms->small_u) expand_poly_ternary(u, parms, ntt_u_e1_pte);
    // print_poly_full("u reduced", ntt_u_e1_pte, n);

    // -- Initialize ntt roots
    ntt_roots_initialize(parms, ntt_roots);

    // -- Calculate ntt(u). If requested, save ntt(u) for testing later
    ntt_inpl(parms, ntt_roots, ntt_u_e1_pte);
    // print_poly("ntt(u) (inside)", ntt_u_e1_pte, n);

#ifndef SE_DISABLE_TESTING_CAPABILITY
    // if (ntt_u_save) printf("ntt u save is not null\n");
    // else            printf("ntt u save is null\n");
    if (ntt_u_save) memcpy(ntt_u_save, ntt_u_e1_pte, n * sizeof(ZZ));
        // if (ntt_u_save) print_poly("ntt(u) (inside, ntt_u_save)", ntt_u_save, n);
#endif

    // -- Calculate [ntt(pk1) . ntt(u)]_Rq. Store result in pk_c1
    poly_mult_mod_ntt_form_inpl(pk_c1, ntt_u_e1_pte, n, mod);
    // print_poly("pk1*u (ntt)", pk_c1, n);

    // -- Calculate [ntt(pk0) . ntt(u)]_Rq. Store result in pk_c0
    poly_mult_mod_ntt_form_inpl(pk_c0, ntt_u_e1_pte, n, mod);
    // print_poly("pk0*u (ntt)", pk_c0, n);

    // -------------------------
    //      [pk1*u + e1]_Rq
    // -------------------------
    reduce_set_e_small(parms, e1, ntt_u_e1_pte);
    // print_poly("e1 reduced", ntt_u_e1_pte, n);
    ntt_inpl(parms, ntt_roots, ntt_u_e1_pte);

#ifndef SE_DISABLE_TESTING_CAPABILITY
    if (ntt_e1_save) memcpy(ntt_e1_save, ntt_u_e1_pte, n * sizeof(ntt_u_e1_pte[0]));
#endif

    // print_poly("ntt(e1)", ntt_u_e1_pte, n);
    poly_add_mod_inpl(pk_c1, ntt_u_e1_pte, n, mod);
    // print_poly("c1 = pk1*u + e1 (ntt)", pk_c1, n);

    // -----------------------------
    //      [pk0*u + m + e0]_Rq
    // -----------------------------
    // -- We no longer need ntt_u, but we do need ntt((m + e0) = conj_vals_in)
    // print_poly_int64("conj_vals_int      ", conj_vals_int, n);
    reduce_set_pte(parms, conj_vals_int, ntt_u_e1_pte);
    // print_poly("m+e0 reduced", ntt_u_e1_pte, n);
    ntt_inpl(parms, ntt_roots, ntt_u_e1_pte);
    // print_poly("ntt(m+e0)", ntt_u_e1_pte, n);
    poly_add_mod_inpl(pk_c0, ntt_u_e1_pte, n, mod);
    // print_poly("c0 = pk0*u + m + e0 (ntt)", pk_c0, n);
}

bool ckks_next_prime_asym(Parms *parms, ZZ *u)
{
    se_assert(parms && parms->is_asymmetric);
    se_assert(u || parms->small_u);

    // -- Update curr_modulus_idx to next index
    if (!next_modulus(parms)) return 0;

    // -- If 'u' is not in small form, we convert it to use the next prime
    if (!parms->small_u) convert_poly_ternary_inpl(u, parms);
    return 1;
}

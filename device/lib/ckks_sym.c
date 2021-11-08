// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_sym.c
*/

#include "ckks_sym.h"

#include <complex.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>  // memset

#include "ckks_common.h"
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
size_t ckks_get_mempool_size_sym(size_t degree)
{
    se_assert(degree >= 16);
    if (degree == SE_DEGREE_N) return MEMPOOL_SIZE_sym;
    size_t n            = degree;
    size_t mempool_size = 4 * n;  // minimum

#ifdef SE_IFFT_OTF
#if defined(SE_NTT_ONE_SHOT) || defined(SE_NTT_REG)
    mempool_size += n;
#elif defined(SE_NTT_FAST)
    mempool_size += 2 * n;
#endif
#else
    mempool_size += 4 * n;
#endif

#if defined(SE_INDEX_MAP_PERSIST) || defined(SE_INDEX_MAP_LOAD_PERSIST) || \
    defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM) || defined(SE_SK_INDEX_MAP_SHARED)
    mempool_size += n / 2;
#endif

#ifdef SE_SK_PERSISTENT
    mempool_size += n / 16;
#endif

#ifdef SE_MEMPOOL_ALLOC_VALUES
    mempool_size += n / 2;
#endif

    se_assert(mempool_size);
    return mempool_size;
}

ZZ *ckks_mempool_setup_sym(size_t degree)
{
    size_t mempool_size = ckks_get_mempool_size_sym(degree);
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

void ckks_set_ptrs_sym(size_t degree, ZZ *mempool, SE_PTRS *se_ptrs)
{
    se_assert(mempool && se_ptrs);
    const size_t n = degree;

    // -- First, set everything to set size or 0
    se_ptrs->conj_vals         = (double complex *)mempool;
    se_ptrs->conj_vals_int_ptr = (int64_t *)mempool;
    se_ptrs->c1_ptr            = &(mempool[2 * n]);
    se_ptrs->c0_ptr            = &(mempool[3 * n]);
    se_ptrs->ntt_pte_ptr       = &(mempool[2 * n]);

    se_ptrs->ternary       = &(mempool[3 * n]);  // default: SE_SK_NOT_PERSISTENT
    se_ptrs->ifft_roots    = 0;                  // default: SE_IFFT_OTF
    se_ptrs->index_map_ptr = 0;                  // default: SE_INDEX_MAP_OTF
    se_ptrs->ntt_roots_ptr = 0;                  // default: SE_NTT_OTF
    se_ptrs->values        = 0;

    // -- Sizes
    size_t ifft_roots_size        = 0;
    size_t ntt_roots_size         = 0;
    size_t index_map_persist_size = 0;
    size_t s_persist_size         = 0;

    // -- Set ifft_roots based on IFFT type
#ifndef SE_IFFT_OTF
    ifft_roots_size      = 4 * n;
    se_ptrs->ifft_roots  = (double complex *)&(mempool[4 * n]);
    se_ptrs->ntt_pte_ptr = &(mempool[6 * n]);
#endif

    // -- Set ntt_roots based on NTT type
#if defined(SE_NTT_ONE_SHOT) || defined(SE_NTT_REG)
    ntt_roots_size         = n;
    se_ptrs->ntt_roots_ptr = &(mempool[4 * n]);
#elif defined(SE_NTT_FAST)
    ntt_roots_size         = 2 * n;
    se_ptrs->ntt_roots_ptr = &(mempool[4 * n]);
#endif

    size_t total_block2_size = ifft_roots_size ? ifft_roots_size : ntt_roots_size;

    // -- Set pi inverse based on index map type
#if defined(SE_INDEX_MAP_LOAD)
    se_ptrs->index_map_ptr = (uint16_t *)(&(mempool[4 * n]));  // 16n / sizeof(ZZ) = 4n
#elif defined(SE_INDEX_MAP_PERSIST) || defined(SE_INDEX_MAP_LOAD_PERSIST) || \
    defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
    // -- If ifft, this will be + the ifft_roots size
    //    else, this will be + the ntt size
    se_ptrs->index_map_ptr = (uint16_t *)(&(mempool[4 * n + total_block2_size]));
    index_map_persist_size = n / 2;
#endif

#ifdef SE_SK_PERSISTENT
    s_persist_size   = n / 16;
    se_ptrs->ternary = &(mempool[4 * n + total_block2_size + index_map_persist_size]);
#elif !defined(SE_IFFT_OTF) && defined(SE_SK_PERSISTENT_ACROSS_PRIMES)
    se_ptrs->ternary       = &(mempool[7 * n]);
#elif defined(SE_SK_INDEX_MAP_SHARED)
    se_ptrs->ternary       = &(mempool[4 * n]);
    index_map_persist_size = n / 2;
#endif

#ifdef SE_MEMPOOL_ALLOC_VALUES
    se_ptrs->values =
        (flpt *)&(mempool[4 * n + total_block2_size + index_map_persist_size + s_persist_size]);
#endif

    size_t address_size = 4;
    se_assert(((ZZ *)se_ptrs->conj_vals) == ((ZZ *)se_ptrs->conj_vals_int_ptr));
    se_assert(se_ptrs->c1_ptr ==
              (ZZ *)se_ptrs->conj_vals_int_ptr + 2 * n * sizeof(ZZ) / address_size);
    se_assert(se_ptrs->c1_ptr + n * sizeof(ZZ) / address_size == se_ptrs->c0_ptr);

    // -- Debugging: print all adresses
#ifdef SE_USE_MALLOC
    se_print_addresses(mempool, se_ptrs, n, 1);
    se_print_relative_positions(mempool, se_ptrs, n, 1);
#else
    se_print_addresses(mempool, se_ptrs);
    se_print_relative_positions(mempool, se_ptrs);
#endif
}

void ckks_setup_s(const Parms *parms, uint8_t *seed, SE_PRNG *prng, ZZ *s)
{
    // -- Keep s in small form until a later point, so we can store in
    //    separate memory in compressed form
    if (parms->sample_s)
    {
        se_assert(prng);
        prng_randomize_reset(prng, seed);
        sample_small_poly_ternary_prng_96(parms->coeff_count, prng, s);
        // -- TODO: Does not work to sample s for multi prime for now
        //    if s and index map share mem
    }
    else
    {
        SE_UNUSED(prng);
        load_sk(parms, s);
    }
}

void ckks_sym_init(const Parms *parms, uint8_t *share_seed, uint8_t *seed, SE_PRNG *shareable_prng,
                   SE_PRNG *prng, int64_t *conj_vals_int)
{
    // -- Each prng must be reset & re-randomized once per encode-encrypt sequence.
    // -- 'prng_randomize_reset' will set the prng seed to a random value and the prng counter to 0
    // -- (If seeds are !NULL, seeds will be used to seed prng instead of a random value.)
    // -- The seed associated with the prng used to sample 'a' can be shared
    // -- NOTE: The re-randomization is not strictly necessary if counter has not wrapped around
    //    and we share both the seed and starting counter value with the server
    //    for the shareable part.
    prng_randomize_reset(shareable_prng, share_seed);  // Used for 'a'
    prng_randomize_reset(prng, seed);                  // Used for error

    // -- Sample ep and add it to the signed pt.
    // -- This prng's seed value should not be shared.
    sample_add_poly_cbd_generic_inpl_prng_16(conj_vals_int, parms->coeff_count, prng);
}

void ckks_encode_encrypt_sym(const Parms *parms, const int64_t *conj_vals_int,
                             const int8_t *ep_small, SE_PRNG *shareable_prng, ZZ *s_small,
                             ZZ *ntt_pte, ZZ *ntt_roots, ZZ *c0_s, ZZ *c1, ZZ *s_save, ZZ *c1_save)
{
    se_assert(parms);
#ifdef SE_DISABLE_TESTING_CAPABILITY
    SE_UNUSED(ep_small);
    SE_UNUSED(s_save);
    SE_UNUSED(c1_save);
#endif

    // ==============================================================
    //   Generate ciphertext: (c[1], c[0]) = (a, [-a*s + m + e]_Rq)
    // ==============================================================
    const PolySizeType n = parms->coeff_count;
    const Modulus *mod   = parms->curr_modulus;

    // ----------------------
    //     c1 = a <--- U
    // ----------------------
    sample_poly_uniform(parms, shareable_prng, c1);
    // print_poly("rlwe a(c1)", c1, n);

#ifndef SE_DISABLE_TESTING_CAPABILITY
    se_assert(conj_vals_int || ep_small);
    // -- At this point, it is safe to send c1 away. This will allow us to re-use c1's memory.
    //    However, we may be debugging and need to store c1 somewhere for debugging later.
    // -- Note: This method provides very little memory savings overall, so isn't necessary to use.
    if (c1_save) memcpy(c1_save, c1, n * sizeof(ZZ));
#endif

    // ----------------------------
    //    c0 = [-a*s + m + e]_Rq
    // ----------------------------
    // -- Load s (if not already loaded)
    // -- For now, we require s to be in small form.
    se_assert(s_small);
#ifdef SE_SK_NOT_PERSISTENT
    se_assert(!parms->sample_s);
    load_sk(parms, s_small);
#elif defined(SE_SK_PERSISTENT_ACROSS_PRIMES)
    // -- Note that if we are here, ifft type is not otf, which means that SE_REVERSE_CT_GEN_ENABLED
    //    cannot be defined. Therefore, we only have to check that the current
    //    modulus is 0 to know that we are in the first prime of the modulus chain
    if (parms->curr_modulus_idx == 0)
    {
        se_assert(!parms->sample_s);
        load_sk(parms, s_small);
    }
#endif
    // print_poly_small("s (small)", s_small, parms->coeff_count);

    // -- Expand and store s in c0
    // print_poly_uint8_full("s (small)", (uint8_t*)s_small, parms->coeff_count/4);
    // print_poly_small_full("s (small)", s_small, parms->coeff_count);
    expand_poly_ternary(s_small, parms, c0_s);
    // print_poly_full("s", c0_s, parms->coeff_count);
    // print_poly_ternary("s", c0_s, parms->coeff_count, false);

    // -- Calculate [a*s]_Rq = [c1*s]_Rq. This will free up c1 space too.
    //    First calculate ntt(s) and store in c0_s. Note that this will load
    //    the ntt roots into ntt_roots memory as well (used later for
    //    calculating ntt(pte))

    // -- Note: Calling ntt_roots_initialize will do nothing if SE_NTT_OTF is defined
    ntt_roots_initialize(parms, ntt_roots);
    ntt_inpl(parms, ntt_roots, c0_s);
#ifndef SE_DISABLE_TESTING_CAPABILITY
    // -- Save ntt(reduced(s)) for later decryption
    // print_poly_ternary("s (ntt)", c0_s, parms->coeff_count, false);
    if (s_save) memcpy(s_save, c0_s, n * sizeof(c0_s[0]));
        // print_poly_ternary("s_save (ntt)", s_save, parms->coeff_count, false);
#endif
    poly_mult_mod_ntt_form_inpl(c0_s, c1, n, mod);
    // print_poly("rlwe a*s  ", c0_s, n);

    // -- Negate [a*s]_Rq to get [-a*s]_Rq
    poly_neg_mod_inpl(c0_s, n, mod);
    // print_poly("rlwe -a*s ", c0_s, n);

    // -- Calculate reduce(m + e) == reduce(conj_vals_int) ---> store in ntt_pte
#ifndef SE_DISABLE_TESTING_CAPABILITY
    if (ep_small)
        reduce_set_e_small(parms, ep_small, ntt_pte);
    else
#endif
        reduce_set_pte(parms, conj_vals_int, ntt_pte);
    // print_poly("red(pte)", ntt_pte, parms->coeff_count);

    // -- Calculate ntt(m + e) = ntt(reduce(conj_vals_int)) = ntt(ntt_pte)
    //    and store result in ntt_pte. Note: ntt roots (if required) should already be
    //    loaded from above
    ntt_inpl(parms, ntt_roots, ntt_pte);
    // print_poly("ntt(m + e)", ntt_pte, n);

    // -- Debugging
    // intt_roots_initialize(parms, ntt_roots);
    // intt(parms, ntt_roots, ntt_pte);
    // print_poly_full("intt(ntt(pte))", ntt_pte, parms->coeff_count);

    poly_add_mod_inpl(c0_s, ntt_pte, n, mod);
    // print_poly("a*s + m + e (ntt form)", c0_s, n);
}

bool ckks_next_prime_sym(Parms *parms, ZZ *s)
{
    se_assert(parms && !parms->is_asymmetric);

    if (!parms->small_s) convert_poly_ternary_inpl(s, parms);

    // -- Update curr_modulus_idx to next index
    bool ret = next_modulus(parms);
    return ret;
}

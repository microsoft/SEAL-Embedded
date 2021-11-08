// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_asym.h
*/

#pragma once

#include <complex.h>
#include <stdbool.h>

#include "ckks_common.h"
#include "defines.h"
#include "modulo.h"
#include "parameters.h"
#include "rng.h"

#ifdef SE_USE_MALLOC
/**
Returns the required size of the memory pool in units of sizeof(ZZ).

@param[in] degree  Desired polynomial ring degree
@returns           Required size of the memory pool units of sizeof(ZZ)
*/
size_t ckks_get_mempool_size_asym(size_t degree);

/**
Sets up the memory pool for CKKS asymmetric encryption.

Note: This function calls calloc.

@param[in] degree  Desired polynomial ring degree
@returns           A handle to the memory pool
*/
ZZ *ckks_mempool_setup_asym(size_t degree);
#endif

/**
Sets addresses of objects according to parameter settings for asymmetric CKKS encryption.
Should only need to be called once during initial memory allocation.

@param[in]  degree   Polynomial ring degree
@param[in]  mempool  Handle to memory pool
@param[out] se_ptrs  Pointers object. Pointers will be updated to mempool locations
*/
void ckks_set_ptrs_asym(size_t degree, ZZ *mempool, SE_PTRS *se_ptrs);

/**
Generates a CKKS public key from a CKKS secret key. Mainly useful for testing.

Size req: If SE_NTT_OTF is not defined, 'ntt_roots' must contain space for roots according to NTT
type chosen. If seed != NULL, seed must be SE_PRNG_SEED_BYTE_COUNT long.

@param[in]     parms           Parameters set by ckks_setup
@param[in]     s_small         Secret key in small form
@param         ntt_roots       [Optional]. Scratch for ntt roots. Ignored if SE_NTT_OTF is chosen.
@param[in]     seed            [Optional]. Seed to seed 'prng' with.
@param[in,out] shareable_prng  A shareable prng instance. Will reset and update counter.
@param[out]    s_save          Secret key mod current modulus (in ntt form) (for testing)
@param[out]    ep_small        Secret key error polynomial ep in small form
@param[out]    ntt_ep          Expanded ep modulus the current modulus (in ntt form)
@param[out]    pk_c0           First component of public key for current modulus
@param[out]    pk_c1           Second component of public key for current modulus
*/
void gen_pk(const Parms *parms, ZZ *s_small, ZZ *ntt_roots, uint8_t *seed, SE_PRNG *shareable_prng,
            ZZ *s_save, int8_t *ep_small, ZZ *ntt_ep, ZZ *pk_c0, ZZ *pk_c1);

/**
Initializes values for a single full asymmetric CKKS encryption. Samples the errors (w/o respect to
any prime), as well as the ternary polynomial 'u'. Should be called once per encode-encrypt sequence
(just after ckks_encode_base).

Note: This function modifies (i.e. resets and re-randomizes) the prng instance.

Size req: If seed != NULL, seed must be SE_PRNG_SEED_BYTE_COUNT long.

@param[in]     parms          Parameters set by ckks_setup
@param[in]     seed           [Optional]. Seed to seed 'prng' with.
@param[in,out] prng           PRNG instance needed to generate error and ternary polynomials.
@param[in,out] conj_vals_int  As pointed to by conj_vals_int_ptr (n int64 values). In:
                              ckks plaintext; Out: plaintext + e0 (in non-reduced form)
@param[out]    u              Ternary polynomial
@param[out]    e1             Error e1 (in non-reduced form)
*/
void ckks_asym_init(const Parms *parms, uint8_t *seed, SE_PRNG *prng, int64_t *conj_vals_int, ZZ *u,
                    int8_t *e1);

/**
Encodes and asymmetrically encrypts a vector of values using CKKS, for the current modulus prime.
Optionally returns some additional values useful for testing, if SE_DISABLE_TESTING_CAPABILITY is
not defined.

Size req: 'ntt_roots' should have space for NTT roots according to NTT option chosen.
'ntt_u_e1_pte', 'pk_c0', and 'pk_c1' should have space for n ZZ elements.  If testing,
'ntt_u_save' and 'ntt_e1_save' should have space for n ZZ elements.

@param[in]  parms          Parameters set by ckks_setup
@param[in]  conj_vals_int  As set by ckks_asym_init
@param[in]  u              As set by ckks_asym_init
@param[in]  e1             As set by ckks_asym_init
@param      ntt_roots      [Optional]. Scratch space for ntt roots. Ignored if SE_NTT_OTF is chosen.
@param[out] ntt_u_e1_pte   Scratch space. Out: m + e0 in reduced and ntt form (useful for testing).
@param[out] ntt_u_save     [Optional, ignored if NULL]. Expanded and ntt form of u (for testing)
@param[out] ntt_e1_save    [Optional, ignored if NULL]. Reduced and ntt form of e1 (for testing)
@param[out] pk_c0          First component of ciphertext
@param[out] pk_c1          Second component of ciphertext
*/
void ckks_encode_encrypt_asym(const Parms *parms, const int64_t *conj_vals_int, const ZZ *u,
                              const int8_t *e1, ZZ *ntt_roots, ZZ *ntt_u_e1_pte, ZZ *ntt_u_save,
                              ZZ *ntt_e1_save, ZZ *pk_c0, ZZ *pk_c1);

/**
Updates parameters to next prime in modulus switching chain for asymmetric CKKS encryption. Also
reduces ternary polynomial to next prime modulus if used in expanded form (compressed form u will be
reduced later).

@param[in,out] parms  Parameters set by ckks_setup. curr_modulus_idx will be advanced by 1
@param[in,out] u      [Optional]. Ternary polynomial for asymmetric encryption.
                      Can be null if 'u' is stored in compressed form.
@returns              1 on success, 0 on failure (reached end of modulus chain)
*/
bool ckks_next_prime_asym(Parms *parms, ZZ *u);

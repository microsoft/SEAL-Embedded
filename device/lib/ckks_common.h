// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_common.h
*/

#pragma once

#include <complex.h>
#include <stdbool.h>

#include "defines.h"
#include "modulo.h"
#include "parameters.h"
#include "rng.h"

/**
Object that stores pointers to various objects for CKKS encode/encryption.
For the following, n is the polynomial ring degree.

@param conj_vals          Storage for output of encode
@param ifft_roots         Roots for inverse fft (n double complex values)
@param values             Floating point values to encode/encrypt
@param ternary            Ternary polynomial ('s' for symmetric, 'u' for asymmetric).
                          May be in small or expanded form, depending on parameters.
@param conj_vals_int_ptr
@param c0_ptr             1st component of a ciphertext for a particular prime
@param c1_ptr             2nd component of a ciphertext for a particular prime
@param index_map_ptr      Index map values
@param ntt_roots_ptr      Storage for NTT roots
@param ntt_pte_ptr        Used for adding the plaintext to the error.
                          If asymmetric, this is also used for ntt(u) and ntt(e1).
@param e1_ptr             Second error polynomial (unused in symmetric case)
*/
typedef struct SE_PTRS
{
    double complex *conj_vals;
    double complex *ifft_roots;  // Roots for inverse fft (n double complex values)
    flpt *values;  // Floating point values to encode/encrypt (up to n/2 type-ZZ values)
    ZZ *ternary;   // Ternary polynomial ('s' for symmetric, 'u' for asymmetric).

    // -- The following will point to sections of conj_vals and ifft_roots above

    int64_t *conj_vals_int_ptr;
    ZZ *c0_ptr;               // 1st component of a ciphertext for a particular prime
    ZZ *c1_ptr;               // 2nd component of a ciphertext for a particular prime
    uint16_t *index_map_ptr;  // Index map values
    ZZ *ntt_roots_ptr;        // Storage for NTT roots
    ZZ *ntt_pte_ptr;          // Used for adding the plaintext to the error.
    int8_t *e1_ptr;           // Second error polynomial (unused in symmetric case)
} SE_PTRS;

/**
Computes the values for the index map. This corresponds to the "pi" inverse projection symbol in the
CKKS paper, merged with the bit-reversal required for the ifft/fft.

Size req: 'index_map' must constain space for n uint16_t elements.

@param[in]  parms      Parameters set by ckks_setup
@param[out] index_map  Index map values
*/
void ckks_calc_index_map(const Parms *parms, uint16_t *index_map);

/**
Sets the parameters according to the request polynomial ring degree. Also sets the index map if
required (based on index map option defined). This should be called once during memory allocation to
setup the parameters.

Note: index_map is non-const in case SE_INDEX_MAP_LOAD is defined (or, if
SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM is defined and asymmetric encryption is used).

Size req: If index map needs to be loaded (see 'Note' above), index_map must constain
space for n uint16_t elements.

@param[in]  degree     Desired polynomial ring degree.
@param[in]  nprimes    Desired number of primes
@param[out] index_map  [Optional]. Pointer to index map values buffer
@param[out] parms      Parameters instance
*/
void ckks_setup(size_t degree, size_t nprimes, uint16_t *index_map, Parms *parms);

/**
Sets the parameters according to request custom parameters. Also sets the index map if required
(based on index map option defined). This should be called once during memory allocation to setup
the parameters. If either 'modulus_vals' or 'ratios' is NULL, uses regular (non-custom) ckks_setup
instead.

Note: index_map is non-const in case SE_INDEX_MAP_LOAD is defined (or, if
SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM is defined and asymmetric encryption is used).

Size req: If index map needs to be loaded (see 'Note' above), index_map must constain space for n
uint16_t elements.

@param[in]  degree        Desired polynomial ring degree.
@param[in]  nprimes       Desired number of primes
@param[in]  modulus_vals  An array of nprimes type-ZZ modulus values.
@param[in]  ratios        An array of const_ratio values for each custom modulus value
                          (high word, followed by low word).
@param[out] index_map     [Optional]. Pointer to index map values buffer
@param[out] parms         Parameters instance
*/
void ckks_setup_custom(size_t degree, size_t nprimes, const ZZ *modulus_vals, const ZZ *ratios,
                       uint16_t *index_map, Parms *parms);

/**
Resets the encryption parameters. Should be called once per encode-encrypt sequence to set
curr_modulus_idx back to the start of the modulus chain. Does not need to be called the very first
time after ckks_setup_parms is called, however.

@param[in,out] parms  Parameters set by ckks_setup
*/
void ckks_reset_primes(Parms *parms);

/**
CKKS encoding base (w/o respect to a particular modulus). Should be called once per encode-encrypt
sequence. Encoding can fail for certain inputs, so returns a value indicating success or failure.

Note: index_map is non-const in case SE_INDEX_MAP_LOAD is defined (or, if
SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM is defined and asymmetric encryption is used).

Size req: 'values' can contain at most n/2 slots (i.e. n/2 ZZ values), where n is the polynomial
ring degree. 'conj_vals' must contain space for n double complex values. If index map needs to be
loaded (see 'Note' above), index_map must constain space for n uint16_t elements.

@param[in]  parms       Parameters set by ckks_setup
@param[in]  values      Initial message array with (up to) n/2 slots
@param[in]  values_len  Number of elements in values array. Must be <= n/2.
@param[in]  index_map   [Optional]. If passed in, can avoid 1 flash read
@param      ifft_roots  Scratch space to load ifft roots
@param[out] conj_vals   conj_vals_int in first n ZZ values
@returns                True on success, False on failure
*/
bool ckks_encode_base(const Parms *parms, const flpt *values, size_t values_len,
                      uint16_t *index_map, double complex *ifft_roots, double complex *conj_vals);

/**
Reduces all values in conj_vals_int modulo the current modulus and stores result in out.

Size req: out must contain space for n ZZ values

@param[in]  parms          Parameters set by ckks_setup
@param[in]  conj_vals_int  Array of values to be reduced, as output by ckks_encode_base
@param[out] out            Result array of reduced values
*/
void reduce_set_pte(const Parms *parms, const int64_t *conj_vals_int, ZZ *out);

/**
Reduces all values in conj_vals_int modulo the current modulus and addres result to out.

Size req: out must contain space for n ZZ values

@param[in]  parms          Parameters set by ckks_setup
@param[in]  conj_vals_int  Array of values to be reduced
@param[out] out            Updated array
*/
void reduce_add_pte(const Parms *parms, const int64_t *conj_vals_int, ZZ *out);

/**
Converts an error polynomial in small form to an error polynomial modulo the current
modulus.

Size req: out must contain space for n ZZ values

@param[in]  parms  Parameters set by ckks_setup
@param[in]  e      Error polynomial in small form
@param[out] out    Result error polynomial modulo the current modulus
*/
void reduce_set_e_small(const Parms *parms, const int8_t *e, ZZ *out);

/**
Converts an error polynomial in small form to an error polynomial modulo the current
modulus and adds it to the values stored in the 'out' array.

Size req: out must contain space for n ZZ values

@param[in]  parms  Parameters set by ckks_setup
@param[in]  e      Error polynomial in small form
@param[out] out    Result updated polynomial
*/
void reduce_add_e_small(const Parms *parms, const int8_t *e, ZZ *out);

#ifdef SE_USE_MALLOC
/**
Prints the relative positions of various objects.

@param[in] st       Starting address of memory pool
@param[in] se_ptrs  SE_PTRS object contains pointers to objects
@param[in] n        Polynomial ring degree
@param[in] sym      Set to 1 if in symmetric mode
*/
void se_print_relative_positions(const ZZ *st, const SE_PTRS *se_ptrs, size_t n, bool sym);

/**
Prints the addresses of various objects.

@param[in] mempool  Memory pool handle
@param[in] se_ptrs  SE_PTRS object contains pointers to objects
@param[in] n        Polynomial ring degree
@param[in] sym      Set to 1 if in symmetric mode
*/
void se_print_addresses(const ZZ *mempool, const SE_PTRS *se_ptrs, size_t n, bool sym);

/**
Prints a banner for the size of the memory pool

@param[in] n        Polynomial ring degree
@param[in] sym      Set to 1 if in symmetric mode
*/
void print_ckks_mempool_size(size_t n, bool sym);
#else
/**
Prints the relative positions of various objects.

@param[in] st       Starting address of memory pool
@param[in] se_ptrs  SE_PTRS object contains pointers to objects
*/
void se_print_relative_positions(const ZZ *st, const SE_PTRS *se_ptrs);

/**
Prints the addresses of various objects.

@param[in] mempool  Memory pool handle
@param[in] se_ptrs  SE_PTRS object contains pointers to objects
*/
void se_print_addresses(const ZZ *mempool, const SE_PTRS *se_ptrs);

/**
Prints a banner for the size of the memory pool
*/
void print_ckks_mempool_size(void);
#endif

// -- Calculate mempool size for no-malloc case
#ifdef SE_IFFT_OTF
#if defined(SE_NTT_ONE_SHOT) || defined(SE_NTT_REG)
#define MEMPOOL_SIZE_BASE 5 * SE_DEGREE_N
#elif defined(SE_NTT_FAST)
#define MEMPOOL_SIZE_BASE 7 * SE_DEGREE_N
#else
#define MEMPOOL_SIZE_BASE 4 * SE_DEGREE_N
#endif
#else
#define MEMPOOL_SIZE_BASE 8 * SE_DEGREE_N
#endif

#if defined(SE_INDEX_MAP_PERSIST) || defined(SE_INDEX_MAP_LOAD_PERSIST) || \
    defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM) || defined(SE_SK_INDEX_MAP_SHARED)
#define SE_INDEX_MAP_PERSIST_SIZE_sym SE_DEGREE_N / 2
#else
#define SE_INDEX_MAP_PERSIST_SIZE_sym 0
#endif

#ifdef SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM
#define SE_INDEX_MAP_PERSIST_SIZE_asym 0
#else
#define SE_INDEX_MAP_PERSIST_SIZE_asym SE_INDEX_MAP_PERSIST_SIZE_sym
#endif

#ifdef SE_SK_PERSISTENT
#define SK_PERSIST_SIZE SE_DEGREE_N / 16
#else
#define SK_PERSIST_SIZE 0
#endif

#ifdef SE_MEMPOOL_ALLOC_VALUES
#define VALUES_ALLOC_SIZE SE_DEGREE_N / 2
#else
#define VALUES_ALLOC_SIZE 0
#endif

#define MEMPOOL_SIZE_sym \
    MEMPOOL_SIZE_BASE + SE_INDEX_MAP_PERSIST_SIZE_sym + SK_PERSIST_SIZE + VALUES_ALLOC_SIZE

#ifdef SE_IFFT_OTF
#define MEMPOOL_SIZE_BASE_Asym MEMPOOL_SIZE_BASE + SE_DEGREE_N + SE_DEGREE_N / 4 + SE_DEGREE_N / 16
// 4n + n + n + n/4 + n/16
#else
#define MEMPOOL_SIZE_BASE_Asym MEMPOOL_SIZE_BASE
#endif

#define MEMPOOL_SIZE_Asym \
    MEMPOOL_SIZE_BASE_Asym + SE_INDEX_MAP_PERSIST_SIZE_asym + VALUES_ALLOC_SIZE

#ifdef SE_ENCRYPT_TYPE_SYMMETRIC
#define MEMPOOL_SIZE MEMPOOL_SIZE_sym
#else
#define MEMPOOL_SIZE MEMPOOL_SIZE_Asym
#endif

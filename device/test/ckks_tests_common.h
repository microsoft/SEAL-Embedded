// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_tests_common.h
*/

#pragma once

#include <complex.h>

#include "defines.h"
#include "parameters.h"
#include "test_common.h"

/**
Sets the values of 'v' according to the particular test number.
If testnum > 8, will set testnum to 8.

@param[in]  testnum  Test number in range [0,9]
@param[in]  vlen     Number of flpt elements in v
@param[out] v        Array to set
*/
void set_encode_encrypt_test(size_t testnum, size_t vlen, flpt *v);

/**
(Pseudo) ckks decode. 'values_decoded' and 'pt' may share the same starting address for in-place
computation (see: ckks_decode_inpl)

Note: index_map is non-const in case SE_INDEX_MAP_LOAD is defined (or, if
SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM is defined and asymmetric encryption is used).

Size req: 'temp' must contain space for n double complex values
'values_decoded' must contain space for n/2 flpt elements.
If index map needs to be loaded (see 'Note' above), index_map must constain space for n uint16_t
elements.

@param[in]  pt              Plaintext to decode
@param[in]  values_len      True number of entries in initial values message. Must be <= n/2
@param[in]  index_map       [Optional]. If passed in, can avoid 1 flash read
@param[in]  parms           Parameters set by ckks_setup
@param      temp	        Scratch space
@param[out] values_decoded  Decoded message
*/
void ckks_decode(const ZZ *pt, size_t values_len, uint16_t *index_map, const Parms *parms,
                 double complex *temp, flpt *values_decoded);

/**
(Pseudo) in-place ckks decode.

Note: index_map is non-const in case SE_INDEX_MAP_LOAD is defined (or, if
SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM is defined and asymmetric encryption is used).

Size req: 'temp' must contain space for n double complex values
'values_decoded' must contain space for n/2 flpt elements
If index map needs to be loaded (see 'Note' above), index_map must constain space for n uint16_t
elements.

@param[in,out] pt          In: Plaintext to decode; Out: Decoded message
@param[in]     values_len  True number of entries in initial values message. Must be <= n/2
@param[in]     index_map   [Optional]. If passed in, can avoid 1 flash read
@param[in]     parms       Parameters set by ckks_setup
@param         temp	       Scratch space
*/
static inline void ckks_decode_inpl(ZZ *pt, size_t values_len, uint16_t *index_map,
                                    const Parms *parms, double complex *temp)
{
    ckks_decode(pt, values_len, index_map, parms, temp, (flpt *)pt);
}

/**
Checks that the pseudo-decoding of a ciphertext is correct.

Note: index_map is non-const in case SE_INDEX_MAP_LOAD is defined (or, if
SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM is defined and asymmetric encryption is used).

Size req: 'temp' must contain space for n double complex values
If index map needs to be loaded (see 'Note' above), index_map must constain space for n uint16_t
elements.

@param[in,out] pt          In: Plaintext to decode; Out: Decoded message
@param[in]     values      Cleartext input message
@param[in]     values_len  True number of entries in initial values message. Must be <= n/2
@param[in]     index_map   [Optional]. If passed in, can avoid 1 flash read
@param[in]     parms       Parameters instance
@param         temp	       Scratch space
*/
void check_decode_inpl(ZZ *pt, const flpt *values, size_t values_len, uint16_t *index_map,
                       const Parms *parms, ZZ *temp);

/**
Pseudo-decrypts a ciphertext in place (for a particular prime component). Currently only works if
'small_s' is 0. 'c0' and 'pt' may share the same starting address for in-place computation (see:
ckks_decrypt_inpl).

Size req: 'pt' must have space for n elements

@param[in]  c0       1st ciphertext component for this prime
@param[in]  c1       2nd ciphertext component for this prime
@param[in]  s        Secret key. Must be in expanded form!
@param[in]  small_s  If true, secret key is in small form; Else, is in expanded form
@param[in]  parms    Parameters instance
@param[out] pt       Decypted result (a ckks plaintext)
*/
void ckks_decrypt(const ZZ *c0, const ZZ *c1, const ZZ *s, bool small_s, const Parms *parms,
                  ZZ *pt);

/**
Pseudo-decrypts a ciphertext in place (for a particular prime component). Currently only works if
small_s is 0. Note that this function will also corrupt the memory pointed to by c1.

@param[in,out] c0       In: First ciphertext component for this prime;
                        Out: Decrypted result (a CKKS plaintext)
@param[in]     c1       Second ciphertext component for this prime. Will be corrupted
@param[in]     s        Secret key. Must be in expanded form!
@param[in]     small_s  If true, secret key is in small form; Else, is in expanded form
@param[in]     parms    Parameters instance
*/
void ckks_decrypt_inpl(ZZ *c0, ZZ *c1, const ZZ *s, bool small_s, const Parms *parms);

/**
Checks that the pseudo-decoding and pseudo-decryption of a ciphertext is correct (for a particular
prime ciphertext component). Currently only works if 'small_s' is 0.

Correctness: 'small_s' must be true (and 's' must be in small form)

Note: index_map is non-const in case SE_INDEX_MAP_LOAD is defined (or, if
SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM is defined and asymmetric encryption is used).

Size req: 'temp' must contain space for n double complex values
If index map needs to be loaded (see 'Note' above), index_map must constain space for n uint16_t
elements.

@param[in]  c0          In: 1st component of ciphertext to check;
                        Out: Decrypted and decoded message
@param[in]  c1          2nd component of ciphertext to check. Will be corrupted!
@param[in]  values      Cleartext input message
@param[in]  values_len  True number of entries in initial values message. Must be <= n/2
@param[in]  s           Secret key. Must be in expanded form!
@param[in]  small_s     If true, secret key is in small form; Else, is in expanded form
@param[in]  pte_calc    Plaintext + error (in NTT form) (output by ckks_encode for testing)
@param[in]  index_map   [Optional]. If passed in, can avoid 1 flash read
@param[in]  parms       Parameters instance
@param      temp	    Scratch space
*/
void check_decode_decrypt_inpl(ZZ *c0, ZZ *c1, const flpt *values, size_t values_len, const ZZ *s,
                               bool small_s, const ZZ *pte_calc, uint16_t *index_map,
                               const Parms *parms, ZZ *temp);

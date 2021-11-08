// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file seal_embedded.h

SEAL-Embedded offers an easy to use API for easy development. A user should call the se_setup
function once at the start of library initialization and se_encrypt once for every array of values
to encode/encrypt. 'se_cleanup' exists for completeness to free library memory, but should never
need to be called. SEAL-Embedded offers three types of setup functions for developers, providing
different degrees of library configurability. se_setup_custom allows for the most customization,
while se_setup_default uses default parameter settings. Note: Currently, SEAL-Embedded does not
support reconfiguring parameters to a different parameter set after initial setup.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ckks_common.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
Error code values range from 0 to -9999.
Application-specific errors should use values outside this range.
TODO: Use error codes
*/
#define SE_SUCCESS 0
#define SE_ERR_NO_MEMORY -12
#define SE_ERR_INVALD_ARGUMENT -22
#define SE_ERR_UNKNOWN -1000
/** Minimum negative error code value used by SEAL-Embedded */
#define SE_ERR_MINIMUM -9999

#ifndef SE_USE_MALLOC
static ZZ **mempool_ptr_global;
#endif

/**
SEAL-Embedded parameters struct for API.

@param parms    Pointer to internal parameters struct
@param se_ptrs  Pointer to SE_PTRS struct
*/
typedef struct
{
    Parms *parms;
    SE_PTRS *se_ptrs;
} SE_PARMS;

typedef enum { SE_SYM_ENCR, SE_ASYM_ENCR } EncryptType;

/**
Network send function pointer.
The first input parameter should represent a pointer to the data to send.
The second input parameter should represent the size of the data to send.
*/
typedef size_t (*SEND_FNCT_PTR)(void *, size_t);

/**
Randomness generator function pointer.
The first input parameter should represent a pointer to the buffer to store the random values.
The second input parameter should represent the size of randomness to generate.
The third input parameters can specify any required flags.
*/
typedef ssize_t (*RND_FNCT_PTR)(void *, size_t, unsigned int flags);

/**
Setups up SEAL-Embedded for a particular encryption type for a custom parameter set, including a
custom degree, number of modulus primes, modulus prime values, and scale. If either modulus_vals or
ratios is NULL, reverts to se_setup functionality.

Note: This function calls calloc.

@param[in] degree        Polynomial ring degree
@param[in] nprimes       Number of prime moduli
@param[in] modulus_vals  An array of nprimes type-ZZ modulus values.
@param[in] ratios        An array of const_ratio values for each custom modulus value
                         (high word, followed by low word).
@param[in] scale         Scale
@param[in] enc_type      Encryption type
@returns                 A handle to the set SE_PARMS instance
*/
SE_PARMS *se_setup_custom(size_t degree, size_t nprimes, const ZZ *modulus_vals, const ZZ *ratios,
                          double scale, EncryptType encrypt_type);

/**
Setups up SEAL-Embedded for a particular encryption type for the requested parameter set.

Note: This function calls calloc.

@param[in] degree    Polynomial ring degree
@param[in] nprimes   Number of prime moduli
@param[in] scale     Scale
@param[in] enc_type  Encryption type
@returns             A handle to an SE_PARMS instance
*/
SE_PARMS *se_setup(size_t degree, size_t nprimes, double scale, EncryptType encrypt_type);

/**
Setups up SEAL-Embedded for a particular encryption type for a parameter set of degree = 4096, a
coefficient modulus of 3 30-bit primes, and a scale of pow(2,25).

Note: This function calls calloc.

@param[in] enc_type  Encryption type (SE_SYM_ENCR or SE_ASYM_ENCR)
@returns             A handle to the set SE_PARMS instance
*/
SE_PARMS *se_setup_default(EncryptType enc_type);

bool se_encrypt_seeded(uint8_t *shareable_seed, uint8_t *seed, SEND_FNCT_PTR network_send_function,
                       void *v, size_t vlen_bytes, bool print, SE_PARMS *se_parms);

bool se_encrypt(SEND_FNCT_PTR network_send_function, void *v, size_t vlen_bytes, bool print,
                SE_PARMS *se_parms);

/**
Frees some library memory and resets parameters object. Should never need to be called by the
typical user.

@param[in] se_parms  SE_PARMS instance to free
*/
void se_cleanup(SE_PARMS *se_parms);

#ifdef __cplusplus
}
#endif

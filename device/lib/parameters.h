// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file parameters.h

Functions for creating and modifying a Parameters instance
*/

#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdint.h>  // uint64_t
#include <stdlib.h>  // size_t

#include "defines.h"
#include "modulo.h"  // Modulus

/**
Storage for encryption parameters.

@param coeff_count       Number of coefficients in polyomial = n = poly_modulus_degree
@param logn              log2(n) (number of bits to represent n)
@param moduli            Array of moduli that represents the modulus switching chain
@param curr_modulus      Pointer to current modulus in switching chain
@param curr_modulus_idx  Index of current modulus in 'moduli' vector
@param nprimes           Number of 'Modulus' objects in 'moduli' array
@param scale             CKKS scale value
@param is_asymmetric     Set to 1 if using public key encryption
@param pk_from_file      Set to 1 to use a public key from a file
@param sample_s          Set to 1 to sample the secret key
@param small_s           Set to 1 to store the secret key in small form while processing
                         Note: SEAL-Embedded currently only works if this is 1
@param small_u           Set to 1 to store the 'u' vector in small form while processing
                         Note: SEAL-Embedded currently only works if this is 1
@param curr_param_direction  Set to 1 to operate over primes in reverse order. Only
                             available if SE_REVERSE_CT_GEN_ENABLED is enabled.
@param skip_ntt_load         Set to 1 to skip a load of the NTT roots (if applicable,
                             based on NTT option chosen.) Only available if
                             SE_REVERSE_CT_GEN_ENABLED is enabled.
*/
typedef struct
{
    size_t coeff_count;  // Number of coefficients in polyomial = n = poly_modulus_degree
    size_t logn;         // log2(n) (number of bits to represent n)
#ifdef SE_USE_MALLOC
    Modulus *moduli;  // Array of moduli that represents the modulus switching chain
#else
    Modulus moduli[SE_NPRIMES];  // Array of moduli that represents the modulus switching
                                 // chain
#endif
    Modulus *curr_modulus;    // Pointer to current modulus in switching chain
    size_t curr_modulus_idx;  // Index of current modulus in 'moduli' vector

    size_t nprimes;      // Number of 'Modulus' objects in 'moduli' array
    double scale;        // CKKS scale value
    bool is_asymmetric;  // Set to 1 if using public key encryption
    bool pk_from_file;   // Set to 1 to use a public key from a file
    bool sample_s;       // Set to 1 to sample the secret key
    bool small_s;        // Set to 1 to store the secret key in small form while processing
    bool small_u;        // Set to 1 to store the 'u' vector in small form while processing
#ifdef SE_REVERSE_CT_GEN_ENABLED
    bool curr_param_direction;  // Set to 1 to operate over primes in reverse order
    bool skip_ntt_load;         // Set to 1 to skip a load of the NTT roots
#endif
} Parms;

/**
Utility function to get the log2 of a value.

@param[in] val  Value
@returns        log2(val)
*/
static inline size_t get_log2(size_t val)
{
    switch (val)
    {
        case 2048: return 11;
        case 4096: return 12;
        case 8192: return 13;
        case 16384: return 14;
        default: return (size_t)log2(val);
    }
}

/**
Releases the memory allocated for modulus chain is SE_USE_MALLOC is defined. Does nothing otherwise.

@param[in,out] parms  Parameters instance
*/
void delete_parameters(Parms *parms);

/**
Resets parameters (sets curr_modulus_idx back to the start of modulus chain)

@param[in,out] parms  Parameters instance
*/
void reset_primes(Parms *parms);

/**
Updates parms to next modulus in modulus chain.

@param[in,out] parms  Parameters instance
@returns              1 on success, 0 on fail (reached end of chain)
*/
bool next_modulus(Parms *parms);

/**
Sets up SEAL-Embedded parameters object with default moduli for requested degree for CKKS.
Also sets the scale.

Req: degree must be a power of 2 (between 1024 and 16384, inclusive)
Note: Moduli should be the same as the default moduli chosen by the SEAL-Embedded adapter
(not including the special prime).

@param[in]  degree   Polynomial ring degree
@param[in]  nprimes  Number of prime moduli
@param[out] parms    Parameters instance
*/
void set_parms_ckks(size_t degree, size_t nprimes, Parms *parms);

/**
Sets up SEAL-Embedded parameters object according to requested custom parameters for CKKS. If either
'modulus_vals' or 'ratios' is NULL, uses default values.

@param[in]  degree        Polynomial ring degree
@param[in]  scale         Scale to use for encoding/decoding
@param[in]  nprimes       Number of prime moduli
@param[in]  modulus_vals  An array of nprimes type-ZZ modulus values.
@param[in]  ratios        An array of const_ratio values for each custom modulus value
                          (high word, followed by low word).
@param[out] parms         Parameters instance
*/
void set_custom_parms_ckks(size_t degree, double scale, size_t nprimes, const ZZ *modulus_vals, const ZZ *ratios,
                           Parms *parms);

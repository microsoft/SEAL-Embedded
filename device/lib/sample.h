// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file sample.h

Functions to sample a value or polynomial from a uniform, uniform ternary or centered binomial
distribution.
*/

#pragma once

#include "defines.h"
#include "parameters.h"
// #include "util_print.h" // TODO: Including this breaks things?
#include <stdint.h>  // uint64_t

#include "rng.h"
#include "stdio.h"

#ifdef SE_RAND_GETRANDOM
#include <sys/random.h>  // getrandom
#elif defined(SE_RAND_NRF5)
#include "nrf_crypto.h"
#include "nrf_crypto_rng.h"
#endif

// ----------------------------------------------
//                   Randomness
// ----------------------------------------------
#ifdef SE_ON_NRF5
/** Enables randomness on the NRF5. Must be called once on the NRF5. */
void se_randomness_init(void);
#else
/** Empty stub. Does nothing. */
#define se_randomness_init() {};
#endif

/**
Samples sizeof(ZZ) bytes from the udev device using getrandom().

@returns A random value of type ZZ
*/
static inline ZZ random_zz(void)
{
    ZZ result;
#ifdef SE_RAND_GETRANDOM
    ssize_t ret = getrandom((void *)&result, sizeof(result), 0);
    se_assert(ret == sizeof(result));
#elif defined(SE_RAND_NRF5)
    ret_code_t ret_val = nrf_crypto_rng_vector_generate((uint8_t *)&result, sizeof(ZZ));
    // printf("val: %d\n", result);
    // if (result < 1) printf("something possibly went wrong...\n");
    if (ret_val != NRF_SUCCESS)
    {
        printf("Error: Something went wrong with nrf_crypto_rng_vector_generate()\n");
        while (1)
            ;
    }
    se_assert(ret_val == NRF_SUCCESS);
#else
    // -- NOTE: THIS IS A CONST (NON RANDOM) NUMBER!
    //    Should only be used for debug
    result = 0x12345678;
#endif
    return result;
}

/**
Samples 1 byte from the udev device using getrandom().

@returns A random uint8
*/
static inline uint8_t random_uint8(void)
{
    uint8_t result;
#ifdef SE_RAND_GETRANDOM
    ssize_t ret = getrandom((void *)&result, sizeof(result), 0);
    se_assert(ret == sizeof(result));
#elif defined(SE_RAND_NRF5)
    ret_code_t ret_val = nrf_crypto_rng_vector_generate(&result, 1);
    if (ret_val != NRF_SUCCESS)
    {
        printf("Error: Something went wrong with nrf_crypto_rng_vector_generate()\n");
        while (1)
            ;
    }
    se_assert(ret_val == NRF_SUCCESS);
#else
    // -- NOTE: THIS IS A CONST (NON RANDOM) NUMBER!
    //    Should only be used for debug
    result = 0x7;
#endif
    return result;
}

/**
Samples 8 bytes from the udev device using getrandom().

@returns A random double
*/
static inline double random_double(void)
{
    double result;
#ifdef SE_RAND_GETRANDOM
    ssize_t ret = getrandom((void *)&result, sizeof(result), 0);
    se_assert(ret == sizeof(result));
#elif defined(SE_RAND_NRF5)
    ret_code_t ret_val = nrf_crypto_rng_vector_generate((uint8_t *)&result, sizeof(double));
    if (ret_val != NRF_SUCCESS)
    {
        printf("Error: Something went wrong with nrf_crypto_rng_vector_generate()\n");
        while (1)
            ;
    }
    se_assert(ret_val == NRF_SUCCESS);
#else
    // -- NOTE: THIS IS A CONST (NON RANDOM) NUMBER!
    //    Should only be used for debug
    result = 1234;
#endif
    return result;
}

// ----------------------------------------------------
//                       Uniform
// ----------------------------------------------------
/**
Samples a polynomial with coefficients from the uniform distribution over [0, q).
Used to sample the second element of a ciphertext for symmetric encryption.
Internally samples from the udev device using getrandom(). Uses rejection sampling.

Space req: 'poly' must have space for n ZZ elements.

@param[in]      parms  Parameters set by ckks_setup
@param[in,out]  prng   A prng instance to generate the randomness
@param[out]     poly   The sampled polynomial.
*/
void sample_poly_uniform(const Parms *parms, SE_PRNG *prng, ZZ *poly);

// ----------------------------------------------------
//                       Ternary
// ----------------------------------------------------
/**
Sets the value of the (idx)-th coefficient of 'poly' directly in small (compressed) form.

Note: val_in should be either 0, 1, or 2 (which represent -1, 0, and 1, in some order)

@param[in]  idx     Requested coefficient to set
@param[in]  val_in  Value to write to (idx)-th coefficient of 'poly'
@param[out] poly    A ternary polynomial in small (compressed) form
*/
void set_small_poly_idx(size_t idx, uint8_t val_in, ZZ *poly);

/**
Returns the value of the (idx)-th coefficient of 'poly', leaving 'poly' and result in small form.

@param[in] poly  A ternary polynomial in small (compressed) form
@param[in] idx   Requested coefficient to read
@returns         Value of requested coefficient of 'poly' in small form
*/
uint8_t get_small_poly_idx(const ZZ *poly, size_t idx);

/**
Returns the value of the (idx)-th coefficient of 'poly', leaving 'poly' in small form.

@param[in] poly  A ternary polynomial in small (compressed) form
@param[in] idx   Requested coefficient to read
@param[in] q     Modulus value
@returns         Value of requested coefficient of 'poly' in expanded form
*/
ZZ get_small_poly_idx_expanded(const ZZ *poly, size_t idx, ZZ q);

/**
Expands a small (compressed) ternary polynomial w.r.t. the current modulus and places the result in
`dest'. 'dest' and 'src' memory may share the same starting address, since expansion occurs
backwards (see: 'in place' version below).

Note: This function does not keep track of 'small_u' or 'small_s' flag. These flags must be
tracked/updated outside of this function if necessary. Space req: 'dest' must have space for n ZZ
elements.

@param[in]  src    Source polynomial in small form
@param[in]  parms  Parameters instance
@param[out] dest   Destination polynomial in expanded form.
*/
void expand_poly_ternary(const ZZ *src, const Parms *parms, ZZ *dest);

/**
Expands a small (compressed) ternary polynomial in place w.r.t. the current modulus.

Note: This function does not keep track of 'small_u' or 'small_s' flag. These flags must be
tracked/updated outside of this function if necessary. Space req: 'poly' must have space for n ZZ
elements.

@param[in,out] poly   In: Source polynomial in small form; Out: result polynomial in expanded form
@param[in]     parms  Parameters instance
*/
void expand_poly_ternary_inpl(ZZ *poly, const Parms *parms);

/**
Reduces an (expanded) ternary polynomial w.r.t. the current modulus and places the result in `dest'.
'dest' and 'src' memory may share the same starting address (see: 'in place' version below).

Space req: 'dest' must have space for n ZZ elements.

@param[in]  src    Source polynomial (reduced modulo previous modulus)
@param[in]  parms  Parameters instance
@param[out] dest   Destination polynomial (reduced modulo current modulus)
*/
void convert_poly_ternary(const ZZ *src, const Parms *parms, ZZ *dest);

/**
In-place reduces an (expanded) ternary polynomial w.r.t. the current modulus.

Space req: 'poly' must have space for n ZZ elements.

@param[in,out] poly   In: Polynomial to convert; Out: converted polynomial
@param[in]     parms  Parameters instance
*/
void convert_poly_ternary_inpl(ZZ *poly, const Parms *parms);

/**
Samples an (expanded) polynomial from the uniform ternary distribution over {-q-1, 0, 1}, where q is
the value of the current modulus. This function is mainly useful for testing, since most of the time
we would want to sample the polynomial in compressed form (see: sample_small_poly_ternary). PRNG
instance is used to generate randomness for 1 coefficient at a time.

Space req: 'poly' must have space for n ZZ values

@param[in]     parms  Parameters instance
@param[in,out] prng   PRNG instance (counter will be updated)
@param[out]    poly   Output sampled polynomial
*/
void sample_poly_ternary(const Parms *parms, SE_PRNG *prng, ZZ *poly);

/**
Samples a small (compressed) polynomial from the uniform ternary distribution over {-q-1, 0, 1},
where q is the value of the current modulus. Note that this does *not* assume the input buffer is in
small form. It assumes that the input buffer has space for n uint8_t values. PRNG instance is used
to generate randomness for 1 coefficient at a time.

Space req: 'poly' must have space for n uin8_t values

@param[in]     n     Number of coefficients to sample (e.g. degree of 'poly')
@param[in,out] prng  PRNG instance (counter will be updated)
@param[out]    poly  Result sampled polynomial
*/
void sample_small_poly_ternary(PolySizeType n, SE_PRNG *prng, ZZ *poly);

/**
Samples a small (compressed) polynomial from the uniform ternary distribution over {-q-1, 0, 1},
where q is the value of the current modulus, while leaving the polynomial in compressed form. PRNG
instance is used to generate randomness for k coefficients at a time (prior to any required
rejection re-sampling, which occurs 1 coefficient at a time).

Space req: 'poly' must have space for n uin8_t values

@param[in]     k     Number of coefficients to generate randomness for at once
@param[in]     n     Number of coefficients to sample (e.g. degree of 'poly')
@param[in,out] prng  PRNG instance (counter will be updated)
@param[out]    poly  Result sampled polynomial
*/
// void sample_small_poly_ternary_prng_k(size_t k, PolySizeType n, SE_PRNG *prng, ZZ *poly);

/**
Samples a small (compressed) polynomial from the uniform ternary distribution over {-q-1, 0, 1},
where q is the value of the current modulus, while leaving the polynomial in compressed form. PRNG
instance is used to generate randomness for 96 coefficients at a time (prior to any required
rejection re-sampling, which occurs 1 coefficient at a time).

Space req: 'poly' must have space for n uin8_t values

@param[in]     n     Number of coefficients to sample (e.g. degree of 'poly')
@param[in,out] prng  PRNG instance (counter will be updated)
@param[out]    poly  Result sampled polynomial
*/
void sample_small_poly_ternary_prng_96(PolySizeType n, SE_PRNG *prng, ZZ *poly);

// ------------------------------------------------------
//                   Centered Binomial
// ------------------------------------------------------

/**
Samples coefficients of a polynomial from a centered binomial distribution (non-modulo-reduced).
PRNG instance is used to sample 1 coefficient at a time.

@param[in]     n     Number of coefficients to sample (e.g. degree of 'poly')
@param[in,out] prng  PRNG instance (will update counter)
@param[out]    poly  Result sampled polynomial
*/
void sample_poly_cbd_generic(PolySizeType n, SE_PRNG *prng, int8_t *poly);

/**
Samples coefficients of a polynomial from a centered binomial distribution (non-modulo-reduced).
PRNG instance is used to generate randomness for k coefficients at a time.

@param[in]     k     Number of coefficients to generate randomness for at once
@param[in]     n     Total number of coefficients to sample (e.g. degree of 'poly')
@param[in,out] prng  PRNG instance (will update counter)
@param[out]    poly  Result sampled polynomial
*/
// void sample_poly_cbd_generic_prng_k(size_t k, PolySizeType n, SE_PRNG *prng, int8_t *poly);

/**
Samples coefficients of a polynomial from a centered binomial distribution (non-modulo-reduced).
PRNG instance is used to generate randomness for 16 coefficients at a time.

@param[in]     n     Number of coefficients to sample (e.g. degree of 'poly')
@param[in,out] prng  PRNG instance (will update counter)
@param[out]    poly  Result sampled polynomial
*/
void sample_poly_cbd_generic_prng_16(PolySizeType n, SE_PRNG *prng, int8_t *poly);

/**
Samples coefficients of a polynomial from a centered binomial distribution (non-modulo-reduced) and
adds them in-place to the polynomial 'poly'. PRNG instance is used to generate randomness for 1
coefficient at a time.

@param[in, out] poly  In: Initial polynomial; Out: Initial polynomial + cbd error polynomial
@param[in]      n     Number of coefficients to sample (e.g. degree of 'poly')
@param[in,out]  prng  PRNG instance (will update counter)
*/
void sample_add_poly_cbd_generic_inpl(int64_t *poly, PolySizeType n, SE_PRNG *prng);

/**
Samples coefficients of a polynomial from a centered binomial distribution (non-modulo-reduced) and
adds them in-place to the polynomial 'poly'. PRNG instance is used to generate randomness for k
coefficients at a time.

@param[in, out] poly  In: Initial polynomial; Out: Initial polynomial + cbd error polynomial
@param[in]      k     Number of coefficients to generate randomness for at once
@param[in]      n     Total number of coefficients to sample (e.g. degree of 'poly')
@param[in,out]  prng  PRNG instance (will update counter)
*/
// void sample_add_poly_cbd_generic_inpl_prng_k(int64_t *poly, size_t k, PolySizeType n, SE_PRNG
// *prng);

/**
Samples coefficients of a polynomial from a centered binomial distribution (non-modulo-reduced) and
adds them in-place to the polynomial 'poly'. PRNG instance is used to generate randomness for 16
coefficients at a time.

@param[in, out] poly  In: Initial polynomial; Out: Initial polynomial + cbd error polynomial
@param[in]      n     Number of coefficients to sample (e.g. degree of 'poly')
@param[in,out]  prng  PRNG instance (will update counter)
*/
void sample_add_poly_cbd_generic_inpl_prng_16(int64_t *poly, PolySizeType n, SE_PRNG *prng);

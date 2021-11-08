// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file sample.c
*/

#include "sample.h"

#include "defines.h"
#include "parameters.h"
#include "util_print.h"  // TODO: Including this breaks things?

#ifdef SE_RAND_NRF5
#include "nrf_crypto.h"
#endif

//#define DEBUG_EASYMOD

// ----------------------------  Randomness ------------------------------

#ifdef SE_RAND_NRF5
void se_randomness_init()
{
    // -- Initialize crypto library.
    ret_code_t ret_val = nrf_crypto_init();
    if (ret_val != NRF_SUCCESS)
    {
        printf("Error: Something went wrong with nrf_crypto_init\n");
        while (1)
            ;
    }
    se_assert(ret_val == NRF_SUCCESS);
}
#endif

// -----------------------------  Uniform ---------------------------------

void sample_poly_uniform(const Parms *parms, SE_PRNG *prng, ZZ *poly)
{
    PolySizeType n   = parms->coeff_count;
    const Modulus *q = parms->curr_modulus;

    // -- We sample numbers up to 2^32-1
    ZZ max_random   = (ZZ)0xFFFFFFFFUL;
    ZZ max_multiple = max_random - barrett_reduce_32input_32modulus(max_random, q) - 1;

    prng_fill_buffer(n * sizeof(ZZ), prng, (void *)poly);

    for (size_t i = 0; i < n; i++)
    {
        // -- Rejection sampling
        ZZ rand_val = poly[i];
        while (rand_val >= max_multiple) { prng_fill_buffer(sizeof(ZZ), prng, (void *)&rand_val); }
        poly[i] = barrett_reduce_32input_32modulus(rand_val, q);
    }
}

// ----------------------------  Ternary ---------------------------------

void set_small_poly_idx(size_t idx, uint8_t val_in, ZZ *poly)
{
    se_assert(val_in <= 2);

    // -- This isn't really necessary if inputs are correct
    val_in &= 0x3;

    // -- Every byte can hold 4 samples
    uint8_t *poly_small = (uint8_t *)(&(poly[0]));
    size_t byte_num     = idx / 4;
    size_t byte_pos     = idx % 4;  // 0, 1, 2, or 3

    uint8_t byte_in  = poly_small[byte_num];
    uint8_t get_mask = (uint8_t)(~(0x3 << (6 - byte_pos * 2)));
    uint8_t old_val  = byte_in & get_mask;
    uint8_t new_val  = (uint8_t)(old_val | (val_in << (6 - byte_pos * 2)));

    // -- Debugging
    // printf("idx:         %zu, val_in:      %"PRIu8"\n", idx, val_in);
    // printf("byte_num:    %zu, byte_pos:    %zu\n", byte_num, byte_pos);

    poly_small[byte_num] = new_val;

    // -- Debugging
    // uint8_t byte_out = poly_small[byte_num];
    // printf("b_in:     0x%x, b_out:    0x%x\n\n", byte_in, byte_out);
}

uint8_t get_small_poly_idx(const ZZ *poly, size_t idx)
{
    const uint8_t *poly_small = (uint8_t *)(&(poly[0]));
    size_t byte_num           = idx / 4;
    size_t byte_pos           = idx % 4;  // 0, 1, 2, or 3
    uint8_t byte_val          = poly_small[byte_num];
    return (byte_val >> (6 - byte_pos * 2)) & 0x3;
}

ZZ get_small_poly_idx_expanded(const ZZ *poly, size_t idx, ZZ q)
{
    uint8_t val = get_small_poly_idx(poly, idx);
    // print_zz("val", val); // Debugging
    se_assert(val <= 2);
#ifdef DEBUG_EASYMOD
    // -- If debugging, want an easy mapping: 0 -> 0, 1 -> 1, 2 -> q-1
    return (val < 2) ? val : (q - 1);
#else
    // -- Mapping: 0 -> q-1, 1 -> 0, 2 -> 1 (see explanation below)
    // return (!val) ? (q-1) : (ZZ)(val - 1);
    return val + ((-(ZZ)(val == 0)) & q) - 1;
#endif
}

void expand_poly_ternary(const ZZ *src, const Parms *parms, ZZ *dest)
{
    PolySizeType n = parms->coeff_count;
    ZZ q           = parms->curr_modulus->value;

    // -- Debugging
    // print_poly_ternary_full("src", src, n, true);

    // -- Fill in back first so we don't overwrite any values
    // -- Need to be careful with overflow and backwards loops
    for (size_t i = n; i > 0; i--)
    {
        // printf("current idx: %zu\n", i-1); // debugging
        ZZ val      = get_small_poly_idx_expanded(src, i - 1, q);
        dest[i - 1] = val;
    }
}

void expand_poly_ternary_inpl(ZZ *poly, const Parms *parms)
{
    // -- Debugging
    // print_poly_ternary_full("src", poly, parms->coeff_count, true);
    expand_poly_ternary(poly, parms, poly);
}

void convert_poly_ternary(const ZZ *src, const Parms *parms, ZZ *dest)
{
    PolySizeType n = parms->coeff_count;
    ZZ q_m1        = parms->curr_modulus->value - 1;

    if (src != dest) memcpy(dest, src, n * sizeof(ZZ));
    for (size_t i = 0; i < n; i++)
    {
        if (src[i] > 1) dest[i] = q_m1;
    }
}

void convert_poly_ternary_inpl(ZZ *poly, const Parms *parms)
{
    convert_poly_ternary(poly, parms, poly);
}

void sample_poly_ternary(const Parms *parms, SE_PRNG *prng, ZZ *poly)
{
    PolySizeType n = parms->coeff_count;
    ZZ q           = parms->curr_modulus->value;

    // -- Follow same format as sample_poly_uniform, but with modulus of 3
    //    Uniform over [-1, 1] ---> Uniform over [0, 2]
    //    We know that max_random = 0xFFFF_FFFFULL = 0 (mod 3)
    //    So, max_multiple = max_random - (max_random % 3) - 1
    //                     = max_random - 1
    //                     = 0xFFFF_FFFEULL
    ZZ max_multiple = (ZZ)0xFFFFFFFEUL;

    prng_fill_buffer(n * sizeof(ZZ), prng, (void *)poly);

    for (size_t i = 0; i < n; i++)
    {
        // -- Rejection sampling
        ZZ rand_val = poly[i];
        while (rand_val >= max_multiple) { prng_fill_buffer(sizeof(ZZ), prng, (void *)&rand_val); }
#ifdef DEBUG_EASYMOD
        // -- If debugging, want an easy mapping: 0 -> 0, 1 -> 1, 2 -> q-1
        //    We also don't need constant-time
        ZZ rand_ternary = rand_val % 3;
        poly[i]         = (rand_ternary > 1) ? q - 1 : rand_ternary;
#else
        // -- If not debugging, we want a constant time version of mod 3.
        //    This leads to a mapping of: 0 -> q-1, 1 -> 0, 2 -> 1
        //    This mapping matches the mapping in SEAL
        uint8_t rand_ternary = mod3_zzinput(rand_val);
        poly[i]              = rand_ternary + ((-(ZZ)(rand_val == 0)) & q) - 1;
#endif
    }
}

/*
void sample_small_poly_ternary_prng_k(size_t k, PolySizeType n, SE_PRNG *prng, ZZ *poly)
{
    se_assert(k <= n);
    se_assert(prng && poly);
    uint8_t max_multiple = (uint8_t)(0xFE);
    for (size_t j = 0; j < n; j += k)
    {
        uint8_t buffer[k];
        prng_fill_buffer(k, prng, &(buffer[0]));

        size_t i_stop = ((j + k - 1) < n) ? k : (n - j);

        for (size_t i = 0; i < i_stop; i++)
        {
            uint8_t rand_val = buffer[i];
            while (rand_val >= max_multiple) { prng_fill_buffer(1, prng, (void *)&rand_val); }
#ifdef DEBUG_EASYMOD
            uint8_t rand_ternary = rand_val % 3;
#else
            uint8_t rand_ternary = mod3_uint8input(rand_val);
#endif
            set_small_poly_idx(i + j, rand_ternary, poly);
        }
    }
}
*/

void sample_small_poly_ternary_prng_96(PolySizeType n, SE_PRNG *prng, ZZ *poly)
{
    se_assert(96 <= n);
    se_assert(prng && poly);
    uint8_t max_multiple = (uint8_t)(0xFE);
    for (size_t j = 0; j < n; j += 96)
    {
        uint8_t buffer[96];
        prng_fill_buffer(96, prng, &(buffer[0]));

        size_t i_stop = ((j + 95) < n) ? 96 : (n - j);

        for (size_t i = 0; i < i_stop; i++)
        {
            uint8_t rand_val = buffer[i];
            while (rand_val >= max_multiple) { prng_fill_buffer(1, prng, (void *)&rand_val); }
#ifdef DEBUG_EASYMOD
            uint8_t rand_ternary = rand_val % 3;
#else
            uint8_t rand_ternary = mod3_uint8input(rand_val);
#endif
            set_small_poly_idx(i + j, rand_ternary, poly);
        }
    }
}

// ---------------------------  Centered Binomial -----------------------------
// -- A binomial distribution with parameter k has standard deviation \sqrt{k/2}.
//    Each sample requires 2k random bits computed as \sum_{i=0}^{k-1}{a_i - b_i}
//    where a_i and b_i are two random bits. Alternatively, it can be represented
//    as the difference between the Hamming weights of two k-bit uniform random
//    values. The HE security standard (homomorphicencryption.org) suggests a
//    standard deviation value of >= 3.2 This translates to k >= 21. We choose k=21,
//    resulting in a standard deviation value of 3.24.
//
//    When this is used, noise should be interpreted modulo the modulus.
//    That is, when noise is negative, must convert it to modulus - |noise| instead.
//    However, we never actually need to reduce the error polynomial in this library.

/**
Helper function to return the hamming weight of a byte value

@param[in] value  A byte-sized value
@returns          Hamming weight of value
*/
static inline int8_t hamming_weight(uint8_t value)
{
    int t = (int)value;
    t -= (t >> 1) & 0x55;
    t = (t & 0x33) + ((t >> 2) & 0x33);
    return (int8_t)(t + (t >> 4)) & 0x0F;
}

/**
Helper function to get the cbd sample (from a cbd w/ stddev 3.24) from 6 random bytes (non-modulo
reduced)

@param[in] x  Six random bytes
@returns      A (non-modulo-reduced) cbd sample
*/
static inline int8_t get_cbd_val(uint8_t x[6])
{
    x[2] &= 0x1F;
    x[5] &= 0x1F;
    return (int8_t)(hamming_weight(x[0]) + hamming_weight(x[1]) + hamming_weight(x[2]) -
                    hamming_weight(x[3]) - hamming_weight(x[4]) - hamming_weight(x[5]));
}

void sample_poly_cbd_generic(PolySizeType n, SE_PRNG *prng, int8_t *poly)
{
    // -- Every 42 bits (6 bytes) generates a sample
    uint8_t buffer[6];
    for (size_t i = 0; i < n; i++)
    {
        prng_fill_buffer(6, prng, (void *)buffer);
        poly[i] = get_cbd_val(buffer);
    }
}

/*
void sample_poly_cbd_generic_prng_k(size_t k, PolySizeType n, SE_PRNG *prng, int8_t *poly)
{
    for (size_t j = 0; j < n; j += k)
    {
        // -- Every 42 bits (6 bytes) generates a sample
        uint8_t buffer[6 * k]; // Generate k samples at once
        prng_fill_buffer(6 * k, prng, (void *)buffer);

        for (size_t i = 0; i < k; i++) { poly[i + j] = get_cbd_val(buffer + 6 * i); }
    }
}
*/

void sample_poly_cbd_generic_prng_16(PolySizeType n, SE_PRNG *prng, int8_t *poly)
{
    for (size_t j = 0; j < n; j += 16)
    {
        // -- Every 42 bits (6 bytes) generates a sample
        uint8_t buffer[96];  // Generate 16 samples at once (6 * 16 = 96 bytes)
        prng_fill_buffer(96, prng, (void *)buffer);

        for (size_t i = 0; i < 16; i++) { poly[i + j] = get_cbd_val(buffer + 6 * i); }
    }
}

void sample_add_poly_cbd_generic_inpl(int64_t *poly, PolySizeType n, SE_PRNG *prng)
{
    // -- Every 42 bits (6 bytes) generates a sample
    uint8_t buffer[6];
    for (size_t i = 0; i < n; i++)
    {
        prng_fill_buffer(6, prng, (void *)buffer);
        poly[i] += get_cbd_val(buffer);
    }
}

/*
void sample_add_poly_cbd_generic_inpl_prng_k(int64_t *poly, size_t k, PolySizeType n, SE_PRNG *prng)
{
    for (size_t j = 0; j < n; j += k)
    {
        // -- Every 42 bits (6 bytes) generates a sample
        uint8_t buffer[6 * k]; // Generate k samples at once
        prng_fill_buffer(6 * k, prng, (void *)buffer);
        for (size_t i = 0; i < k; i++) { poly[i + j] += get_cbd_val(buffer + 6 * i); }
    }
}
*/

void sample_add_poly_cbd_generic_inpl_prng_16(int64_t *poly, PolySizeType n, SE_PRNG *prng)
{
    for (size_t j = 0; j < n; j += 16)
    {
        // -- Every 42 bits (6 bytes) generates a sample
        uint8_t buffer[96];  // Generate 16 samples at once (6 * 16 = 96 bytes)
        prng_fill_buffer(96, prng, (void *)buffer);
        for (size_t i = 0; i < 16; i++) { poly[i + j] += get_cbd_val(buffer + 6 * i); }
    }
}

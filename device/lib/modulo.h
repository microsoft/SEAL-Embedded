// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file modulo.h
*/

#pragma once

#include "defines.h"
#include "modulus.h"  // Modulus
#include "uintops.h"  // multiply_uint64

/**
Constant-time shift of input from [0, 2q) to [0, q)

@param[in] input  Input in [0, 2q)
@param[in] q      Modulus value
@returns          Result in [0, q)
*/
static inline ZZ shift_result(ZZ input, ZZ q)
{
    // -- If input is in [0, 2q) instead of [0, q), is_2q = 1 (case 1)
    //    Otherwise, is_2q = 0 (case 2)
    ZZsign is_2q = (ZZsign)(input >= q);

    // -- If case 1, mask = all 1s; if case 2, mask = all 0s;
    ZZ mask = (ZZ)(-is_2q);

    // -- Use mask to subtract q if necessary
    return (ZZ)(input) - (q & mask);
}

/**
Reduces input using base 2^32 Barrett reduction

Req: modulus must be at most 31 bits

@param[in] input    32-bit input
@param[in] modulus  Modulus object with 31-bit value
@returns            Result of input mod q in a uint32_t
*/
static inline uint32_t barrett_reduce_32input_32modulus(uint32_t input, const Modulus *modulus)
{
    // -- (x = input)
    // -- (q = modulus->value)
    // -- We want to calculate [x]q (i.e. x mod q).
    //    we will use barrett reduction to calculate r
    //      where r = [x]q or [x]2q
    //    the formula for this is:
    //      r = [ [x]b - [floor(x*u/t) * q]b ]b
    //    where u = floor(t/q), b = 2^64, t = 2^64

    uint32_t tmp;
    // -- We have modulus->const_ratio = floor(2^128/q).
    //    We can get floor(2^64/q) from floor(2^128/q) by taking the upper 64-bits.
    //    So, x*u = x * floor(2^64/q)
    //            = x * floor((2^128/2^64) * 1/q)
    //            = x * upper_64_bits(floor(2^128/q))
    //            = x * modulus->const_ratio[1]
    //    Note that we only need the upper 64 bits of temp (see below)
    tmp = mul_uint32_high(input, modulus->const_ratio[1]);

    // -- Barrett subtraction. We use the same technique as above once more:
    //      floor(x*u/t) * q = floor(tmp/2^64) * q
    //                       = upper_64_bits(tmp) * q
    //    Since we actually want [floor(x*u/t) * q]b, we can just take lower bits

    // -- These are the same because q is bounded by 2^31 - 1
    tmp = input - tmp * modulus->value;
    // tmp = input - mul_uint32_low(tmp, modulus->value);

    // -- Make sure result is in [0, q) instead of [0, 2q)
    return shift_result(tmp, modulus->value);
}

/**
Reduces input using constant-time base 2^32 Barrett reduction

@param[in] input    64-bit input
@param[in] modulus  Modulus object with 32-bit value
@returns            32-bit result of input mod q
*/
static inline uint32_t barrett_reduce_64input_32modulus(const uint32_t *input,
                                                        const Modulus *modulus)
{
    const uint32_t *const_ratio = modulus->const_ratio;

    // -- The following code is essentially multiplying input (128-bit)
    //    with modulus->const_ratio (63-bit--> ~64 bits), but is optimized
    //    to only calculate the highest word of the 192-bit result since
    //    it is equivalent to the reduced result.
    //    (hw = high_word, lw = low_word)

    // -- Round 1
    uint32_t right_hw = mul_uint32_high(input[0], const_ratio[0]);

    uint32_t middle_temp[2];
    mul_uint32_wide(input[0], const_ratio[1], middle_temp);
    uint32_t middle_lw;
    uint32_t middle_lw_carry = add_uint32(right_hw, middle_temp[0], &middle_lw);
    uint32_t middle_hw       = middle_temp[1] + middle_lw_carry;

    // -- Round 2
    uint32_t middle2_temp[2];
    mul_uint32_wide(input[1], const_ratio[0], middle2_temp);
    uint32_t middle2_lw;
    uint32_t middle2_lw_carry = add_uint32(middle_lw, middle2_temp[0], &middle2_lw);
    uint32_t middle2_hw       = middle2_temp[1] + middle2_lw_carry;  // We don't need the carry

    uint32_t tmp = input[1] * const_ratio[1] + middle_hw + middle2_hw;

    // -- Barrett subtraction
    tmp = input[0] - tmp * modulus->value;
    return shift_result(tmp, modulus->value);
}

/**
Reduces a 2B-bit input using constant-time base 2^B Barrett reduction for a B-bit modulus
(B = 32 if ZZ = uint32).

@param[in] input    (2*B)-bit input to reduce
@param[in] modulus  Modulus object with B-bit value q
@returns            B-bit result of input mod q
*/
static inline ZZ barrett_reduce_wide(const ZZ *input, const Modulus *modulus)
{
    return barrett_reduce_64input_32modulus(input, modulus);
}

/**
Reduces a B-bit input using constant-time base 2^B Barrett reduction for a B-bit modulus
(B = 32 if ZZ = uint32).

@param[in] input    B-bit input to reduce
@param[in] modulus  Modulus object with B-bit value q
@returns            B-bit result of input mod q
*/
static inline ZZ barrett_reduce(const ZZ input, const Modulus *modulus)
{
    return barrett_reduce_32input_32modulus(input, modulus);
}

/**
Optimized constant-time modulo 3 reduction for an 8-bit unsigned integer input

@param[in] r  8-bit unsigned integer input
@returns     'r' mod 3
*/
static inline uint8_t mod3_uint8input(uint8_t r)
{
    // r        = (r >> 4) + (r & 0xf);  // r' = r mod 3, since 2^4=1
    // r        = (r >> 2) + (r & 0x3);  // r'= r mod 3, since 2^2=1
    // r        = (r >> 2) + (r & 0x3);  // r'= r mod 3, since 2^2=1, reducing r to [0, 3]
    // int8_t t = r - 3;                 // 0,1,2 --> 0xF?, 3 --> 0x00
    // int8_t c = t >> 7;                // 0xF? --> 0x01, 0x00 --> 0x00
    // return (c & r) ^ ((~c) & t);
    r        = (uint8_t)((r >> 4) + (r & 0xf));  // r' = r mod 3, since 2^4=1
    r        = (uint8_t)((r >> 2) + (r & 0x3));  // r'= r mod 3, since 2^2=1
    r        = (uint8_t)((r >> 2) + (r & 0x3));  // r'= r mod 3, since 2^2=1, reducing r to [0, 3]
    int8_t t = (int8_t)(r - 3);                  // 0,1,2 --> 0xF?, 3 --> 0x00
    int8_t c = t >> 7;                           // 0xF? --> 0x01, 0x00 --> 0x00
    return (uint8_t)((c & r) ^ ((~c) & t));
}

/**
Constant-time modulo 3 reduction

@param[in] input  Input to be reduced
@returns          'input' mod 3
*/
static inline uint8_t mod3_zzinput(uint32_t input)
{
    uint32_t r;
    r = (input >> 16) + (input & 0xffff);  // r' = r mod 3, since 2^16=1
    r = (r >> 8) + (r & 0xff);             // r' = r mod 3, since 2^8=1
    r = (r >> 4) + (r & 0xf);              // r' = r mod 3, since 2^4=1
    r = (r >> 2) + (r & 0x3);              // r'= r mod 3, since 2^2=1
    r = (r >> 2) + (r & 0x3);              // r'= r mod 3, since 2^2=1, reducing r to [0, 3]
    // int8_t t = (uint8_t)r - 3; // 0,1,2 --> 0xF?, 3 --> 0x00
    // int8_t c = t >> 7; // 0xF? --> 0x01, 0x00 --> 0x00
    // return (c & (uint8_t)r) ^ ((~c) & t);
    int8_t t = (int8_t)((uint8_t)r - 3);  // 0,1,2 --> 0xF?, 3 --> 0x00
    int8_t c = t >> 7;                    // 0xF? --> 0x01, 0x00 --> 0x00
    return (uint8_t)((c & (uint8_t)r) ^ ((~c) & t));
}

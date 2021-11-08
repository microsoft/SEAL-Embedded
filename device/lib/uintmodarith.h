// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file uintmodarith.h
*/

#pragma once

#include <stdint.h>  // uint64_t

#include "defines.h"
#include "modulo.h"  // barrett_reduce_128, shift_result
#include "uint_arith.h"
#include "uintops.h"  // mul_uint64
#include "util_print.h"

/**
Modular addition. Correctness: (op1 + op2) <= (2q - 1).

@param[in] op1  Operand 1
@param[in] op2  Operand 2
@param[in] q    Modulus. Must be < 2^32 (if ZZ == uint32)
@returns        (op1 + op2) mod q
*/
static inline ZZ add_mod(ZZ op1, ZZ op2, const Modulus *q)
{
    ZZ q_val = q->value;
    se_assert((op1 + op2) <= (2 * q_val - 1));

    // -- We know that the sum can fit into a uint32:
    //      max(uint32_t) = 2^32 - 1
    //      max(q) = 2^32 - 1
    //      max(op1 + op2) = 2*max(q) - 1
    //                     = 2*(2^32 - 1) - 1
    //                     = 2^33 - 3
    //                     < max(uint32_t)

    ZZ sum;
    add_uint_nocarry(op1, op2, &sum);  // We don't need the carry
    return shift_result(sum, q_val);
}

/**
In-place modular addition. Correctness: (op1 + op2) <= (2q - 1).

@param[in,out] op1  In: Operand 1; Out: (op1 + op2) mod q
@param[in]     op2  Operand 2
@param[in]     q    Modulus. Must be < 2^32 (if ZZ == uint32)
*/
static inline void add_mod_inpl(ZZ *op1, ZZ op2, const Modulus *q)
{
    se_assert(op1 && ((op1[0] + op2) <= (2 * q->value - 1)));
    op1[0] = add_mod(op1[0], op2, q);
}

/**
Modular negation.

@param[in] op  Operand. Must <= q.
@param[in] q   Modulus
@returns       (-op) mod q
*/
static inline ZZ neg_mod(ZZ op, const Modulus *q)
{
    se_assert(op <= q->value);
    ZZsign non_zero = (ZZsign)(op != 0);
    ZZ mask         = (ZZ)(-non_zero);

    // -- If op == 0, return 0;
    //    Else, result is q - op (if op < q)
    return (q->value - op) & mask;
}

/**
In-place modular negation.

@param[in,out] op  In: Operand. Must be <= q; Out: (-op) mod q
@param[in]     q   Modulus
*/
static inline void neg_mod_inpl(ZZ *op, const Modulus *q)
{
    se_assert(op && op[0] <= q->value);
    op[0] = neg_mod(op[0], q);
}

/**
Modular subtraction.

@param[in] op1  Operand 1. Must be <= q.
@param[in] op2  Operand 2. Must be <= q.
@param[in] q    Modulus
@returns        (op1 - op2) mod q
*/
static inline ZZ sub_mod(ZZ op1, ZZ op2, const Modulus *q)
{
    se_assert(op1 <= q->value && op2 <= q->value);
    ZZ negated = neg_mod(op2, q);
    return add_mod(op1, negated, q);
}

/**
In-place modular subtraction.

@param[in,out] op1  In: Operand 1. Must be <= q; Out: (op1 - op2) mod q
@param[in]     op2  Operand 2. Must be <= q
@param[in]     q    Modulus
*/
static inline void sub_mod_inpl(ZZ *op1, ZZ op2, const Modulus *q)
{
    se_assert(op1 && op1[0] <= q->value && op2 <= q->value);
    add_mod_inpl(op1, neg_mod(op2, q), q);
}

/**
Modular multiplication using Barrett reduction.

@param[in] op1  Operand 1
@param[in] op2  Operand 2
@param[in] q    Modulus
@returns        (op1 * op2) mod q
*/
static inline ZZ mul_mod(ZZ op1, ZZ op2, const Modulus *q)
{
    ZZ product[2];
    mul_uint_wide(op1, op2, product);
    return barrett_reduce_wide(product, q);
}

/**
In-place modular multiplication using Barrett reduction.

@param[in,out] op1  In: Operand 1; Out: (op1 * op2) mod q
@param[in]     op2  Operand 2
@param[in]     q    Modulus
*/
static inline void mul_mod_inpl(ZZ *op1, ZZ op2, const Modulus *q)
{
    se_assert(op1);
    op1[0] = mul_mod(op1[0], op2, q);
}

/**
Modular multiplication of two values followed by a modular addion of a third value.

@param[in] op1  Operand 1
@param[in] op2  Operand 2
@param[in] op3  Operand 3
@param[in] q    Modulus
@returns        (op1 + (op2 * op3) mod q) mod q
*/
static inline ZZ mul_add_mod(ZZ op1, ZZ op2, ZZ op3, const Modulus *q)
{
    return add_mod(op1, mul_mod(op2, op3, q), q);
}

/**
In-place modular multiplication of two values followed by a modular addition of a third value.

@param[in,out] op1  In: Operand 1; Out: (op1 + (op2 * op3) mod q) mod q
@param[in]     op2  Operand 2
@param[in]     op3  Operand 3
@param[in]     q    Modulus
*/
static inline void mul_add_mod_inpl(ZZ *op1, ZZ op2, ZZ op3, const Modulus *q)
{
    add_mod_inpl(op1, mul_mod(op2, op3, q), q);
}

/**
Exponentiates a ZZ-type value with respect to a modulus. Prior to exponentiation, bit-reverses the
value. May potentially be faster than calling bit-reverse first and exponentiate_uint_mod after.

@param[in] operand   Value to exponentiate
@param[in] exponent  Exponent to raise operand to
@param[in] logn
@param[in] mod       Modulus
@returns             [(operand)^(exponent)]_mod
*/
static inline ZZ exponentiate_uint_mod_bitrev(ZZ operand, ZZ exponent, size_t logn,
                                              const Modulus *mod)
{
    // -- Result is supposed to be only one digit

    // -- Fast cases
    if (exponent == 0) return 1;

    size_t shift_count = logn - 1;
    // printf("shift count: %zu\n", shift_count);
    if (exponent == (1 << shift_count)) return operand;

    // -- Perform binary exponentiation.
    ZZ power        = operand;
    ZZ product      = 0;
    ZZ intermediate = 1;
    // print_zz("exponent begin", exponent);

    // -- Initially: power = operand and intermediate = 1, product is irrelevant.
    while (true)
    {
        if (exponent & (ZZ)(1 << shift_count))
        {
            // printf("1 ");
            product = mul_mod(power, intermediate, mod);

            // -- Swap product and intermediate
            ZZ temp      = product;
            product      = intermediate;
            intermediate = temp;
        }
        // else printf("0 ");
        exponent &= (ZZ)(~(1 << shift_count));
        // exponent <<= 1;
        if (exponent == 0) break;
        // print_zz("exponent", exponent);
        product = mul_mod(power, power, mod);

        // -- Swap product and power
        ZZ temp = product;
        product = power;
        power   = temp;
        shift_count--;
    }
    // printf("\n");
    return intermediate;
}

/**
Exponentiates a ZZ-type value with respect to a modulus.

@param[in] operand   Value to exponentiate
@param[in] exponent  Exponent to raise operand to
@param[in] mod       Modulus
@returns             [(operand)^(exponent)]_mod
*/
static inline ZZ exponentiate_uint_mod(ZZ operand, ZZ exponent, const Modulus *mod)
{
    // -- Result is supposed to be only one digit

    // -- Fast cases
    if (exponent == 0) return 1;
    if (exponent == 1) return operand;

    // -- Perform binary exponentiation.
    ZZ power        = operand;
    ZZ product      = 0;
    ZZ intermediate = 1;
    // print_zz("exponent begin", exponent);

    // -- Initially: power = operand and intermediate = 1, product is irrelevant.
    while (true)
    {
        if (exponent & 1)
        {
            // printf("1 ");
            product = mul_mod(power, intermediate, mod);

            // -- Swap product and intermediate
            ZZ temp      = product;
            product      = intermediate;
            intermediate = temp;
        }
        // else printf("0 ");
        exponent >>= 1;
        if (exponent == 0) break;
        // print_zz("exponent", exponent);
        product = mul_mod(power, power, mod);

        // -- Swap product and power
        ZZ temp = product;
        product = power;
        power   = temp;
    }
    // printf("\n");
    return intermediate;
}

/**
This struct contains:
    - an operand
    - a precomputed quotient: floor((operand << 64)/q) = floor(operand * (2^64)/q) for a
specific modulus q.

When passed to mul_mod, a faster variant of Barrett reduction will be performed.
When used in the ntt, 'operand' stores a particular power of a root of unity.

Note: This struct is the same as "MultiplyUIntModOperand" in SEAL.
Requires: operand < q.

@param operand  The operand
@param quotient The precomputed quotient
*/
typedef struct MUMO
{
    ZZ operand;   // The operand
    ZZ quotient;  // The precomputed quotient
} MUMO;

/**
Modular mult. using a highly-optimized variant of Barrett reduction. Reduces result to [0, 2q- 1].
Correctness: q <= 31-bit, y < q.

@param[in] x  Operand 1
@param[in] y  Operand 2
@param[in] q  Modulus. Must be <= 31 bits
@returns      ((x * y) mod q) or (((x * y) mod q) + q)
*/
static inline ZZ mul_mod_mumo_lazy(ZZ x, const MUMO *y, const Modulus *modulus)
{
    ZZ q = modulus->value;

    // -- We want to calculate [x*y]q (i.e. (x*y) mod q)
    //    We will use Barrett reduction to calculate r = [x*y]q or [x*y]2q
    //    The formula for this is:
    //      r = [ [x*y]b - [floor(x*u/t) * q]b ]b
    //        = [ [op1]b - [op2 * q]b ]b
    //     where u = floor(y*t/q), b = 2^32, t = 2^32

    // -- This will implicitly give us [x*y]b by only returning the lower 32 bits of x*y
    ZZ op1 = x * y->operand;

    // -- We can get op2 = floor(x*u/t) by multiplying x with floor(u) and taking the high
    // word
    ZZ op2 = mul_uint_high(x, y->quotient);

    // -- Use the same trick two more times to get result.
    ZZ r = op1 - op2 * q;

    // -- Note that we don't need to call 'shift_result' here in this lazy version
    return r;
}

/**
Modular mult. using a highly-optimized variant of Barrett reduction. Reduces result to [0, q).
Correctness: q <= 31-bit, y < q.

@param[in] x  Operand 1
@param[in] y  Operand 2
@param[in] q  Modulus. Must be <= 31 bits.
@returns      (x * y) mod q
*/
static inline ZZ mul_mod_mumo(ZZ x, const MUMO *y, const Modulus *q)
{
    ZZ r = mul_mod_mumo_lazy(x, y, q);
    return shift_result(r, q->value);
}

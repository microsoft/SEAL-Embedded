// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file uintops.h
*/

#pragma once

#include <stdint.h>  // uint64_t

#include "defines.h"
#include "uint_arith.h"

/**
Adds two ZZ-type unsigned integers together but does not keep track of the carry.

@param[in]  op1  A pointer to a ZZ-type value
@param[in]  op2  A pointer to another ZZ-type value
@param[out] res  Pointer to sizeof(ZZ)-sized result op1 + op2 (w/o the carry)
*/
static inline void add_uint_nocarry(ZZ op1, ZZ op2, ZZ *res)
{
    *res = op1 + op2;
}

/**
Adds two ZZ-type unsigned integers together.

@param[in]  op1  A pointer to a ZZ-type value
@param[in]  op2  A pointer to another ZZ-type value
@param[out] res  Pointer to sizeof(ZZ)-sized result of op1 + op2 (w/o the carry)
@returns         The carry value
*/
static inline uint8_t add_uint(ZZ op1, ZZ op2, ZZ *res)
{
    return add_uint32(op1, op2, res);
}

/**
Multiplies two ZZ-type values and returns the full sized-(2*sizeof(ZZ)) result.

@param[in]  op1  A type-ZZ value
@param[in]  op2  Another type-ZZ value
@param[out] res  Pointer to full (2*sizeof(ZZ))-sized result of op1 * op2
*/
static inline void mul_uint_wide(ZZ op1, ZZ op2, ZZ *res)
{
    return mul_uint32_wide(op1, op2, res);
}

/**
Multiplies two ZZ-type values and returns the upper sizeof(ZZ) bytes of the
sized-(2*sizeof(ZZ)) result.

@param[in] op1  A type-ZZ value
@param[in] op2  Another type-ZZ value
@returns        Upper sizeof(ZZ) bytes of op1 * op2
*/
static inline ZZ mul_uint_high(ZZ op1, ZZ op2)
{
    return mul_uint32_high(op1, op2);
}

/**
Multiplies two ZZ-type values and returns the lower sizeof(ZZ) bytes of the
sized-(2*sizeof(ZZ)) result.

@param[in] op1  A type-ZZ value
@param[in] op2  Another type-ZZ value
@returns        Lower sizeof(ZZ) bytes of op1 * op2
*/
static inline ZZ mul_uint_low(ZZ op1, ZZ op2)
{
    return mul_uint32_low(op1, op2);
}

/**
Adds two 128-bit unsigned integers together.

@param[in]  op1  A pointer to a 128-bit value
@param[in]  op2  A pointer to another 128-bit value
@param[out] res  Pointer to the 128-bit result of op1 + op2 (w/o the carry)
@returns         The carry value
*/
static inline unsigned char add_uint128(const uint64_t *op1, const uint64_t *op2, uint64_t *res)
{
    uint64_t res_right;
    uint64_t carry_right = (uint64_t)add_uint64(op1[0], op2[0], (uint64_t *)&res_right);

    uint64_t res_left;
    unsigned char carry_left = add_uint64(op1[1], op2[1], (uint64_t *)&res_left);

    // -- Carry for below should be 0, so don't need to save it
    add_uint64(res_left, carry_right, (uint64_t *)&res_left);

    res[1] = res_left;
    res[0] = res_right;

    return carry_left;
}

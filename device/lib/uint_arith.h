// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file uint_arith.h
*/

#pragma once

#include <stdint.h>

#include "defines.h"

/**
Adds two uint32_t values.

@param[in]  op1  32-bit Operand 1
@param[in]  op2  32-bit Operand 2
@param[out] res  Pointer to 64-bit result of op1 + op2 (w/o the carry)
@retuns          The carry bit
*/
static inline uint8_t add_uint32(uint32_t op1, uint32_t op2, uint32_t *res)
{
    *res = op1 + op2;
    return (uint8_t)(*res < op1);
}

/**
Adds two uint64_t values.

@param[in]  op1  64-bit Operand 1
@param[in]  op2  64-bit Operand 2
@param[out] res  Pointer to 128-bit result of op1 + op2 (w/o the carry)
@retuns          The carry bit
*/
static inline uint8_t add_uint64(uint64_t op1, uint64_t op2, uint64_t *res)
{
    *res = op1 + op2;
    return (uint8_t)(*res < op1);
}

/**
Multiplies two uint32_t values.

@param[in]  op1  32-bit Operand 1
@param[in]  op2  32-bit Operand 2
@param[out] res  Pointer to full 64-bit result of op1 * op2
*/
static inline void mul_uint32_wide(uint32_t op1, uint32_t op2, uint32_t *res)
{
#ifdef SE_USE_ASM_ARITH
    __asm("UMULL %0, %1, %2, %3" : "=r"(res[0]), "=r"(res[1]) : "r"(op1), "r"(op2));
#else
    uint64_t res_temp = (uint64_t)op1 * (uint64_t)op2;
    res[0]            = (uint32_t)(res_temp & 0xFFFFFFFF);
    res[1]            = (uint32_t)((res_temp >> 32) & 0xFFFFFFFF);
#endif
}

/**
Multiplies two uint32_t values and returns the upper 32 bits of the 64-bit result.

@param[in]  op1  Operand 1
@param[in]  op2  Operand 2
@returns         Upper 32 bits of the 64-bit result of op1 * op2
*/
static inline uint32_t mul_uint32_high(uint32_t op1, uint32_t op2)
{
    uint32_t res[2];
    mul_uint32_wide(op1, op2, res);
    return res[1];
}

/**
Multiplies two uint32_t values and returns the lower 32 bits of the 64-bit result.

@param[in]  op1  Operand 1
@param[in]  op2  Operand 2
@returns         Lower 32 bits of the 64-bit result of op1 * op2
*/
static inline uint32_t mul_uint32_low(uint32_t op1, uint32_t op2)
{
    return op1 * op2;
}

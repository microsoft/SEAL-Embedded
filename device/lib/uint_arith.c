// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file uint_arith.c
*/

#include "uint_arith.h"

#include "defines.h"

// -- Need these to avoid duplicate symbols error

extern inline uint8_t add_uint32(uint32_t op1, uint32_t op2, uint32_t *res);
extern inline uint8_t add_uint64(uint64_t op1, uint64_t op2, uint64_t *res);

extern inline void mul_uint32_wide(uint32_t op1, uint32_t op2, uint32_t *res);
extern inline uint32_t mul_uint32_high(uint32_t op1, uint32_t op2);
extern inline uint32_t mul_uint32_low(uint32_t op1, uint32_t op2);

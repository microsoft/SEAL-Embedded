// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file modulus.h
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>  // uint64_t

#include "defines.h"

/**
Struct to store a modulus. 'const_ratio' can be precomputed and used later for faster modular
reduction in some cases.

@param value        Value of the modulus (aka 'q')
@param const_ratio  floor(2^64/q)
*/
typedef struct Modulus
{
    ZZ value;  // Value of the modulus (aka 'q')

    // -- Note: SEAL const_ratio is size 3 to store the remainder,
    //    but we don't need the remainder so we can use a size 2 array

    ZZ const_ratio[2];  // floor(2^64/q)
} Modulus;

/**
Sets up the modulus object for a particular modulus value. Useful for setting up a modulus if
const_ratio for modulus value has not been pre-computed by set_modulus' table.

@param[in]  q    Modulus value
@param[in]  hw   High word of const_ratio for 'q'
@param[in]  lw   Low word of const_ratio for 'q'
@param[out] mod  Modulus object to set
*/
void set_modulus_custom(const ZZ q, ZZ hw, ZZ lw, Modulus *mod);

/**
Sets up the modulus object for a particular modulus value. Implements const_ratio set as a table
lookup. If table does not contain const_ratio for the requested modulus value, returns a failure. In
this case, set_modulus_custom should be used instead.

@param[in]  q    Modulus value
@param[out] mod  Modulus object to set
@returns         1 on success, 0 on failure
*/
bool set_modulus(const ZZ q, Modulus *mod);

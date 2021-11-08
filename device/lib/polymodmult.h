// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file polymodmult.h
*/

#pragma once

#include <stddef.h>

#include "defines.h"
#include "inttypes.h"
#include "modulo.h"

/**
TODO: Move to testing code?
Multiplies two ring polynomials using schoolbook multiplication. The result polynomial will be
returned in the first n ZZ elements pointed to by 'res'.

Note: This function is *not* constant-time, and is mainly useful for testing.

Space req: 'res' must constain space for 2n ZZ elements, where each input polynomial consists of n
ZZ elements (each).

@param[in]  a    Input polynomial 1
@param[in]  b    Input polynomial 2
@param[in]  n    Number of coefficients to multiply
@param[in]  mod  Modulus
@param[out] res  Result of [a . b]_mod (in the first n ZZ elements)
*/
void poly_mult_mod_sb(const ZZ *a, const ZZ *b, PolySizeType n, const Modulus *mod, ZZ *res);

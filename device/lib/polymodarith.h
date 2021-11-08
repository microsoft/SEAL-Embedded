// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file polymodarith.h
*/

#pragma once

#include "defines.h"
#include "uintmodarith.h"

/**
Modular polynomial addition. 'p1' and 'res' may share the same starting address for in-place
computation.

Space req: 'res' must have space for n ZZ values.

@param[in]  p1   Input polynomial 1
@param[in]  p2   Input polynomial 2
@param[in]  n    Number of elements (ZZ coefficients) in p1 and p2
@param[in]  mod  Modulus
@param[out] res  Result polynomial
*/
static inline void poly_add_mod(const ZZ *p1, const ZZ *p2, PolySizeType n, const Modulus *mod,
                                ZZ *res)
{
    for (PolySizeType i = 0; i < n; i++) { res[i] = add_mod(p1[i], p2[i], mod); }
}

/**
In-place modular polynomial addition.

@param[in]  p1   In: Input polynomial 1; Out: Result polynomial
@param[in]  p2   Input polynomial 2
@param[in]  n    Number of elements (ZZ coefficients) in p1 and p2
@param[in]  mod  Modulus
*/
static inline void poly_add_mod_inpl(ZZ *p1, const ZZ *p2, PolySizeType n, const Modulus *mod)
{
    for (PolySizeType i = 0; i < n; i++) { add_mod_inpl(&(p1[i]), p2[i], mod); }
}

/**
Modular polynomial negation. 'p1' and 'res' may share the same starting address for in-place
computation.

Space req: 'res' must have space for n ZZ values.

@param[in]  p1   Input polynomial
@param[in]  n    Number of elements (ZZ coefficients) in p1
@param[in]  mod  Modulus
@param[out] res  Result polynomial
*/
static inline void poly_neg_mod(const ZZ *p1, PolySizeType n, const Modulus *mod, ZZ *res)
{
    for (PolySizeType i = 0; i < n; i++) { res[i] = neg_mod(p1[i], mod); }
}

/**
In-place modular polynomial negation.

@param[in]  p1   In: Input polynomial; Out: Result polynomial
@param[in]  n    Number of elements (ZZ coefficients) in p1
@param[in]  mod  Modulus
*/
static inline void poly_neg_mod_inpl(ZZ *p1, PolySizeType n, const Modulus *mod)
{
    for (PolySizeType i = 0; i < n; i++) { neg_mod_inpl(&(p1[i]), mod); }
}

/**
Pointwise modular polynomial negation. 'p1' and 'res' may share the same starting address for
in-place computation (see: poly_pointwise_mul_mod_inpl).

Space req: 'res' must have space for n ZZ values.

@param[in]  p1   Input polynomial
@param[in]  n    Number of elements (ZZ coefficients) in p1
@param[in]  mod  Modulus
@param[out] res  Result polynomial.
*/
static inline void poly_pointwise_mul_mod(const ZZ *p1, const ZZ *p2, PolySizeType n,
                                          const Modulus *mod, ZZ *res)
{
    for (PolySizeType i = 0; i < n; i++) { res[i] = mul_mod(p1[i], p2[i], mod); }
}

/**
In-place pointwise modular polynomial negation.

@param[in]  p1   In: Input polynomial 1; Out: Result polynomial
@param[in]  p2   Input polynomial 2
@param[in]  n    Number of elements (ZZ coefficients) in p1
@param[in]  mod  Modulus
*/
static inline void poly_pointwise_mul_mod_inpl(ZZ *p1, const ZZ *p2, PolySizeType n,
                                               const Modulus *mod)
{
    poly_pointwise_mul_mod(p1, p2, n, mod, p1);
}

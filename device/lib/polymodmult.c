// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file polymodmult.c

Polynomial modular multiplication.
*/

#include "polymodmult.h"

#include <stdbool.h>

#include "defines.h"
#include "parameters.h"
#include "sample.h"
#include "string.h"  // memset
#include "uintmodarith.h"
#include "util_print.h"

/**
Helper funciton to multiply two ring polynomials using schoolbook multiplication, without the final
polynomial reduction. The result polynomial will be returned in the first n ZZ elements pointed to
by 'res'.

Note: This function is *not* constant-time, and is mainly useful for testing.

Space req: 'res' must contain space for 2n ZZ elements, where each input polynomial consists of n ZZ
elements (each).

@param[in]  a    Input polynomial 1
@param[in]  b    Input polynomial 2
@param[in]  n    Number of coefficients to multiply
@param[in]  mod  Modulus
@param[out] res  Sized-(2*sizeof(ZZ)) result of [a . b]_mod
*/
void poly_mult_mod_sb_not_reduced(const ZZ *a, const ZZ *b, PolySizeType n, const Modulus *mod,
                                  ZZ *res)
{
    memset(res, 0, 2 * n * sizeof(ZZ));  // This is necessary

    for (PolySizeType i = 0; i < n; i++)
    {
        for (PolySizeType j = 0; j < n; j++)
        {
            // -- Does the following: res[i + j] += a[i] * b[j] (mod p)
            mul_add_mod_inpl(&(res[i + j]), a[i], b[j], mod);
        }
    }
}

/**
Helper function to perform the final polynomial reduction.
Initial address of 'a' and initial address of 'res' may be the same (see: poly_reduce_inpl).

Space req: 'res' must contain space for n ZZ elements (for 'a' with 2n coefficients)

@param[in]  a    Input polynomial
@param[in]  n    Degree to reduce by
@param[in]  mod  Modulus
@param[out] res  Result polynomial
*/
void poly_reduce(const ZZ *a, PolySizeType n, const Modulus *mod, ZZ *res)
{
    for (PolySizeType i = 0; i < n; i++)
    {
        // -- Does the following:
        //    res[i] = (a[i] - a[i + n]) % (ZZ)(mod->value);
        res[i] = sub_mod((a[i]), a[i + n], mod);
    }
}

/**
Helper function to perform the final polynomial reduction in place.

@param[in]  a    In: Input polynomial (of 2n ZZ values);
                 Out: Result polynomial (in first n ZZ values).
@param[in]  n    Degree to reduce by
@param[in]  mod  Modulus
*/
void poly_reduce_inpl(ZZ *a, PolySizeType n, const Modulus *mod)
{
    for (PolySizeType i = 0; i < n; i++)
    {
        // -- Does the following: a[i] = (a[i] - a[i + n]) % (ZZ)(mod->value);
        sub_mod_inpl(&(a[i]), a[i + n], mod);
    }
}

void poly_mult_mod_sb(const ZZ *a, const ZZ *b, PolySizeType n, const Modulus *mod, ZZ *res)
{
    // -- Multiplication (not reduced)
    poly_mult_mod_sb_not_reduced(a, b, n, mod, res);
    // print_poly("not-reduced (expected)", res, 2*n);
    // print_poly_ring("not-reduced", res, 2*n);
    // print_poly_"not-reduced (expected)", res, 2*n);

    // -- Ring reduction
    poly_reduce_inpl(res, n, mod);
    // print_poly("reduced (expected)    ", res, n);
}

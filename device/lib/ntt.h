// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ntt.h

Number Theoretic Transform.
*/

#pragma once

#include "defines.h"
#include "fileops.h"
#include "parameters.h"
#include "polymodarith.h"
#include "uintmodarith.h"

/*
For all "Harvey" butterfly NTTs:
The input is a polynomial of degree n in R_q, where n is assumed to be a power of 2 and q is a prime
such that q = 1 (mod 2n). The output is a vector A such that the following hold:
A[j] = a(psi**(2*bit_reverse(j) + 1)), 0 <= j < n. For more details, see the SEAL-Embedded paper and
Michael Naehrig and Patrick Longa's paper.
*/

/**
Initializes NTT roots.

If SE_NTT_FAST is defined, will load "fast"(a.k.a. "lazy") roots from file.
Else, if SE_NTT_REG is defined, will load regular roots from file.
Else, if SE_NTT_ONE_SHOT is defined, will calculate NTT roots one by one.
Else, (SE_NTT_OTF), will do nothing ('ntt_roots' can be null and will be ignored).

Space req: 'ntt_roots' should have space for 2n ZZ elements if SE_NTT_FAST is defined or n ZZ
elements if SE_NTT_ONE_SHOT or SE_NTT_REG is defined.

@param[in]  parms      Parameters set by ckks_setup
@param[out] ntt_roots  NTT roots. Will be null if SE_NTT_OTF is defined.
*/
void ntt_roots_initialize(const Parms *parms, ZZ *ntt_roots);

/**
Negacyclic in-place NTT using the Harvey butterfly.

If SE_NTT_FAST is defined, will use "fast"(a.k.a. "lazy") NTT computation.
Else, if SE_NTT_REG or SE_NTT_ONE_SHOT is defined, will use regular NTT computation.
Else, (SE_NTT_OTF is defined), will use truly "on-the-fly" NTT computation. In this last
case, 'ntt_roots' may be null (and will be ignored).

@param[in]     parms      Parameters set by ckks_setup
@param[in]     ntt_roots  NTT roots set by ntt_roots_initialize. Ignored if SE_NTT_OTF is defined.
@param[in,out] vec        Input/output polynomial of n ZZ elements
*/
void ntt_inpl(const Parms *parms, const ZZ *ntt_roots, ZZ *vec);

/**
Polynomial multiplication for inputs already in NTT form. 'res' and 'a' may share the same starting
address (see: poly_mult_mod_ntt_form_inpl)

@param[in]  a    Input polynomial 1, in NTT form, with n ZZ coefficients
@param[in]  b    Input polynomial 2, in NTT form, with n ZZ coefficients
@param[in]  n    Number of coefficients in a, b, and c
@param[in]  mod  Modulus
@param[out] res  Result polynomial, in NTT form, with n ZZ coefficients
*/
static inline void poly_mult_mod_ntt_form(const ZZ *a, const ZZ *b, size_t n, const Modulus *mod,
                                          ZZ *res)
{
    // -- Inputs in NTT form can be multiplied component-wise
    poly_pointwise_mul_mod(a, b, n, mod, res);
}

/**
In-place polynomial multiplication for inputs already in NTT form.

@param[in,out] a    In: Input polynomial 1; Out: Result polynomial
@param[in]     b    Input polynomial 2, in NTT form, with n ZZ coefficients.
@param[in]     n    Number of coefficients in a and b
@param[in]     mod  Modulus
*/
static inline void poly_mult_mod_ntt_form_inpl(ZZ *a, const ZZ *b, size_t n, const Modulus *mod)
{
    // -- Values in NTT form can be multiplied component-wise
    poly_pointwise_mul_mod_inpl(a, b, n, mod);
}

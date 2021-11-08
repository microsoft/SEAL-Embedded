// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file intt.h

Inverse Number Theoretic Transform.
*/

#pragma once

#include "defines.h"
#include "parameters.h"

#ifndef SE_DISABLE_TESTING_CAPABILITY  // INTT is only needed for on-device testing
/*
For all "Harvey" butterfly INTTs:
The input is a polynomial of degree n in R_q, where n is assumed to be a power of 2 and q is a prime
such that q = 1 (mod 2n). The output is a vector A such that the following hold:
A[j] = a(psi**(2*bit_reverse(j) + 1)), 0 <= j < n. For more details, see the SEAL-Embedded paper and
Michael Naehrig and Patrick Longa's paper.
*/

/**
Initializes INTT roots.

If SE_INTT_ONE_SHOT is defined, will calculate INTT roots one by one.
Else if  SE_INTT_FAST is defined, will load "fast"/"lazy" roots from file.
Else (if SE_INTT_REG is defined), will load regular roots from file.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'intt_roots' should
have space for n ZZ elements if SE_INTT_ONE_SHOT or SE_INTT_REG is defined,
or 2n ZZ elements (i.e. n MUMO elements) if SE_INTT_FAST is defined.

@param[in]  parms       Parameters set by ckks_setup
@param[out] intt_roots  INTT roots. Will be null if SE_INTT_OTF is defined.
*/
void intt_roots_initialize(const Parms *parms, ZZ *intt_roots);

/**
Performs a negacyclic inverse NTT using the Harvey butterfly.

@param[in]      parms       Parameters set by ckks_setup
@param[in]      intt_roots  Roots set by intt_roots_initialize. Ignored if SE_INTT_OTF is defined.
@param[in, out] vec         Input/output polynomial of n ZZ elements
*/
void intt_inpl(const Parms *parms, const ZZ *intt_roots, ZZ *vec);
#endif

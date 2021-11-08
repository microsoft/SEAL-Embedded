// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file fft.h

Forward and Inverse Fast Fourier Transform.

Note: Currently, the fft is only useful for testing, since only the ifft is used in the main
algorithm. Leave both here for future use.
*/

#pragma once

#include <complex.h>

#include "defines.h"

/*
static inline size_t bitrev(size_t input, size_t numbits)
{
    size_t res = 0;
    for (size_t i = 0; i < numbits; i++)
    {
        // -- Get least significant bit of input and place in result.
        // -- Shift result to the left towards msb position.
        size_t lsb = input & 1;
        res <<= 1;
        res |= lsb;
        input = input >> 1;
    }
    return res;
}
*/

/**
Bit reversal algorithm for the FFT/IFFT.
Ex: bitrev(6, 3): 6 = 0b110 -> (bit reverse) -> 0b011 = 3

Requires: numbits <= 16

(This does essentially the same as the commented out code above)

@param[in] input    Value to bit reverse
@param[in] numbits  Number of bits required to represent input. Must be <= 16.
@returns            'input' in bit reversed order
*/
static inline size_t bitrev(size_t input, size_t numbits)
{
    size_t t = (((input & 0xaaaa) >> 1) | ((input & 0x5555) << 1));
    t        = (((t & 0xcccc) >> 2) | ((t & 0x3333) << 2));
    t        = (((t & 0xf0f0) >> 4) | ((t & 0x0f0f) << 4));
    t        = (((t & 0xff00) >> 8) | ((t & 0x00ff) << 8));
    return numbits == 0 ? 0 : t >> (16 - (size_t)(numbits));
}

// ===========================
//          Roots
// ===========================

/**
Generates the roots for the FFT from scratch (in bit-reversed order)

Space req: 'roots' must have storage for n double complex values.

@param[in]  n      FFT transform size (number of roots to generate)
@param[in]  logn   Minimum number of bits required to represent n (i.e. log2(n))
@param[out] roots  FFT roots (in bit-reversed order)
*/
void calc_fft_roots(size_t n, size_t logn, double complex *roots);

/**
Generates the roots for the IFFT from scratch (in bit-reversed order)

Space req: 'ifft_roots' must have storage for n double complex values.

@param[in]  n           IFFT transform size (number of roots to generate)
@param[in]  logn        Minimum number of bits required to represent n (i.e. log2(n))
@param[out] ifft_roots  IFFT roots (in bit-reversed order)
*/
void calc_ifft_roots(size_t n, size_t logn, double complex *ifft_roots);

// ===========================
//          FFT/IFFT
// ===========================
/**
In-place Inverse Fast-Fourier Transform using the Harvey butterfly.
'roots' is ignored (and may be null) if SE_IFFT_OTF is chosen.

Note: This function does not divide the final result by n. This step must be performed outside of
this function.

@param[in,out] vec    Input/Output vector of n double complex values
@param[in]     n      IFFT transform size (i.e. polynomial degree)
@param[in]     logn   Minimum number of bits required to represent n (i.e. log2(n))
@param[in]     roots  [Optional]. As set by calc_ifft_roots or load_ifft_roots
*/
void ifft_inpl(double complex *vec, size_t n, size_t logn, const double complex *roots);

/**
In-place forward Fast-Fourier Transform using the Harvey butterfly.
'roots' is ignored (and may be null) if SE_FFT_OTF is chosen.

@param[in,out] vec    Input/Output vector of n double complex values.
@param[in]     n      FFT transform size (i.e. polynomial degree)
@param[in]     logn   Minimum number of bits required to represent n (i.e. log2(n))
@param[in]     roots  [Optional]. As set by calc_fft_roots or load_fft_roots
*/
void fft_inpl(double complex *vec, size_t n, size_t logn, const double complex *roots);

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file test_common.h
*/

#pragma once

#include <math.h>
#include <stdint.h>  // uint64_t, UINT64_MAX
#include <string.h>  // memset

#include "defines.h"
#include "modulo.h"
#include "sample.h"
#include "util_print.h"

//#define TEST_DEBUG

#define BILLION 1000000000L

#ifndef MAX64
#define MAX64 0xFFFFFFFFFFFFFFFFULL
#endif

#ifndef MAX63
#define MAX63 0x7FFFFFFFFFFFFFFFULL
#endif

#ifndef MAX32
#define MAX32 0xFFFFFFFFUL
#endif

#ifndef MAX31
#define MAX31 0x7FFFFFFFUL
#endif

#ifndef MAX16
#define MAX16 0xFFFFU
#endif

#define MAX_ZZ MAX32
#define MAX_Q MAX31

// -----------------------------------------------
// Generate "random" uint
// Note: random_zz is already defined in sample.h
// -----------------------------------------------
/**
@param[in] q  Modulus value
@returns      A random ZZ value mod q
*/
static inline ZZ random_zzq(Modulus *q)
{
    return barrett_reduce(random_zz(), q);
}

/**
@returns a random sized-(sizeof(ZZ)/2) value
*/
static inline ZZ random_zz_half(void)
{
    // -- Don't do this. It doesn't work:
    // ZZ result;
    // getrandom((void*)&result, sizeof(result)/2, 0);

    return random_zz() & 0xFFFF;
}

/**
@returns a random sized-(sizeof(ZZ)/4) value
*/
static inline ZZ random_zz_quarter(void)
{
    return random_zz() & 0xFF;
}

/**
@returns a random sized-(sizeof(ZZ)/8) value
*/
static inline ZZ random_zz_eighth(void)
{
    return random_zz() & 0xF;
}

// -------------------------
// Generate "random" double
// -------------------------
static inline double gen_double(int64_t div)
{
    return (double)random_zz() / (double)div;
}
static inline double gen_double_half(int64_t div)
{
    return (double)random_zz_half() / (double)div;
}
static inline double gen_double_quarter(int64_t div)
{
    return (double)random_zz_quarter() / (double)div;
}
static inline double gen_double_eighth(int64_t div)
{
    return (double)random_zz_eighth() / (double)div;
}

// -----------------------
// Generate "random" flpt
// -----------------------
static inline flpt gen_flpt(int64_t div)
{
    return (flpt)gen_double(div);
}
static inline flpt gen_flpt_half(int64_t div)
{
    return (flpt)gen_double_half(div);
}
static inline flpt gen_flpt_quarter(int64_t div)
{
    return (flpt)gen_double_quarter(div);
}
static inline flpt gen_flpt_eighth(int64_t div)
{
    return (flpt)gen_double_eighth(div);
}

// ------------------
// Compare functions
// ------------------
static inline void compare_poly(const char *a_name, const ZZ *a, const char *b_name, const ZZ *b,
                                size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (a[i] != b[i])
        {
            printf("\n");
            printf("Mismatched index: %zu\n", i);
            print_zz(a_name, a[i]);
            print_zz(b_name, b[i]);
        }
        se_assert(a[i] == b[i]);
    }
}

static inline bool compare_poly_flpt(const char *a_name, const flpt *a, const char *b_name,
                                     const flpt *b, size_t len, flpt max_diff)
{
    printf("Comparing...\n");
    for (size_t i = 0; i < len; i++)
    {
        flpt diff = (flpt)fabs(a[i] - b[i]);
        if (diff >= max_diff)
        {
            printf("%s[%zu]: %0.9f\n", a_name, i, a[i]);
            printf("%s[%zu]: %0.9f\n", b_name, i, b[i]);
            return 1;
        }
    }
    return 0;
}

static inline bool all_zeros(ZZ *vec, size_t n)
{
    size_t num_zeros = 0;
    for (size_t i = 0; i < n; i++)
    {
        if (!vec[i]) num_zeros++;
    }
    if (n == num_zeros)
        return true;
    else
        return false;
}

static inline bool compare_poly_double_complex(const double complex *a, const double complex *b,
                                               size_t n, double maxdiff)
{
    for (size_t i = 0; i < n; i++)
    {
        // printf("Checking index %zu: \n", i);
        double diff = cabs(a[i] - b[i]);
        if (diff >= maxdiff)
        {
            printf("vec1[%zu]: %0.9f + %0.9fi\n", i, se_creal(a[i]), se_cimag(a[i]));
            printf("vec2[%zu]: %0.9f + %0.9fi\n", i, se_creal(b[i]), se_cimag(b[i]));
            return 1;
        }
    }
    return 0;
}
// -------------------------------------------------
// Set uint
// (Note that clear is already defined in defines.h)
// -------------------------------------------------
static inline void set(ZZ *vec, size_t vec_len, ZZ val)
{
    for (size_t i = 0; i < vec_len; i++) vec[i] = val;
}

// ------------------
// Set/clear flpt
// -------------------
static inline void clear_flpt(flpt *poly, PolySizeType n)
{
    for (PolySizeType i = 0; i < n; i++) poly[i] = 0;
}

static inline void set_flpt(flpt *vec, size_t vec_len, flpt val)
{
    for (size_t i = 0; i < vec_len; i++) vec[i] = val;
}

// ----------------------
// Set/clear double poly
// ----------------------
static inline void clear_double(double *vec, PolySizeType n)
{
    memset(vec, 0, sizeof(double) * n);
    // for (PolySizeType i = 0; i < n; i++) vec[i] = 0;
}

static inline void set_double(double *vec, size_t vec_len, double val)
{
    for (size_t i = 0; i < vec_len; i++) vec[i] = val;
}

// ------------------------------
// Set/clear double complex poly
// ------------------------------
static inline void clear_double_complex(double complex *vec, size_t n)
{
    memset(vec, 0, sizeof(double complex) * n);
    // for (size_t i = 0; i < n; i++) vec[i] = double complex(0, 0);
}

static inline void set_double_complex(double complex *vec, size_t n, flpt val)
{
    for (size_t i = 0; i < n; i++) vec[i] = (double complex)_complex((double)val, (double)0);
}

// ----------------------
// "Random" uint poly
// ----------------------
static inline void random_zz_quarter_poly(ZZ *poly, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = random_zz_quarter();
}

static inline void random_zz_half_poly(ZZ *poly, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = random_zz_half();
}

static inline void random_zz_poly(ZZ *poly, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = random_zz();
}

static inline void random_zzq_poly(ZZ *poly, size_t n, Modulus *q)
{
    for (size_t i = 0; i < n; i++) poly[i] = random_zzq(q);
}

// -------------------------
// "Random" double complex
// -------------------------
static inline void gen_double_complex_eighth_vec(double complex *vec, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++)
    { vec[i] = (double complex)(gen_double_eighth(div), gen_double_eighth(div)); }
}

static inline void gen_double_complex_quarter_vec(double complex *vec, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++)
    { vec[i] = (double complex)(gen_double_quarter(div), gen_double_quarter(div)); }
}

static inline void gen_double_complex_half_vec(double complex *vec, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++)
    { vec[i] = (double complex)(gen_double_half(div), gen_double_half(div)); }
}

static inline void gen_double_complex_vec(double complex *vec, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) { vec[i] = (double complex)(gen_double(div), gen_double(div)); }
}

// ----------------------
// "Random" double poly
// ----------------------
static inline void gen_double_eighth_poly(double *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_double_eighth(div);
}

static inline void gen_double_quarter_poly(double *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_double_quarter(div);
}

static inline void gen_double_half_poly(double *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_double_half(div);
}

static inline void gen_double_poly(double *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_double(div);
}

// ----------------------
// "Random" float poly
// ----------------------
static inline void gen_flpt_eighth_poly(flpt *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_flpt_eighth(div);
}

static inline void gen_flpt_quarter_poly(flpt *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_flpt_quarter(div);
}

static inline void gen_flpt_half_poly(flpt *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_flpt_half(div);
}

static inline void gen_flpt_poly(flpt *poly, int64_t div, size_t n)
{
    for (size_t i = 0; i < n; i++) poly[i] = gen_flpt(div);
}

static inline void print_test_banner(const char *test_name, const Parms *parms)
{
    printf("***************************************************\n");
    printf("Running Test: %s\n", test_name);
    printf("n: %zu, nprimes: %zu, scale: %0.2f\n", parms->coeff_count, parms->nprimes,
           parms->scale);
    print_config(!parms->is_asymmetric);
    printf("***************************************************\n");
}

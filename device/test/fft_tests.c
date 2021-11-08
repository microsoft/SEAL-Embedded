// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file fft_tests.c
*/

#include <complex.h>
#include <math.h>    // log2
#include <string.h>  // memcpy

#include "defines.h"
#include "fft.h"
#include "fileops.h"
#include "test_common.h"
#include "util_print.h"

#ifndef SE_USE_MALLOC
#if defined(SE_IFFT_LOAD_FULL) || defined(SE_IFFT_ONE_SHOT)
#define IFFT_TEST_ROOTS_MEM SE_DEGREE_N
#else
#define IFFT_TEST_ROOTS_MEM 0
#endif
#if defined(SE_FFT_LOAD_FULL) || defined(SE_FFT_ONE_SHOT)
#define FFT_TEST_ROOTS_MEM SE_DEGREE_N
#else
#define FFT_TEST_ROOTS_MEM 0
#endif
#endif

/**
Multiplies two complex-valued polynomials using schoolbook multiplication.

Space req: 'res' must constain space for n double complex elements, where each input polynomial
consists of n double complex elements (each).

@param[in]  a    Input polynomial 1
@param[in]  b    Input polynomial 2
@param[in]  n    Number of coefficients to multiply
@param[out] res  Result of [a * b]
*/
void poly_mult_sb_complex(const double complex *a, const double complex *b, PolySizeType n,
                          double complex *res)
{
    for (PolySizeType i = 0; i < n; i++)
    {
        for (PolySizeType j = 0; j < n; j++) { res[i + j] += a[i] * b[j]; }
    }
}

/**
In-place divides each element of a polynomial by a divisor.

@param[in,out] poly     In: Input polynomial; Out: Result polynomial
@param[in]     n        Number of coefficients to multiply
@param[in]     divisor  Divisor value
*/
static inline void poly_div_inpl_complex(double complex *poly, size_t n, size_t divisor)
{
    for (size_t i = 0; i < n; i++) poly[i] = (double)poly[i] / (double)divisor;
}

/**
Pointwise multiplies two complex-valued polynomials. 'a' and 'res' starting address may overlap for
in-place computation (see: pointwise_mult_inpl_complex)

Space req: 'res' must constain space for n double complex elements, where each input polynomial
consists of n double complex elements (each).

@param[in]  a    Input polynomial 1
@param[in]  b    Input polynomial 2
@param[in]  n    Number of coefficients to multiply
@param[out] res  Result of [a . b]
*/
static inline void pointwise_mult_complex(const double complex *a, const double complex *b,
                                          PolySizeType n, double complex *res)
{
    for (size_t i = 0; i < n; i++) res[i] = a[i] * b[i];
}

/**
In-place pointwise multiplies two complex-valued polynomials.

@param[in]  a    Input polynomial 1
@param[in]  b    Input polynomial 2
@param[in]  n    Number of coefficients to multiply
@param[out] res  Result of [a . b]
*/
static inline void pointwise_mult_inpl_complex(double complex *a, const double complex *b,
                                               PolySizeType n)
{
    for (size_t i = 0; i < n; i++) a[i] *= b[i];
}

/**
Helper function for FFT test
*/
void test_fft_mult_helper(size_t n, double complex *v1, double complex *v2, double complex *v_exp,
                          double complex *temp, double complex *roots)
{
    size_t logn = (size_t)log2(n);

    // -- Correctness: ifft(fft(vec) .* fft(v2))*(1/n) = vec * vec2
    print_poly_double_complex("v1              ", v1, n);
    print_poly_double_complex("v2              ", v2, n);

    double complex *v_res = temp;
    clear_double_complex(v_res, n);

    poly_mult_sb_complex(v1, v2, n / 2, v_res);
    print_poly_double_complex("vec_res (expected)", v_res, n);

#ifdef SE_FFT_LOAD_FULL
    se_assert(roots);
    load_fft_roots(n, roots);
#elif defined(SE_FFT_ONE_SHOT)
    se_assert(roots);
    calc_fft_roots(n, logn, roots);
#endif

    fft_inpl(v1, n, logn, roots);
    // print_poly_double_complex("v1 (after fft)  ", v1, n);
    fft_inpl(v2, n, logn, roots);
    // print_poly_double_complex("v2 (after fft)  ", v2, n);

    pointwise_mult_inpl_complex(v1, v2, n);
    // print_poly_double_complex("vec (after mult)  ", v1, n);

#ifdef SE_IFFT_LOAD_FULL
    se_assert(roots);
    load_ifft_roots(n, roots);
#elif defined(SE_IFFT_ONE_SHOT)
    se_assert(roots);
    calc_ifft_roots(n, logn, roots);
#endif

    ifft_inpl(v1, n, logn, roots);
    // print_poly_double_complex("vec (after ifft)  ", v1, n);

    poly_div_inpl_complex(v1, n, n);
    print_poly_double_complex("vec_res (actual)  ", v1, n);

    double maxdiff = 0.0001;
    bool err       = compare_poly_double_complex(v1, v_res, n, maxdiff);
    se_assert(!err);

    if (v_exp)
    {
        print_poly_double_complex("v_exp2          ", v_exp, n);
        err = compare_poly_double_complex(v_exp, v_res, n, maxdiff);
        se_assert(!err);
    }
}

void test_fft_helper(size_t degree, const double complex *v, double complex *temp,
                     double complex *roots)
{
    size_t n    = degree;
    size_t logn = (size_t)log2(degree);

    // -- Correctness: ifft(fft(vec)) * (1/n) = vec
    // -- Save vec for comparison later. Write to temp to apply fft/ifft in-place
    print_poly_double_complex("vec               ", v, n);
    double complex *v_fft = temp;
    memcpy(v_fft, v, n * sizeof(double complex));
    print_poly_double_complex("vec               ", v_fft, n);

#ifdef SE_FFT_LOAD_FULL
    se_assert(roots);
    load_fft_roots(n, roots);
#elif defined(SE_FFT_ONE_SHOT)
    se_assert(roots);
    calc_fft_roots(n, logn, roots);
#endif

    // -- Note: 'roots' will be ignored if SE_FFT_OTF is chosen
    fft_inpl(v_fft, n, logn, roots);
    print_poly_double_complex("vec (after fft)   ", v, n);

#ifdef SE_IFFT_LOAD_FULL
    se_assert(roots);
    load_ifft_roots(n, roots);
    print_poly_double_complex("roots               ", roots, n);
    // print_poly_double_complex_full("roots               ", roots, n);
#elif defined(SE_IFFT_ONE_SHOT)
    se_assert(roots);
    calc_ifft_roots(n, logn, roots);
#endif

    // -- Note: 'roots' will be ignored if SE_IFFT_OTF is chosen
    ifft_inpl(v_fft, n, logn, roots);
    print_poly_double_complex("vec (after ifft)  ", v_fft, n);

    poly_div_inpl_complex(v_fft, n, n);
    print_poly_double_complex("vec (after *(1/n))", v_fft, n);

    double maxdiff = 0.0001;
    bool err       = compare_poly_double_complex(v_fft, v, n, maxdiff);
    se_assert(!err);
}

/**
FFT test function

@param[in] n  Polynomial ring degree (ignored if SE_USE_MALLOC is defined)
*/
void test_fft(size_t n)
{
#ifndef SE_USE_MALLOC
    se_assert(n == SE_DEGREE_N);  // sanity check
    if (n != SE_DEGREE_N) n = SE_DEGREE_N;
#endif

        // -- Note: For multiplication tests, v1 and v2 must not be
        //    larger than length n/2 in data and should be zero
        //    padded up to length n. Result vector will be length n.

        // clang-format off
#ifdef SE_USE_MALLOC
    size_t ifft_roots_size = 0;
#if defined(SE_IFFT_LOAD_FULL) || defined(SE_IFFT_ONE_SHOT)
    ifft_roots_size = n;
#endif

    size_t fft_roots_size = 0;
#if defined(SE_FFT_LOAD_FULL) || defined(SE_FFT_ONE_SHOT)
    fft_roots_size = n;
#endif

    size_t roots_size       = ifft_roots_size ? ifft_roots_size : fft_roots_size; 
    size_t mempool_size     = 4*n + roots_size;
    double complex *mempool = calloc(mempool_size, sizeof(double complex));
#else
    double complex mempool[4*SE_DEGREE_N + IFFT_TEST_ROOTS_MEM + FFT_TEST_ROOTS_MEM];
    size_t roots_size   = IFFT_TEST_ROOTS_MEM + FFT_TEST_ROOTS_MEM;
    size_t mempool_size = 4*n + roots_size;
    memset(&mempool, 0, mempool_size * sizeof(double complex));
#endif

    size_t start_idx        = 0;
    double complex *v1    = &(mempool[start_idx]);                  start_idx += n;
    double complex *v2    = &(mempool[start_idx]);                  start_idx += n;
    double complex *v_exp = &(mempool[start_idx]);                  start_idx += n;
    double complex *temp  = &(mempool[start_idx]);                  start_idx += n;
    double complex *roots = roots_size ? &(mempool[start_idx]) : 0; start_idx += roots_size;
    se_assert(start_idx == mempool_size);
    // clang-format on

    Parms parms;
    set_parms_ckks(n, 1, &parms);
    print_test_banner("fft/ifft", &parms);

    for (size_t testnum = 0; testnum < 15; testnum++)
    {
        printf("\n--------------- Test: %zu -----------------\n", testnum);
        clear_double_complex(mempool, mempool_size);
        switch (testnum)
        {
            case 0: set_double_complex(v1, n, 1); break;  // {1, 1, 1, ... }
            case 1: set_double_complex(v1, n, 2); break;  // {2, 2, 2, ... }
            case 2:                                       // {0, 1, 2, 3, ...}
                for (size_t i = 0; i < n; i++)
                { v1[i] = (double complex)_complex((double)i, (double)0); }
                break;
            case 3:
                for (size_t i = 0; i < n; i++)
                { v1[i] = (double complex)(gen_double_eighth(10), (double)0); }
                break;
            case 4:
                for (size_t i = 0; i < n; i++)
                { v1[i] = (double complex)(gen_double_quarter(100), (double)0); }
                break;
            case 5:
                for (size_t i = 0; i < n; i++)
                { v1[i] = (double complex)(gen_double_half(-100), (double)0); }
                break;
            case 6:
                for (size_t i = 0; i < n; i++)
                { v1[i] = (double complex)(gen_double(1000), (double)0); }
                break;
            case 7:  // {1, 0, 0, ...} * {2, 2, 2, ...} = {2, 2, 2, ..., 0, 0, ...}
                v1[0] = (double complex)_complex((double)1, (double)0);
                set_double_complex(v2, n / 2, 2);
                set_double_complex(v_exp, n / 2, 2);
                break;
            case 8:  // {-1, 0, 0, ...} * {2, 2, 2, ...} = {-2, -2, -2, ..., 0, 0,
                     // ...}
                v1[0] = (double complex)_complex((double)-1, (double)0);
                set_double_complex(v2, n / 2, (flpt)2);
                set_double_complex(v_exp, n / 2, (flpt)(-2));
                break;
            case 9:  // {1, 0, 0, ...} * {-2, -2, -2, ...} = {-2, -2, -2, ..., 0, 0,
                     // ...}
                v1[0] = (double complex)_complex((double)1, (double)0);
                set_double_complex(v2, n / 2, (flpt)(-2));
                set_double_complex(v_exp, n / 2, (flpt)(-2));
                break;
            case 10:  // {1, 1, 1, ...} * {2, 2, 2, ...} = {2, 4, 8, ..., 4, 2, 0}
                set_double_complex(v1, n / 2, (flpt)1);
                set_double_complex(v2, n / 2, (flpt)2);
                for (size_t i = 0; i < n / 2; i++)
                { v_exp[i] = (double complex)_complex((double)(2 * (i + 1)), (double)0); }
                for (size_t i = 0; i < (n / 2) - 1; i++)
                { v_exp[i + n / 2] = v_exp[n / 2 - (i + 2)]; }
                break;
            case 11:
                for (size_t i = 0; i < n / 2; i++)
                {
                    v1[i] = (double complex)_complex(gen_double_eighth(pow(10, 1)), (double)0);
                    v2[i] = (double complex)_complex(gen_double_eighth(pow(10, 1)), (double)0);
                }
                break;
            case 12:
                for (size_t i = 0; i < n / 2; i++)
                {
                    v1[i] = (double complex)_complex(gen_double_quarter(-pow(10, 2)), (double)0);
                    v2[i] = (double complex)_complex(gen_double_quarter(-pow(10, 2)), (double)0);
                }
                break;
            case 13:
                for (size_t i = 0; i < n / 2; i++)
                {
                    v1[i] = (double complex)(gen_double_half(pow(10, 3)), (double)0);
                    v2[i] = (double complex)(gen_double_half(pow(10, 3)), (double)0);
                }
                break;
            case 14:
                for (size_t i = 0; i < n / 2; i++)
                {
                    v1[i] = (double complex)(gen_double(pow(10, 6)), (double)0);
                    v2[i] = (double complex)(gen_double(pow(10, 6)), (double)0);
                }
                break;
        }
        if (testnum < 7)
            test_fft_helper(n, v1, temp, roots);
        else if (testnum < 11)
            test_fft_mult_helper(n, v1, v2, v_exp, temp, roots);
        else
            test_fft_mult_helper(n, v1, v2, 0, temp, roots);
    }
#ifdef SE_USE_MALLOC
    if (mempool)
    {
        free(mempool);
        mempool = 0;
    }
#endif
}

#ifndef SE_USE_MALLOC
#ifdef IFFT_TEST_ROOTS_MEM
#undef IFFT_TEST_ROOTS_MEM
#endif
#ifdef FFT_TEST_ROOTS_MEM
#undef FFT_TEST_ROOTS_MEM
#endif
#endif

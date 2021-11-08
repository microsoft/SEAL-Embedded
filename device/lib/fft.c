// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file fft.c

Note: All roots are mod 2n, where n is the number of elements (e.g. polynomial degree) in the
transformed vector. See the paper for more details.
*/

#include "fft.h"

#include "defines.h"
#include "util_print.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
Helper function to calculate the angle of a particular root

@param[in] k  Index of root
@param[in] m  Degree of roots (i.e. 2n)
@returns      Angle of the root
*/
double calc_angle(size_t k, size_t m)
{
    return 2 * M_PI * (double)k / (double)(m);
}

/**
Helper function to calculates an FFT root

@param[in] k  Index of root to calculate
@param[in] m  Degree of roots (i.e., 2n, where n is the transform size)
@returns      The FFT root for index k
*/
double complex calc_root_otf(size_t k, size_t m)
{
    // -- Note: See calc_root_from_base above for more info
    k &= m - 1;
    double angle = calc_angle(k, m);
    return (double complex)_complex(cos(angle), sin(angle));
}

void calc_fft_roots(size_t n, size_t logn, double complex *roots)
{
    se_assert(n >= 4);
    se_assert(roots);

    PolySizeType m = n << 1;
    for (size_t i = 0; i < n; i++) { roots[i] = calc_root_otf(bitrev(i, logn), m); }
}

void calc_ifft_roots(size_t n, size_t logn, double complex *ifft_roots)
{
    se_assert(n >= 4);
    se_assert(ifft_roots);

    PolySizeType m = n << 1;
    for (size_t i = 0; i < n; i++)
    {
        ifft_roots[i] = se_conj(calc_root_otf(bitrev(i - 1, logn), m));
        // ifft_roots[i] = se_conj(calc_root_otf(bitrev(i - 1, logn) + 1, m));
    }
}

void ifft_inpl(double complex *vec, size_t n, size_t logn, const double complex *roots)
{
#if defined(SE_IFFT_LOAD_FULL) || defined(SE_IFFT_ONE_SHOT)
    se_assert(roots);
    size_t root_idx = 1;
#elif defined(SE_IFFT_OTF)
    SE_UNUSED(roots);
    size_t m = n << 1;  // Degree of roots
#else
    printf("IFFT option not found!\n");
    exit(0);
#endif

    /*
        Example: n = 8, logn = 3 :

        A0 _______ .............
        A1 _______X_________    \
                            \    X
        A2 _______ ..........X../
        A3 _______X_________/
                                        etc...
        A4 _______ .............
        A5 _______X_________    \
                            \    X
        A6 _______ ..........X../
        A7 _______X_________/
              Round 0       Round 1

        Note the following:
         - 'X' represents a butterfly cross
         - A 'group' is a collection of overlapping butterflies
         - Butterfly 'size' is the difference between butterfly indices
         - Round 0: 4 groups of size 1 butterflies
         - Round 1: 2 groups of size 2 bufferflies
         - Round 2: 1 group of size 4 butterflies

         Round 0: tt = 1, h = 4
           j = 0, k = [0-1), butterfly pair: (0, 1)
           j = 1, k = [2-3), butterfly pair: (2, 3)
           j = 2, k = [4-5), butterfly pair: (4, 5)
           j = 3, k = [6-7), butterfly pair: (6, 7)
         Round 1: tt = 2, h = 2
           j = 0, k = [0-2), butterfly pairs: (0, 2), (1, 3)
           j = 1, k = [4-6), butterfly pairs: (4, 6), (5, 7)
         Round 2: tt = 4, h = 1
           j = 0, k = [0-4), butterfly pairs: (0, 4), (1, 5), (2, 6), (3, 7)
    */
    size_t tt = 1;                                      // size of butterflies
    size_t h  = n / 2;                                  // number of groups
    for (size_t i = 0; i < logn; i++, tt *= 2, h /= 2)  // rounds
    {
        for (size_t j = 0, kstart = 0; j < h; j++, kstart += 2 * tt)  // groups
        {
#if defined(SE_IFFT_LOAD_FULL) || defined(SE_IFFT_ONE_SHOT)
            // -- The roots are assumed to be stored in a bit-reversed order
            //    in this case so that memory accesses are consecutive.
            double complex s = roots[root_idx++];
#elif defined(SE_IFFT_OTF)

            double complex s = se_conj(calc_root_otf(bitrev(h + j, logn), m));  // pairs
#else
            printf("Error! IFFT option not found!\n");
            exit(1);
#endif
            for (size_t k = kstart; k < (kstart + tt); k++)
            {
                // -- Use doubles to preserve precision
                double complex u = vec[k];
                double complex v = vec[k + tt];
                vec[k]           = u + v;
                vec[k + tt]      = (u - v) * s;
            }
        }
    }
}

void fft_inpl(double complex *vec, size_t n, size_t logn, const double complex *roots)
{
    // print_poly_double_complex("vec[3603] ", &(vec[3603]), 1);
    size_t m = n << 1;  // Degree of roots
#ifdef SE_FFT_OTF
    SE_UNUSED(roots);
#else
    se_assert(roots);
#endif

    /*
        Example: n = 8, logn = 3 :
        (see ckks_ifft above for illustration of ifft, ie inverse of fft)

        Note the following:
         - A 'group' is a collection of overlapping butterflies
         - Butterfly 'size' is the difference between butterfly indices
         - Round 0: 1 group of size 4 butterflies
         - Round 1: 2 groups of size 2 bufferflies
         - Round 2: 4 groups of size 1 butterflies

         Round 0: tt = 4, h = 1
           j = 0, k = [0-4), butterfly pairs: (0, 4), (1, 5), (2, 6), (3, 7)
         Round 1: tt = 2, h = 2
           j = 0, k = [0-2), butterfly pairs: (0, 2), (1, 3)
           j = 1, k = [4-6), butterfly pairs: (4, 6), (5, 7)
         Round 2: tt = 4, h = 1
           j = 0, k = [0-1), butterfly pair: (0, 1)
           j = 1, k = [2-3), butterfly pair: (2, 3)
           j = 2, k = [4-5), butterfly pair: (4, 5)
           j = 3, k = [6-7), butterfly pair: (6, 7)
    */

    // -- This code is pretty much the same as the ifft code above,
    //    except that 'h' and 'tt' are basically switched, fft roots are
    //    used instead of inverse roots, and the butterfly multiplication
    //    happens before the addition/subtraction instead of after
    size_t h  = 1;
    size_t tt = n / 2;
#if defined(SE_FFT_LOAD_FULL) || defined(SE_FFT_ONE_SHOT)
    size_t root_idx = 1;
#endif
    for (size_t i = 0; i < logn; i++, h *= 2, tt /= 2)  // rounds
    {
        for (size_t j = 0, kstart = 0; j < h; j++, kstart += 2 * tt)  // groups
        {
            double complex s;
#if defined(SE_FFT_LOAD_FULL) || defined(SE_FFT_ONE_SHOT)
            // -- The roots are assumed to be stored in a bit-reversed order
            //    in this case so that memory accesses are consecutive.
            s = roots[root_idx++];
#elif defined(SE_FFT_OTF)
            s = calc_root_otf(bitrev(h + j, logn), m);
#else
            printf("Error! FFT option not found!\n");
            exit(1);
#endif
            for (size_t k = kstart; k < (kstart + tt); k++)  // pairs
            {
                // -- Use doubles to preserve precision
                double complex u = vec[k];
                double complex v = vec[k + tt] * s;
                vec[k]           = u + v;
                vec[k + tt]      = u - v;
            }
        }
    }
}

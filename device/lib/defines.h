// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file defines.h
*/

#pragma once

#include <complex.h>
#include <inttypes.h>  // PRIu32, PRIu64 // not even sure this is needed...
#include <stdint.h>    // uint64_t, UINT64_MAX
#include <stdio.h>
#include <stdlib.h>  // size_t
#include <string.h>  // memset

#include "user_defines.h"

// clang-format off
// ==============================================================
//           Configurations pertaining to testing
// ==============================================================
/**
Inverse NTT type. Options 1 - 3 not guaranteed to always work.

0 = compute "on-the-fly"
1 = compute "one-shot"
2 = load 
3 = load fast
*/
#define SE_INTT_TYPE 0

// ==============================================================
//           Configurations pertaining to benchmarking    
// ==============================================================

/**
Include timer code for benchmarking.
Uncomment to use.
*/
// #define SE_ENABLE_TIMERS

// ==============================================================
//           Configurations pertaining to debugging      
// ==============================================================

// --- If defined, "print_poly" functions will only print PRINT_LEN_SMALL elements 
//     of the requested polynomial, (unless a "full" version of "print_poly" is called)
#define SE_PRINT_SMALL
#define PRINT_LEN_SMALL 8 

// #define SE_DEBUG_WITH_ZEROS
// #define SE_DEBUG_NO_ERRORS
// #define SE_VERBOSE_TESTING

// ==============================================================
//                   Advanced configurations
//              (Unlikely to need modification)
// ==============================================================

/**
Number of bytes to store the seed for the prng. 

For compressed public keys and/or compressed ciphertexts 
(in symmetric mode), must match prng byte count used in SEAL.
*/
#define SE_PRNG_SEED_BYTE_COUNT 64

/**
Data path to use if SE_DATA_PATH is not set in CMAKE
*/
#define SE_DATA_PATH_ "adapter_output_data"

/**
Data path length to use if SE_DATA_PATH is not set in CMAKE.
Must be >= length of the SE_DATA_PATH_ define above (in bytes).

(Most compilers are ok with this calling strlen strlen, but 
some are not.)
*/
#define SE_DATA_PATH_LEN_ strlen(SE_DATA_PATH_)

// ==============================================================
//
//
//                DO NOT MODIFY BELOW THIS LINE
//
//
// ==============================================================

// --------------------------------------------------------------
//      Configurations dervied from user_defines.h and above. 
//                       Do not modify.
// --------------------------------------------------------------



// -- Longest file type adapter output data to be loaded to device storage
//    is named: intt_fast_roots_<degree>_<prime>.dat
// -- max prime bitlen = 32    --> max # chars for prime = 10
// --       max degree = 32768 --> max # chars for prime = 5
// -- total max char count = 21 + 10 + 5 = 36  --> Round to 40
#define MAX_DATA_FILE_SIZE 40

#if !defined(SE_DATA_PATH) || !defined(SE_DATA_PATH_LEN)
    #undef SE_DATA_PATH
    #define SE_DATA_PATH SE_DATA_PATH_ 
    #define MAX_FPATH_SIZE SE_DATA_PATH_LEN_ + 1 + MAX_DATA_FILE_SIZE
#else
    #define MAX_FPATH_SIZE SE_DATA_PATH_LEN + 1 + MAX_DATA_FILE_SIZE
#endif

// ----- Assert type
#  if (SE_ASSERT_TYPE == 0)
    // -- Do nothing
#elif (SE_ASSERT_TYPE == 1)
    #define SE_ASSERT_STANDARD
#elif (SE_ASSERT_TYPE == 2)
    #define SE_ASSERT_CUSTOM
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// ----- Randomness generation type
#  if (SE_RAND_TYPE == 0)
    // -- Do nothing
#elif (SE_RAND_TYPE == 1)
    #define SE_RAND_GETRANDOM 
#elif (SE_RAND_TYPE == 2)
    #define SE_RAND_NRF5      
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// ----- Inverse FFT type
#  if (SE_IFFT_TYPE == 0)
    #define SE_IFFT_OTF
#elif (SE_IFFT_TYPE == 1)
    #define SE_IFFT_LOAD_FULL
// #elif (SE_IFFT_TYPE == 2)
// #define SE_IFFT_ONE_SHOT  // Not yet supported
// #elif (SE_IFFT_TYPE == 3)
// #define SE_IFFT_LOAD_BASE // Not yet supported
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// ----- NTT type
#  if (SE_NTT_TYPE == 0)
    #define SE_NTT_OTF
#elif (SE_NTT_TYPE == 1)
    #define SE_NTT_ONE_SHOT
#elif (SE_NTT_TYPE == 2)
    #define SE_NTT_REG
#elif (SE_NTT_TYPE == 3)
    #define SE_NTT_FAST
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// ----- Index map type
#  if (SE_INDEX_MAP_TYPE == 0)
    #define SE_INDEX_MAP_OTF
#elif (SE_INDEX_MAP_TYPE == 1)
    #define SE_INDEX_MAP_PERSIST
#elif (SE_INDEX_MAP_TYPE == 2)
    #define SE_INDEX_MAP_LOAD
#elif (SE_INDEX_MAP_TYPE == 3)
    #define SE_INDEX_MAP_LOAD_PERSIST
#elif (SE_INDEX_MAP_TYPE == 4)
    #define SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// ----- Secret key type
#  if (SE_SK_TYPE == 0)
    #define SE_SK_NOT_PERSISTENT 
#elif (SE_SK_TYPE == 1)
    #define SE_SK_PERSISTENT_ACROSS_PRIMES
#elif (SE_SK_TYPE == 2)
    #define SE_SK_PERSISTENT
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// ----- Data load type
#  if (SE_DATA_LOAD_TYPE == 0)
    // -- Do nothing
#elif (SE_DATA_LOAD_TYPE == 1)
    #define SE_DATA_FROM_CODE_COPY 
#elif (SE_DATA_LOAD_TYPE == 2)
    #define SE_DATA_FROM_CODE_DIRECT
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

/**
FFT type. For now, we only support "on-the-fly" for the FFT type.

0 = compute "on-the-fly"
1 = load                 (not yet supported)
2 = compute "one-shot"   (not yet supported)
3 = load from base roots (not yet supported)
*/
#define SE_FFT_TYPE 0

// ----- FFT type (for testing only)
// ----- Note: FFT type must not require more memory than IFFT type
#  if (SE_FFT_TYPE == 0)
    #define SE_FFT_OTF
#elif (SE_FFT_TYPE == 1)
    #define SE_FFT_LOAD_FULL // Not yet supported
#elif (SE_FFT_TYPE == 2)
    #define SE_FFT_ONE_SHOT  // Not yet supported
#elif (SE_FFT_TYPE == 3)
    #define SE_FFT_LOAD_BASE // Not yet supported
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// ----- Inverse NTT type (for testing only)
// ----- Note: INTT type must not require more memory than NTT type
#  if (SE_INTT_TYPE == 0)
    #define SE_INTT_OTF
#elif (SE_INTT_TYPE == 1)
    #define SE_INTT_ONE_SHOT
#elif (SE_INTT_TYPE == 2)
    #define SE_INTT_REG
#elif (SE_INTT_TYPE == 3)
    #define SE_INTT_FAST
#else
    #ifndef SE_CONFIG_ERROR
    #define SE_CONFIG_ERROR
    #endif
#endif

// --------------------------------------------------------------
//             Non-configuration. Do not modify.
// --------------------------------------------------------------

// -- No asserts in release mode
#ifdef SE_UNDEF_ASSERT_FORCE
#ifdef SE_ASSERT_STANDARD
#undef SE_ASSERT_STANDARD
#endif
#ifdef SE_ASSERT_CUSTOM
#undef SE_ASSERT_CUSTOM
#endif
#endif

#define SE_UNUSED(x) \
    do               \
    {                \
        (void)(x);   \
    } while (0)
#ifdef SE_ASSERT_STANDARD
#include <assert.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#define se_assert(x) assert(x)
#define se_assert_msg(x, y) \
    do                      \
    {                       \
        if (!(x))           \
        {                   \
            printf(y);      \
            printf("\n");   \
            assert(x);      \
        }                   \
    } while (0)
#elif defined(SE_ASSERT_CUSTOM)
#define se_assert(x)                                                    \
    do                                                                  \
    {                                                                   \
        if (!(x))                                                       \
        {                                                               \
            printf("Error: A SEAL-Embedded assert failed. Exiting..."); \
            while (1)                                                   \
                ;                                                       \
        }                                                               \
    } while (0)
#define se_assert_msg(x, y) \
    do                      \
    {                       \
        if (!(x))           \
        {                   \
            printf(y);      \
            printf("\n");   \
            while (1)       \
                ;           \
        }                   \
    } while (0)
#else
#ifndef NDEBUG
#define NDEBUG
#endif
#define se_assert(x)  \
    do                \
    {                 \
        SE_UNUSED(x); \
    } while (0)
#define se_assert_msg(x, y) \
    do                      \
    {                       \
        SE_UNUSED(x);       \
        SE_UNUSED(y);       \
    } while (0)
#endif

#ifndef __has_builtin       // Optional of course
#define __has_builtin(x) 0  // Compatibility with non-clang compilers.
#endif

#if __has_builtin(__builtin_complex)
#define _complex(x, y) __builtin_complex(x, y)
#else
#ifdef I
//#define _complex(x, y) CMPLX(x, y)
#define _complex(x, y) x + y*I
#else
#define _complex(x, y) x + y*(1.0fi)
#endif
#endif

#ifdef SE_USE_PREDEF_COMPLEX_FUNCS
    #define se_conj(x) conj(x)
    #define se_creal(x) creal(x)
    #define se_cimag(x) cimag(x)
#else
static inline double complex se_conj(double complex val)
{
    double *val_double = (double *)(&val);
    return _complex(val_double[0], -val_double[1]);
}
static inline double se_creal(double complex val)
{
    double *val_double = (double *)(&val);
    return val_double[0];
}
static inline double se_cimag(double complex val)
{
    double *val_double = (double *)(&val);
    return val_double[1];
}
#endif

typedef size_t PolySizeType;
#ifdef SE_PRIMESIZE_64
    typedef uint64_t ZZ;
    typedef int64_t ZZsign;  // signed type
    typedef double flpt;
    #define PRIuZZ PRIu64
    #define PRIiZZ PRIi64
#else
    typedef uint32_t ZZ;
    typedef int32_t ZZsign; // signed ZZ type
    // typedef double flpt;
    typedef float flpt;
    #define PRIuZZ PRIu32
    #define PRIiZZ PRIi32
#endif

/**
Utility function to clear an array

Size req: 'v' must contain at least n ZZ values

@param[in] v  Array to clear
@param[in] n  Number of elements of v to set to 0
*/
static inline void clear(ZZ *v, PolySizeType n)
{
    if (v)
    {
        memset(v, 0, n * sizeof(ZZ));
    }
}

/**
Secure utility function to clear an array. Prevents compiler optimizing out memset().

Size req: 'v' must contain at least n ZZ values

@param[in] v  Array to clear
@param[in] n  Number of elements of v to set to 0
*/
static inline void se_secure_zero_memset(void *v, size_t n)
{
    static void *(*const volatile memset_v)(void *, int, size_t) = &memset;
    memset_v(v, 0, n);
}


// --------------------------------------------------------------
//                   Sanity Checks. Do not modify.
// --------------------------------------------------------------
#ifdef SE_ON_NRF5
    #undef SE_RAND_GETRANDOM
    // #if !defined(SE_DATA_FROM_CODE_COPY) && !defined(SE_DATA_FROM_CODE_DIRECT)
    #ifndef SE_DATA_FROM_CODE_COPY
    #define SE_DATA_FROM_CODE_COPY
    #endif
#else
    #undef SE_RAND_NRF5
    #undef SE_NRF5_UART_PRINTF_ENABLED
#endif

#ifdef SE_ON_SPHERE_M4
    #undef SE_RAND_GETRANDOM
    #undef SE_USE_MALLOC
    // #if !defined(SE_DATA_FROM_CODE_COPY) && !defined(SE_DATA_FROM_CODE_DIRECT)
    #ifndef SE_DATA_FROM_CODE_COPY
    #define SE_DATA_FROM_CODE_COPY
    #endif
#endif

// -- Only need to check one case since only 2 cases are possible
#ifdef SE_ASSERT_STANDARD
    #undef SE_ASSERT_CUSTOM
#endif

// -- Only need to check one case since only 2 cases are possible
#ifdef SE_RAND_GETRANDOM
    #undef SE_RAND_NRF5
#endif

// -- Only need to check one case since only 2 cases are possible
#ifdef SE_DATA_FROM_CODE_COPY
    #undef SE_DATA_FROM_CODE_DIRECT
#endif

#ifdef SE_INDEX_MAP_OTF
    #undef SE_INDEX_MAP_LOAD
    #undef SE_INDEX_MAP_PERSIST
    #undef SE_INDEX_MAP_LOAD_PERSIST
    #undef SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM
#elif defined(SE_INDEX_MAP_LOAD)
    #undef SE_INDEX_MAP_PERSIST
    #undef SE_INDEX_MAP_LOAD_PERSIST
    #undef SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM
#elif defined(SE_INDEX_MAP_PERSIST)
    #undef SE_INDEX_MAP_LOAD_PERSIST
    #undef SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM
#elif defined(SE_INDEX_MAP_LOAD_PERSIST)
    #undef SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM
#elif defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
    // -- Do nothing
#else
    #define SE_INDEX_MAP_OTF
#endif

// -- IFFT defines checking. (The order of these is important)
#ifdef SE_IFFT_OTF
    #undef SE_IFFT_ONE_SHOT
    #undef SE_IFFT_LOAD_FULL
    #define SE_FFT_OTF
#elif defined(SE_IFFT_ONE_SHOT)
    #undef SE_IFFT_LOAD_FULL
    #undef SE_FFT_LOAD_FULL
#elif defined(SE_IFFT_LOAD_FULL)
    // -- Do nothing
#else
    #define SE_IFFT_OTF
    #define SE_FFT_OTF
#endif

// -- FFT defines checking. (The order of these is important)
#ifdef SE_FFT_OTF
    #undef SE_FFT_ONE_SHOT
    #undef SE_FFT_LOAD_FULL
#elif defined(SE_FFT_ONE_SHOT)
    #undef SE_FFT_LOAD_FULL
#elif defined(SE_FFT_LOAD_FULL)
    // -- Do nothing
#else
    #define SE_FFT_OTF
#endif

// -- NTT defines checking. (The order of these is important)
#ifdef SE_NTT_OTF
    #undef SE_NTT_ONE_SHOT
    #undef SE_NTT_REG
    #undef SE_NTT_FAST
    #define SE_INTT_OTF
#elif defined(SE_NTT_ONE_SHOT)
    #undef SE_NTT_REG
    #undef SE_NTT_FAST
    #undef SE_INTT_FAST
#elif defined(SE_NTT_REG)
    #undef SE_NTT_FAST
    #undef SE_INTT_FAST
#elif defined(SE_NTT_FAST)
    // -- Do nothing
#else
    #define SE_NTT_OTF
    #define SE_INTT_OTF
#endif

// -- INTT defines checking. (The order of these is important)
#ifdef SE_INTT_OTF
    #undef SE_INTT_ONE_SHOT
    #undef SE_INTT_REG
    #undef SE_INTT_FAST
#elif defined(SE_INTT_ONE_SHOT)
    #undef SE_INTT_REG
    #undef SE_INTT_FAST
#elif defined(SE_INTT_REG)
    #undef SE_INTT_FAST
#elif defined(SE_INTT_FAST)
    // -- Do nothing
#else
    #define SE_INTT_OTF
#endif

// -- This must be after the IFFT sanity checks
#ifdef SE_REVERSE_CT_GEN_ENABLED
    #if !(defined(SE_IFFT_OTF) && defined(SE_FFT_OTF))
        #undef SE_REVERSE_CT_GEN_ENABLED
    #endif
#endif

#ifdef SE_SK_PERSISTENT
    #undef SE_SK_PERSISTENT_ACROSS_PRIMES
    #undef SE_SK_NOT_PERSISTENT
#elif defined(SE_SK_PERSISTENT_ACROSS_PRIMES)
    #undef SE_SK_NOT_PERSISTENT
#elif defined(SE_SK_NOT_PERSISTENT)
    // -- Do nothing
#else
    #define SE_SK_NOT_PERSISTENT
#endif


// -- This must be afer sk and ntt and ifft and index map types are set
#ifdef SE_IFFT_OTF 
    // -- We don't have any ifft memory to work with, but we may have ntt memory
    #ifdef SE_NTT_OTF
        // -- We don't have any ntt memory to work with either
        #if defined(SE_INDEX_MAP_LOAD) 
            // -- If sym, we have nowhere to load index_map, so can indicate
            //    memory as persistent
            // -- Optimization: Index_map and SK can share memory if SK is not
            //    set as persistent
            #if defined(SE_SK_PERSISTENT)
                // -- Indicate index map load memory as persistent
                #undef SE_INDEX_MAP_LOAD
                #define SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM
            #elif defined(SE_SK_NOT_PERSISTENT)
                // -- SK can be at least persistent across primes
                #undef SE_SK_NOT_PERSISTENT
                #define SE_SK_PERSISTENT_ACROSS_PRIMES
            #endif
            #ifdef SE_SK_PERSISTENT_ACROSS_PRIMES
                #define SE_SK_INDEX_MAP_SHARED
            #endif
        #endif
    #endif
#endif

// clang-format on

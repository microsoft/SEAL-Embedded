// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file util_print.h

Helpful print functions
*/

#pragma once

#include <complex.h>
#include <math.h>     // fabs
#include <stdbool.h>  // bool
#include <stdint.h>   // uint64_t
#include <stdlib.h>   // size_t

#include "defines.h"
#include "sample.h"  // get_small_poly_idx

// clang-format off
#ifdef SE_ON_SPHERE_A7
    #include <applibs/log.h>  // Log_Debug
    #define printf(...) Log_Debug(__VA_ARGS__)
#elif defined(SE_ON_SPHERE_M4)
    #include "printf.h"
    // #include "mt3620.h"
    // #include "os_hal_uart.h"
    // #include "os_hal_gpt.h"
#elif defined(SE_ON_NRF5)
    #include "sdk_config.h"
#else
    #include <stdio.h>
#endif

#ifdef SE_NRF5_UART_PRINTF_ENABLED
    #include "app_error.h"
    #include "app_uart.h"
    #include "bsp.h"  // defines for UART_PRESENT and UARTE_PRESENT
    #include "nrf.h"
    #ifdef UART_PRESENT
        #include "nrf_uart.h"
    #endif
    #ifdef UARTE_PRESENT
        #include "nrf_uarte.h"
    #endif
#endif
// clang-format on

#ifdef SE_ON_SPHERE_M4
extern void _putchar(char character);
#endif

/**
The printf string for printing floating point values. Modify to change the printing precision.
*/
#define SE_PRINT_PREC_STR "%0.2f"

/**
The printf string for printing complex floating point values. Modify to change the printing
precision.
*/
#define SE_PRINT_PREC_CMPLX_STR "%0.2f + %0.2fi"

/**
Helper function to return the value of the print length == min(len, PRINT_LEN_SMALL)

@param[in] len
@returns   Print length
*/
static inline size_t get_print_len(size_t len)
{
#ifdef SE_PRINT_SMALL
    if (PRINT_LEN_SMALL < len) return PRINT_LEN_SMALL;
#endif
    return len;
}

/**
Helper function to print a comma

@param[in] idx Current idx
@param[in] len
*/
static inline void print_comma(size_t idx, size_t len)
{
    if (idx < (len - 1))
        printf(", ");
    else
        printf(" ");
}

/**
Helper function to print the end string.

@param[in] print_len
@param[in] len
*/
static inline void print_end_string(size_t print_len, size_t len)
{
    const char *end_str = (len == print_len) ? "}\n" : "... }\n";
    printf("%s", end_str);
}

/**
Prints an unsigned integer value with name.

@param[in] name  Name of value
@param[in] val   Value to print
*/
static inline void print_zz(const char *name, ZZ val)
{
    printf("%s: %" PRIuZZ "\n", name, val);
}

/**
Prints an unsigned integer value.

@param[in] val  Value to print
*/
static inline void print_zz_plain(ZZ val)
{
    printf("%" PRIuZZ, val);
}

/**
Prints a signed integer value.

@param[in] val  Value to print
*/
static inline void print_zzi_plain(ZZsign val)
{
    printf("%" PRIiZZ, val);
}

/**
Print banner for memory pool size.

@param[in] mempool_size  Size of the memory pool
*/
static inline void print_mem_use(size_t mempool_size)
{
    printf("Total memory required: %zu bytes (= %zu x sizeof(uint32_t) = %zu KB)\n",
           mempool_size * sizeof(ZZ), mempool_size, mempool_size * sizeof(ZZ) / 1024);
}

/**
Prints an array of double complex values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' double complex values (or min(PRINT_LEN_SMALL, len) double
complex values, if SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_double_complex(const char *name, const double complex *a,
                                             PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        if (fabs(se_cimag(a[i])) > 0.0001)
            printf(SE_PRINT_PREC_CMPLX_STR, se_creal(a[i]), se_cimag(a[i]));
        else
            printf(SE_PRINT_PREC_STR, se_creal(a[i]));

        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of double complex values

Size_req: 'a' must have at least 'len' double complex values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_double_complex_full(const char *name, const double complex *a,
                                                  PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        if (fabs(se_cimag(a[i])) > 0.0001)
            printf(SE_PRINT_PREC_CMPLX_STR, se_creal(a[i]), se_cimag(a[i]));
        else
            printf(SE_PRINT_PREC_STR, se_creal(a[i]));

        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of type flpt values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' flpt values (or min(PRINT_LEN_SMALL, len) flpt values, if
SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_flpt(const char *name, const flpt *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    se_assert(print_len == 8);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        printf(SE_PRINT_PREC_STR, (flpt)a[i]);
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of type flpt values

Size_req: 'a' must have at least 'len' flpt values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_flpt_full(const char *name, const flpt *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        printf(SE_PRINT_PREC_STR, (flpt)a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of doubles

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' double values (or min(PRINT_LEN_SMALL, len) double values, if
SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_double(const char *name, const double *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        printf(SE_PRINT_PREC_STR, a[i]);
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of doubles

Size_req: 'a' must have at least 'len' double values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_double_full(const char *name, const double *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        printf(SE_PRINT_PREC_STR, a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of type ZZsign values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' ZZsign values (or min(PRINT_LEN_SMALL, len) ZZsign values, if
SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_sign(const char *name, const ZZsign *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        print_zzi_plain((ZZsign)(a[i]));
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of type ZZsign values

Size_req: 'a' must have at least 'len' ZZsign values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_sign_full(const char *name, const ZZsign *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        print_zzi_plain((ZZsign)(a[i]));
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of int8_t values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' int8_t values (or min(PRINT_LEN_SMALL, len) int8_t values, if
SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_int8(const char *name, const int8_t *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        printf("%" PRIi8, a[i]);
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of int8_t values

Size_req: 'a' must have at least 'len' int8_t values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_int8_full(const char *name, const int8_t *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        printf("%" PRIi8, a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of int64_t values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' int64_t values (or min(PRINT_LEN_SMALL, len) int64_t values,
if SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_int64(const char *name, const int64_t *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        printf("%" PRIi64, a[i]);
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of int64_t values

Size_req: 'a' must have at least 'len' int64_t values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_int64_full(const char *name, const int64_t *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        printf("%" PRIi64, a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of uint64_t values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' uint64_t values (or min(PRINT_LEN_SMALL, len) uint64_t
values, if SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_uint64(const char *name, const uint64_t *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        printf("%" PRIu64, a[i]);
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of uint64_t values

Size_req: 'a' must have at least 'len' uint64_t values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_uint64_full(const char *name, const uint64_t *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        printf("%" PRIu64, a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of type-ZZ values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' type-ZZ values (or min(PRINT_LEN_SMALL, len) type-ZZ values,
if SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly(const char *name, const ZZ *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        print_zz_plain(a[i]);
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of type-ZZ values

Size_req: 'a' must have at least 'len' type-ZZ values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_full(const char *name, const ZZ *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        print_zz_plain(a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of type-ZZ values along with their indices.

Size_req: 'a' must have at least 'len' type-ZZ values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_debug_full(const char *name, const ZZ *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        print_zz_plain(a[i]);
        printf(" (%zu)", i);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of values stored in a compressed (2 bits per value) form.

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 2*'len' bits (or 2*min(PRINT_LEN_SMALL, len) bits, if
SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_small(const char *name, const ZZ *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        uint8_t val = get_small_poly_idx(a, i);
        print_zz_plain((ZZ)val);
        print_comma(i, len);
    }
    print_end_string(print_len, len);
}

/**
Prints an array of values stored in a compressed (2 bits per value) form.

Size_req: 'a' must have at least 2*'len' bits

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_small_full(const char *name, const ZZ *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        uint8_t val = get_small_poly_idx(a, i);
        print_zz_plain((ZZ)val);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints the array storing the values of a ternary polynomial.
Calls the corrrect print function depending on if 'a' is in small/compressed form.

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: If small == 1, 'a' must have at least 2*'len' bits (or 2*min(PRINT_LEN_SMALL, len) bits,
if SE_PRINT_SMALL is defined). Else, 'a' must have at least 'len' type-ZZ values (or
min(PRINT_LEN_SMALL, len) type-ZZ values, if SE_PRINT_SMALL is defined).

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
@param[in] small Set to 1 if 'a' is in small/compressed form
*/
static inline void print_poly_ternary(const char *name, const ZZ *a, PolySizeType len, bool small)
{
    if (small)
        print_poly_small(name, a, len);
    else
        print_poly(name, a, len);
}

/**
Prints the array storing the values of a ternary polynomial.
Calls the corrrect print function depending on if 'a' is in small/compressed form.

Size_req: If small == 1, 'a' must have at least 2*'len' bits.
Else, 'a' must have at least 'len' type-ZZ values.

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
@param[in] small Set to 1 if 'a' is in small/compressed form
*/
static inline void print_poly_ternary_full(const char *name, const ZZ *a, PolySizeType len,
                                           bool small)
{
    if (small)
        print_poly_small_full(name, a, len);
    else
        print_poly_full(name, a, len);
}

/**
Prints an array of uint16_t values

Note: If SE_PRINT_SMALL is defined, will only print up to PRINT_LEN_SMALL elements of 'a'

Size_req: 'a' must have at least 'len' uint16_t values (or min(PRINT_LEN_SMALL, len) uint16_t
values, if SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_uint16(const char *name, const uint16_t *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        printf("%" PRIu16, (uint16_t)a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of uint16_t values

Size_req: 'a' must have at least 'len' uint16_t values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_uint16_full(const char *name, const uint16_t *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        printf("%" PRIu16, (uint16_t)a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of uint8_t values

Size_req: 'a' must have at least 'len' uint8_t values (or min(PRINT_LEN_SMALL, len)
uint8_t values, if SE_PRINT_SMALL is defined)

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_uint8(const char *name, const uint8_t *a, PolySizeType len)
{
    size_t print_len = get_print_len(len);
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < print_len; i++)
    {
        printf("%" PRIu8, (uint8_t)a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints an array of uint8_t values

Size_req: 'a' must have at least 'len' uint8_t values

@param[in] name  Name of array
@param[in] a     Array to print
@param[in] len   Number of elements of 'a' to print
*/
static inline void print_poly_uint8_full(const char *name, const uint8_t *a, PolySizeType len)
{
    printf("%s : { ", name);
    for (PolySizeType i = 0; i < len; i++)
    {
        printf("%" PRIu8, (uint8_t)a[i]);
        print_comma(i, len);
    }
    printf("}\n");
}

/**
Prints a banner with chosen configuration options.

@param[in] sym  Set to 1 if in symmetric mode (will just toggle whether secret key type option is
                printed)
*/
static inline void print_config(bool sym)
{
    const char *platform_str    = "      Platform target  :";
    const char *debug_str       = "                 Mode  :";
    const char *values_str      = "    Values allocation  :";
    const char *assembly_str    = "Using inline assembly? :";
    const char *malloc_str      = "         Using malloc? :";
    const char *predef_cplx_str = " Using predef complex? :";
    const char *timers_str      = "       Timers enabled? :";
    const char *getrand_str     = "   Randomness enabled? :";
    const char *ct_rev_str      = "   Reverse ct enabled? :";
    const char *data_load_str   = "       Data load type  :";
    const char *assert_str      = "          Assert type  :";
    const char *ifft_str        = "            IFFT type  :";
    const char *ntt_str         = "             NTT type  :";
    const char *index_map_str   = "       Index map type  :";
    const char *s_str           = "      Secret key type  :";
    // const char *zz_str          = "              ZZ type  :";

#ifdef SE_ON_SPHERE_A7
    printf("%s Azure Sphere A7 Core (#define SE_ON_SPHERE_A7)\n", platform_str);
#elif defined(SE_ON_SPHERE_M4)
    printf("%s Azure Sphere M4 Core (#define SE_ON_SPHERE_M4)\n", platform_str);
#else
    printf("%s Generic\n", platform_str);
#endif

// -- Debug type
#if defined(NDEBUG) && !defined(SE_DEBUG_WITH_ZEROS) && !defined(SE_VERBOSE_TESTING)
    printf(
        "%s Non-Debug (#define NDEBUG, but SE_DEBUG_WITH_ZEROS and "
        "SE_VERBOSE_TESTING not defined)\n",
        debug_str);
#elif defined(NDEBUG) && (defined(SE_DEBUG_WITH_ZEROS) || defined(SE_VERBOSE_TESTING))
    printf("%s Debug (#define NDEBUG, SE_DEBUG_WITH_ZEROS, SE_VERBOSE_TESTING)\n", debug_str);
#elif !defined(NDEBUG)
    printf("%s Debug (NDEBUG is not defined)\n", debug_str);
#endif

#ifdef SE_MEMPOOL_ALLOC_VALUES
    printf("%s Part of memory pool (#define SE_MEMPOOL_ALLOC_VALUES)\n", values_str);
#else
    printf("%s Not part of memory pool\n", values_str);
#endif

#ifdef SE_USE_ASM_ARITH
    printf("%s Yes (#define SE_USE_ASM_ARITH)\n", assembly_str);
#else
    printf("%s No\n", assembly_str);
#endif

#ifdef SE_USE_MALLOC
    printf("%s Yes (#define SE_USE_MALLOC)\n", malloc_str);
#else
    printf("%s No\n", malloc_str);
    printf("Degree N: %zu\n", (size_t)SE_DEGREE_N);
    printf("Number of primes: %zu\n", (size_t)SE_NPRIMES);
#endif

#ifdef SE_USE_PREDEF_COMPLEX_FUNCS
    printf("%s Yes (#define SE_USE_PREDEF_COMPLEX_FUNCS)\n", predef_cplx_str);
#else
    printf("%s No\n", predef_cplx_str);
#endif

#ifdef SE_ENABLE_TIMERS
    printf("%s Yes (#define SE_ENABLE_TIMERS)\n", timers_str);
#else
    printf("%s No\n", timers_str);
#endif

#ifdef SE_RAND_GETRANDOM
    printf("%s Yes (#define SE_RAND_GETRANDOM)\n", getrand_str);
#elif defined(SE_RAND_NRF5)
    printf("%s Yes (#define SE_RAND_NRF5)\n", getrand_str);
#else
    printf("%s No\n", getrand_str);
#endif

#ifdef SE_REVERSE_CT_GEN_ENABLED
    printf("%s Yes (#define SE_REVERSE_CT_GEN_ENABLED)\n", ct_rev_str);
#else
    printf("%s No\n", ct_rev_str);
#endif

#ifdef SE_DATA_FROM_CODE_COPY
    printf("%s from code copy (#define SE_DATA_FROM_CODE_COPY)\n", data_load_str);
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    printf("%s from code direct (#define SE_DATA_FROM_CODE_DIRECT)\n", data_load_str);
#else
    printf("%s from file (requires an operating system)\n", data_load_str);
#endif

// -- Assert type
#ifdef SE_ASSERT_STANDARD
#ifdef NDEBUG
    printf(
        "%s Standard/None (with NDEBUG defined) (assert.h, #define "
        "SE_ASSERT_STANDARD)\n",
        assert_str);
#else
    printf(
        "%s Standard (without NDEBUG defined) (assert.h, #define "
        "SE_ASSERT_STANDARD)\n",
        assert_str);
#endif
#elif defined(SE_ASSERT_CUSTOM)
    printf("%s Custom (se_assert(), #define SE_ASSERT_CUSTOM)\n", assert_str);
#else
    printf("%s None\n", assert_str);
#endif

    // printf("%s uint32_t\n", zz_str);

#ifdef SE_IFFT_OTF
    printf("%s compute on-the-fly (#define SE_IFFT_OTF)\n", ifft_str);
#elif defined(SE_IFFT_ONE_SHOT)
    printf("%s compute one-shot (#define SE_IFFT_ONE_SHOT)\n", ifft_str);
#elif defined(SE_IFFT_LOAD_FULL)
    printf("%s pre-alloc & load (#define SE_IFFT_LOAD_FULL)\n", ifft_str);
#else
    printf("%s Error! IFFT type not chosen\n");
    while (1)
        ;
#endif

#ifdef SE_NTT_OTF
    printf("%s compute on-the-fly (#define SE_NTT_OTF)\n", ntt_str);
#elif defined(SE_NTT_ONE_SHOT)
    printf("%s compute one-shot (#define SE_NTT_ONE_SHOT)\n", ntt_str);
#elif defined(SE_NTT_REG)
    printf("%s load (from flash) (#define SE_NTT_REG)\n", ntt_str);
#elif defined(SE_NTT_FAST)
    printf("%s load fast (aka \"lazy\") (from flash) (#define SE_NTT_FAST)\n", ntt_str);
#elif defined(SE_NTT_NONE)
    printf("%s None (i.e.. Toom-Cook Karatsuba) (#define SE_NTT_NONE)\n", ntt_str);
#else
    printf("%s Error! NTT type not chosen\n");
    while (1)
        ;
#endif

#ifdef SE_INDEX_MAP_OTF
    printf("%s compute on-the-fly (#define SE_INDEX_MAP_OTF)\n", index_map_str);
#elif defined(SE_INDEX_MAP_LOAD)
    printf("%s load (not persistent) (#define SE_INDEX_MAP_LOAD)\n", index_map_str);
#elif defined(SE_INDEX_MAP_PERSIST)
    printf("%s compute persistent (#define SE_INDEX_MAP_PERSIST)\n", index_map_str);
#elif defined(SE_INDEX_MAP_LOAD_PERSIST)
    printf("%s load + persistent (#define SE_INDEX_MAP_LOAD_PERSIST)\n", index_map_str);
#elif defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
    printf("%s load + persistent (#define SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)\n",
           index_map_str);
#else
    printf("%s Error! Index map type not chosen\n");
    while (1)
        ;
#endif

    if (sym)
    {
#ifdef SE_SK_NOT_PERSISTENT
        printf(
            "%s not persistent (L loads per sequence) (#define "
            "SE_SK_NOT_PERSISTENT)\n",
            s_str);
#elif defined(SE_SK_PERSISTENT_ACROSS_PRIMES)
        printf("%s persistent across primes (1 load per sequence) ", s_str);
        printf("(#define SE_SK_PERSISTENT_ACROSS_PRIMES)\n");
#elif defined(SE_SK_PERSISTENT)
        printf("%s truly persistent (1 load only) (#define SE_SK_PERSISTENT)\n", s_str);
#else
        printf("%s Error! Index map type not chosen\n");
        while (1)
            ;
#endif
    }
}

#ifdef SE_NRF5_UART_PRINTF_ENABLED
/**
Error handler for uart, required for uart on the NRF5.

@param p_event
*/
static inline void uart_error_handle(app_uart_evt_t *p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    { APP_ERROR_HANDLER(p_event->data.error_communication); }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}

// -- When UART is used for communication with the host do not use flow control
#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 256
#define UART_HWFC APP_UART_FLOW_CONTROL_DISABLED

/**
Sets up UART on the NRF. Required for printing.
*/
static inline void se_setup_uart()
{
    // bsp_board_init(BSP_INIT_LEDS);
    uint32_t err_code;
    const app_uart_comm_params_t comm_params = {
        RX_PIN_NUMBER,           TX_PIN_NUMBER, RTS_PIN_NUMBER, CTS_PIN_NUMBER, UART_HWFC, false,
#ifdef UART_PRESENT
        NRF_UART_BAUDRATE_115200
#else
        NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, uart_error_handle,
                       APP_IRQ_PRIORITY_LOWEST, err_code);
    // APP_ERROR_CHECK(err_code);
    se_assert(err_code == NRF_SUCCESS);
}
#endif

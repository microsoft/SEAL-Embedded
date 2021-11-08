// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file user_defines.h
*/

#pragma once

#include <complex.h>
#include <inttypes.h>  // PRIu32, PRIu64 // not even sure this is needed...
#include <stdint.h>    // uint64_t, UINT64_MAX
#include <stdio.h>
#include <stdlib.h>  // size_t
#include <string.h>  // memset

#ifdef SE_ON_SPHERE_M4
#include "mt3620.h"
#include "os_hal_dma.h"
#include "os_hal_gpt.h"
#include "os_hal_uart.h"
#endif

// ==============================================================================
//                           Basic configurations
//
//   Instructions: Configure each of the following by choosing a numbered value.
//     These must match the platform's capabilities or library will not work.
// ==============================================================================
/**
Assert type. Will be force-set to 0 if in cmake release mode.

0 = none (will define NDEBUG)
1 = standard using <assert.h>
2 = custom
*/
#define SE_ASSERT_TYPE 1

/**
Randomness generation type.

Note: Tests will only work if set to 0 on the Azure sphere M4 -- Not secure!
To use this library on the Sphere M4, you need to build a partner application
to sample randomness using an A7 application and pass it to this library.

0 = none (For debugging only. Will not pass all tests)
1 = use getrandom()
2 = nrf5 rng
*/
#define SE_RAND_TYPE 1

// ==============================================================================
//                       Basic configurations: Memory
//
//   Instructions: Configure each of the following by choosing a numbered value
//     for more control over memory allocation.
// ==============================================================================

/**
Inverse FFT type.

Note: On the Sphere M4, will not work if this is non-otf for n = 4K and in Debug mode.

0 = compute "on-the-fly"
1 = load
2 = compute "one-shot" (not yet supported)
*/
#define SE_IFFT_TYPE 0

/**
NTT type.

Note: On the Sphere M4, will not work if this is load-fast for n = 4K and in Debug mode.

0 = compute "on-the-fly"
1 = compute "one-shot"
2 = load
3 = load fast
*/
#define SE_NTT_TYPE 1

/**
Index map type.

If set to load (non-persist) and SK is persistent and IFFT and NTT are on the fly, will be
reset to option 4 (i.e. load persistent for symmetric, load for asymmetric)

0 = compute "on-the-fly"
1 = compute persistent
2 = load
3 = load persistent
4 = load persistent for symmetric, load for asymmetric
*/
#define SE_INDEX_MAP_TYPE 1

/**
Secret key type. (Ignored in asymmetric mode).

Note: If SE_IFFT_TYPE is set as 0, a setting of 1 here will be reset to 2 (same amount of
memory for better performance).

0 = not persistent
1 = persistent across primes
2 = persistent
*/
#define SE_SK_TYPE 2

/**
Data load type. Load method for precomputed keys, roots, and index map.

Note: On M4, this will be force set to 1.

0 = from file                              (uses file I/O)
1 = copy from headers into separate buffer (no file I/O)
2 = load from code directly                (not supported)
*/
#define SE_DATA_LOAD_TYPE 0

// =============================================================================
//                      Optional / Advanced Configurations
//
//   Instructions: Configure each of the following by commenting/uncommenting
// =============================================================================

/**
Include memory for "values" allocation as part of the initial memory pool.
Uncomment to use.
*/
#define SE_MEMPOOL_ALLOC_VALUES

/**
Uncomment to use predefine implementation for conj(), creal(), and cimag(). Otherwise,
treats complex values as two doubles (a real part followed by an imaginary part).
*/
#define SE_USE_PREDEF_COMPLEX_FUNCS

/**
Sometimes malloc is not available or stack memory allocation is more desireable.
Comment out to use stack allocation instead.
*/
#define SE_USE_MALLOC

/**
Optimization to generate prime components in reverse order every other time. Enables more
optimal memory usage and performance. Will be ignored if IFFT type is "compute on-the-fly"
or NTT type is "compute on-the-fly" or "compute one-shot". Uncomment to use.
TODO: Enable this feature in adapter.
*/
// #define SE_REVERSE_CT_GEN_ENABLED

/**
Explicitly sets some unnecessary function arguments (i.e., arguments only necessary for
testing) to 0 at the start of the function for better performance. If defined, testing
will no longer function correctly. Uncomment to use.
*/
// #define SE_DISABLE_TESTING_CAPABILITY

/**
Uncomment to use the 27-bit default for n=4K instead of the 30-bit default
Make sure to uncomment SEALE_DEFAULT_4K_27BIT in the adapter (see: utils.h) as well.
*/
// #define SE_DEFAULT_4K_27BIT

// =============================================================================
//                          Malloc off configurations
//
//   Instructions: Configurations that apply when SE_USE_MALLOC is undefined
//      Uncomment/comment as needed and set desired values for n, # of primes.
// =============================================================================
/**
Encryption type to use if malloc is not enabled (uncomment to use asymmetric encryption)
*/
#define SE_ENCRYPT_TYPE_SYMMETRIC

/**
Number of primes to use if malloc is not enabled.
*/
#define SE_NPRIMES 3

/**
Polynomial ring degree to use if malloc is not enabled
*/
#define SE_DEGREE_N 4096

// =============================================================================
//                        Bare-metal configurations
//
//   Instructions: Configure each of the following by commenting/uncommenting
//     These configurations will be ignored if platform type is A7 or none.
// =============================================================================
/**
Include the code file containing hard-coded public key values.
This file can be generated using the SEAL-Embedded adapter.
(Ignored if SE_DATA_LOAD_TYPE is "from file".)
*/
// #define SE_DEFINE_PK_DATA

/**
Include the code file containing hard-coded secret key values.
This file can be generated using the SEAL-Embedded adapter.
(Ignored if SE_DATA_LOAD_TYPE is "from file".)
*/
#define SE_DEFINE_SK_DATA

/**
Print using UART (will be disabled if platform type != NRF5).

If you are on NRF5 and want output to go to RTT, make sure RETARGET_ENABLED is set to 0 in
sdk_config.h (and that RTT is enabled in the configuration window) and comment out this
preprocessor.

If you do not want output to go to RTT, do the opposite of above and uncomment this
preprocessor.
*/
// #define SE_NRF5_UART_PRINTF_ENABLED

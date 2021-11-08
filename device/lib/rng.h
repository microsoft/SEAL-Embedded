// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file rng.h

Pseudo-random number generator.
*/

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>  // memset

#include "defines.h"
#include "inttypes.h"
#include "shake256/fips202.h"

#ifdef SE_RAND_GETRANDOM
#include <sys/random.h>  // getrandom
#elif defined(SE_RAND_NRF5)
#include "nrf_crypto.h"
#endif

typedef struct SE_PRNG
{
    uint8_t seed[SE_PRNG_SEED_BYTE_COUNT];
    uint64_t counter;
} SE_PRNG;

/**
Randomizes the seed of a PRNG object and resets its internal counter.

Size req: If seed_in != NULL, seed_in must be SE_PRNG_SEED_BYTE_COUNT long.

@param[in,out] prng     PRNG instance to modify
@param[in]     seed_in  [Optional]. Seed to seed the prng with.
*/
inline void prng_randomize_reset(SE_PRNG *prng, uint8_t *seed_in)
{
    se_assert(prng);
    prng->counter = 0;

    if (seed_in)
    {
        memcpy(&(prng->seed[0]), seed_in, SE_PRNG_SEED_BYTE_COUNT);
        return;
    }

#ifdef SE_RAND_GETRANDOM
    ssize_t ret = getrandom((void *)(&(prng->seed[0])), SE_PRNG_SEED_BYTE_COUNT, 0);
    se_assert(ret == SE_PRNG_SEED_BYTE_COUNT);
#elif defined(SE_RAND_NRF5)
    ret_code_t ret_val = nrf_crypto_rng_vector_generate(prng->seed, SE_PRNG_SEED_BYTE_COUNT);
    if (ret_val != NRF_SUCCESS)
    {
        printf("Error: Something went wrong with nrf_crypto_rng_vector_generate()\n");
        while (1)
            ;
    }
    se_assert(ret_val == NRF_SUCCESS);
#else
    // -- This should only be used for debugging!!
    memset(&(prng->seed[0]), 0, SE_PRNG_SEED_BYTE_COUNT);
    // prng->seed[0] = 1;
#endif
}

/**
Fills a buffer with random bytes expanded from a SE_PRNG's seed (and counter).
A call to this function updates the prng object's internal counter.

@param[in]      byte_count  Number of random bytes to generate
@param[in,out]  prng        PRNG instance
@param[out]     buffer      Buffer to store the random bytes
*/
inline void prng_fill_buffer(size_t byte_count, SE_PRNG *prng, void *buffer)
{
    uint8_t seed_ext[SE_PRNG_SEED_BYTE_COUNT + 8];
    memcpy(&(seed_ext[0]), &(prng->seed[0]), SE_PRNG_SEED_BYTE_COUNT);
    memcpy(&(seed_ext[SE_PRNG_SEED_BYTE_COUNT]), &(prng->counter), 8);
    shake256(buffer, byte_count, &(seed_ext[0]), SE_PRNG_SEED_BYTE_COUNT + 8);
    prng->counter++;
    if (prng->counter == 0)  // overflow!
    {
        printf("PRNG counter overflowed.");
        printf("Re-randomizing seed and resetting counter to 0.\n");
        prng_randomize_reset(prng, NULL);
    }
}

/**
Clears the values (both seed and counter) of a prng instance to 0.
Clears the bit that ties prng to a custom seed (so next prng_randomize_reset *will* generate a
random seed)

@param[in,out] prng  PRNG instance to clear
*/
inline void prng_clear(SE_PRNG *prng)
{
    memset(&(prng->seed[0]), 0, SE_PRNG_SEED_BYTE_COUNT);
    prng->counter = 0;
}

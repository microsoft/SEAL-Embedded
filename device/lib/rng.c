// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file rng.c

Pseudo-random number generator.
*/

#include "rng.h"

#include "defines.h"

// -- Need these to avoid duplicate symbols error

extern inline void prng_randomize_reset(SE_PRNG *prng, uint8_t *seed_in);
extern inline void prng_fill_buffer(size_t byte_count, SE_PRNG *prng, void *buffer);
extern inline void prng_clear(SE_PRNG *prng);

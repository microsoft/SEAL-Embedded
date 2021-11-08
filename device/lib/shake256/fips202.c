/*
Origin: Microsoft SEAL library (https://github.com/microsoft/SEAL) commit
2d2a25c916047d2f356d45ad50c98f0a9695905a.
*/

/*
This file is a part of the Kyber library (https://github.com/pq-crystals/kyber)
commit 844057468e69527bd15b17fbe03f4b61f9a22065. The Kyber library is licensed
under CC0 Universal, version 1.0. You can find a copy of this license at
https://creativecommons.org/publicdomain/zero/1.0/legalcode
Minor modifications to the original file have been made and marked
as `Microsoft SEAL edit: ...`.
*/

/* Based on the public domain implementation in
 * crypto_hash/keccakc512/simple/ from http://bench.cr.yp.to/supercop.html
 * by Ronny Van Keer
 * and the public domain "TweetFips202" implementation
 * from https://twitter.com/tweetfips202
 * by Gilles Van Assche, Daniel J. Bernstein, and Peter Schwabe */

#include <stddef.h>
#include <stdint.h>
/* Microsoft SEAL edit: changed the header file path */
#include "fips202.h"
#include "keccakf1600.h"

/* Microsoft SEAL edit: moved the rate macros here from Kyber header fips202.h
 */
#define SHAKE256_RATE 136

// Context for non-incremental API
typedef struct
{
    uint64_t ctx[25];
} shake256ctx;

/*************************************************
 * Name:        keccak_absorb
 *
 * Description: Absorb step of Keccak;
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - uint64_t *s:       pointer to (uninitialized) output Keccak state
 *              - uint32_t r:        rate in bytes (e.g., 168 for SHAKE128)
 *              - const uint8_t *m:  pointer to input to be absorbed into s
 *              - size_t mlen:       length of input in bytes
 *              - uint8_t p:         domain-separation byte for different
 *Keccak-derived functions
 **************************************************/
static void keccak_absorb(uint64_t *s, uint32_t r, const uint8_t *m, size_t mlen, uint8_t p)
{
    size_t i;
    uint8_t t[200];

    while (mlen >= r)
    {
        KeccakF1600_StateXORBytes(s, m, 0, r);
        KeccakF1600_StatePermute(s);
        mlen -= r;
        m += r;
    }

    for (i = 0; i < r; ++i) t[i] = 0;
    for (i = 0; i < mlen; ++i) t[i] = m[i];
    t[i] = p;
    t[r - 1] |= 128;

    KeccakF1600_StateXORBytes(s, t, 0, r);
}

/*************************************************
 * Name:        keccak_squeezeblocks
 *
 * Description: Squeeze step of Keccak. Squeezes full blocks of r bytes each.
 *              Modifies the state. Can be called multiple times to keep squeezing, i.e., is
 *              incremental.
 *
 * Arguments:   - uint8_t *h:     pointer to output blocks
 *              - size_t nblocks: number of blocks to be squeezed (written to h)
 *              - uint64_t *s:    pointer to in/output Keccak state
 *              - uint32_t r:     rate in bytes (e.g., 168 for SHAKE128)
 **************************************************/
static void keccak_squeezeblocks(uint8_t *h, size_t nblocks, uint64_t *s, uint32_t r)
{
    while (nblocks > 0)
    {
        KeccakF1600_StatePermute(s);
        KeccakF1600_StateExtractBytes(s, h, 0, r);
        h += r;
        nblocks--;
    }
}

/*************************************************
 * Name:        shake256
 *
 * Description: SHAKE256 XOF with non-incremental API
 *
 * Arguments:   - uint8_t *out:      pointer to output
 *              - size_t outlen:     requested output length in bytes
 *              - const uint8_t *in: pointer to input
 *              - size_t inlen:      length of input in bytes
 **************************************************/
void shake256(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen)
{
    shake256ctx state;
    uint8_t t[SHAKE256_RATE];
    size_t nblocks = outlen / SHAKE256_RATE;
    size_t i;

    for (i = 0; i < 25; ++i) { state.ctx[i] = 0; }

    /* Absorb input */
    keccak_absorb((uint64_t *)state.ctx, SHAKE256_RATE, input, inlen, 0x1F);

    /* Squeeze output */
    keccak_squeezeblocks(output, nblocks, (uint64_t *)state.ctx, SHAKE256_RATE);

    output += nblocks * SHAKE256_RATE;
    outlen -= nblocks * SHAKE256_RATE;

    if (outlen)
    {
        keccak_squeezeblocks(t, 1, (uint64_t *)state.ctx, SHAKE256_RATE);
        for (i = 0; i < outlen; i++) output[i] = t[i];
    }
}

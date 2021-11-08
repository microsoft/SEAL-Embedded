/*
Origin: pqm4 library (https://github.com/mupq/pqm4) commit
65f12c6c66f10492eac2b434e93fd1aee742f0e1.
*/

#ifndef KECCAKF1600_H
#define KECCAKF1600_H

#include <stdint.h>

void KeccakF1600_StateExtractBytes(uint64_t *state, unsigned char *data, unsigned int offset,
                                   unsigned int length);
void KeccakF1600_StateXORBytes(uint64_t *state, const unsigned char *data, unsigned int offset,
                               unsigned int length);
void KeccakF1600_StatePermute(uint64_t *state);

#endif

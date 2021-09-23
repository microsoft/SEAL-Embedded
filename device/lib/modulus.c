// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file modulus.c
*/

#include "modulus.h"

#include <stdint.h>  // uint64_t, UINT64_MAX
#include <string.h>  // memcpy

#include "defines.h"
#include "util_print.h"

void set_modulus_custom(const ZZ q, ZZ hw, ZZ lw, Modulus *mod)
{
    mod->value          = q;
    mod->const_ratio[1] = hw;
    mod->const_ratio[0] = lw;
}

bool set_modulus(const uint32_t q, Modulus *mod)
{
    switch (q)
    {
        // -- 30-bit ckks primes
        case 1073479681: set_modulus_custom(q, 0x4, 0x004003f0, mod); return 1;
        case 1073184769: set_modulus_custom(q, 0x4, 0x00881202, mod); return 1;
        case 1073053697: set_modulus_custom(q, 0x4, 0x00a81b84, mod); return 1;
        case 1072857089: set_modulus_custom(q, 0x4, 0x00d82d89, mod); return 1;
        case 1072496641: set_modulus_custom(q, 0x4, 0x01305a4a, mod); return 1;
        case 1071513601: set_modulus_custom(q, 0x4, 0x02212189, mod); return 1;
        case 1071415297: set_modulus_custom(q, 0x4, 0x02393baf, mod); return 1;

        // -- Other values for testing
        case 2: set_modulus_custom(q, 1UL << 31, 0, mod); return 1;
        case 3: set_modulus_custom(q, 0x55555555, 0x55555555, mod); return 1;
        case 10: set_modulus_custom(q, 0x19999999, 0x99999999, mod); return 1;
        case 0xFFFF: set_modulus_custom(q, 0x10001, 0x00010001, mod); return 1;
        case 0x10000: set_modulus_custom(q, 0x10000, 0x00000000, mod); return 1;
        case 1559578058: set_modulus_custom(q, 0x2, 0xc1017e44, mod); return 1;
        case 2147483647:
            set_modulus_custom(q, 0x2, 0x4, mod);  // Max Q
            return 1;

        default:
            printf("Modulus const ratio values not found for ");
            print_zz("Modulus value", q);
            printf("Please try set_modulus_custom instead.");
            return 0;
    }
    return 0;
}

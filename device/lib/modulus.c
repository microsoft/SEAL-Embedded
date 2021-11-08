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
        // -- Add cases for custom primes here

        // -- 27 bit primes
        case 134176769: set_modulus_custom(q, 0x20, 0x2802e03, mod); return 1;
        case 134111233: set_modulus_custom(q, 0x20, 0x6814e43, mod); return 1;
        case 134012929: set_modulus_custom(q, 0x20, 0xc84dfe5, mod); return 1;

        // -- 30-bit primes
        case 1062535169: set_modulus_custom(q, 0x4, 0xaccdb49, mod); return 1;
        case 1062469633: set_modulus_custom(q, 0x4, 0xadd3267, mod); return 1;
        case 1061093377: set_modulus_custom(q, 0x4, 0xc34cf30, mod); return 1;
        case 1060765697: set_modulus_custom(q, 0x4, 0xc86c0d4, mod); return 1;
        case 1060700161: set_modulus_custom(q, 0x4, 0xc9725e9, mod); return 1;
        case 1060175873: set_modulus_custom(q, 0x4, 0xd1a6142, mod); return 1;
        case 1058209793: set_modulus_custom(q, 0x4, 0xf07a84a, mod); return 1;
        case 1056440321: set_modulus_custom(q, 0x4, 0x10c52d4a, mod); return 1;
        case 1056178177: set_modulus_custom(q, 0x4, 0x11074e88, mod); return 1;
        case 1055260673: set_modulus_custom(q, 0x4, 0x11ef051e, mod); return 1;
        case 1054212097: set_modulus_custom(q, 0x4, 0x12f85437, mod); return 1;
        case 1054015489: set_modulus_custom(q, 0x4, 0x132a2218, mod); return 1;
        case 1053818881: set_modulus_custom(q, 0x4, 0x135bf4ba, mod); return 1;

        default:
            printf("Modulus const ratio values not found for ");
            print_zz("Modulus value", q);
            printf("Please try set_modulus_custom instead.");
            return 0;
    }
    return 0;
}

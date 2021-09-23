// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ntt.c
*/

#include "ntt.h"

#include <stdio.h>

#include "defines.h"
#include "fft.h"
#include "fileops.h"
#include "parameters.h"
#include "polymodarith.h"
#include "uintmodarith.h"
#include "util_print.h"

#if defined(SE_NTT_OTF) || defined(SE_NTT_ONE_SHOT)

/**
Helper function to return root for certain modulus prime values if SE_NTT_OTF or
SE_NTT_ONE_SHOT is used. Implemented as a table lookup.

@param[in] n  Transform size (i.e. polynomial ring degree)
@param[in] q  Modulus value
*/
ZZ get_ntt_root(size_t n, ZZ q)
{
    // TODO: Add more primes
    ZZ root;
    switch (n)
    {
        case 16:
            switch (q)
            {
                case 1071415297: root = 161442378; break;
                case 1071513601: root = 113726644; break;
                case 1072496641: root = 15809412; break;
                default: {
                    printf("Error! Need first power of root, n = 16\n");
                    print_zz("Modulus value", q);
                    exit(1);
                }
            }
            break;
        case 4096:
            switch (q)
            {
                case 1071415297: root = 12202; break;
                case 1071513601: root = 143907; break;
                case 1072496641: root = 27507; break;
                default: {
                    printf("Error! Need first power of root for ntt, n = 4K\n");
                    print_zz("Modulus value", q);
                    exit(1);
                }
            }
            break;
        default: {
            printf("Error! Need first power of root for ntt\n");
            print_zz("Modulus value", q);
            exit(1);
        }
    }
    return root;
}
#endif

void ntt_roots_initialize(const Parms *parms, ZZ *ntt_roots)
{
#ifdef SE_REVERSE_CT_GEN_ENABLED
    SE_UNUSED(parms);
    SE_UNUSED(ntt_roots);
    if (parms->skip_ntt_load) return;
#endif

#ifdef SE_NTT_OTF
    SE_UNUSED(parms);
    SE_UNUSED(ntt_roots);
    return;
#endif

    se_assert(parms && parms->curr_modulus && ntt_roots);

#ifdef SE_NTT_ONE_SHOT
    size_t n     = parms->coeff_count;
    size_t logn  = parms->logn;
    Modulus *mod = parms->curr_modulus;

    ZZ root      = get_ntt_root(n, mod->value);
    ZZ power     = root;
    ntt_roots[0] = 1;  // Not necessary but set anyway
    for (size_t i = 1; i < n; i++)
    {
        ntt_roots[bitrev(i, logn)] = power;
        power                      = mul_mod(power, root, mod);
    }
#elif defined(SE_NTT_FAST)
    load_ntt_fast_roots(parms, (MUMO *)ntt_roots);
#elif defined(SE_NTT_REG)
    load_ntt_roots(parms, ntt_roots);
#else
    se_assert(0);
#endif
}

#ifdef SE_NTT_FAST
/**
Performs a "fast" (a.k.a. "lazy") negacyclic in-place NTT using the Harvey butterfly. Only
used if "SE_NTT_FAST" is defined. See SEAL_Embedded paper for a more detailed description.
"Lazy"-ness refers to fact that we here we lazily opt to reduce values only at the very
end.

@param[in]     parms           Parameters set by ckks_setup
@param[in]     ntt_fast_roots  NTT roots set by ntt_roots_initialize
@param[in,out] vec             Input/output polynomial of n ZZ elements
*/
void ntt_lazy_inpl(const Parms *parms, const MUMO *ntt_fast_roots, ZZ *vec)
{
    se_assert(parms && ntt_fast_roots && vec);
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;
    ZZ two_q     = mod->value << 1;

    // -- Return the NTT in scrambled order
    size_t h  = 1;
    size_t tt = n / 2;
    // size_t root_idx = 1;

    for (int i = 0; i < parms->logn; i++, h *= 2, tt /= 2)  // Rounds
    {
        // print_poly_full("s in ntt", vec, n);
        for (size_t j = 0, kstart = 0; j < h; j++, kstart += 2 * tt)  // Groups
        {
            const MUMO *s = &(ntt_fast_roots[h + j]);
            // const MUMO *s = &(ntt_fast_roots[bitrev(h + j, parms->logn)]);
            // const MUMO *s = &(ntt_fast_roots[root_idx++]);

            // -- The Harvey butterfly. Assume val1, val2 in [0, 2p)
            // -- Return vec[k], vec[k+tt] in [0, 4p)
            for (size_t k = kstart; k < (kstart + tt); k++)  // Pairs
            {
                ZZ val1 = vec[k];
                ZZ val2 = vec[k + tt];

                ZZ u = val1 - (two_q & (ZZ)(-(ZZsign)(val1 >= two_q)));
                ZZ v = mul_mod_mumo_lazy(val2, s, mod);

                // -- We know these will not generate carries/overflows
                vec[k]      = u + v;
                vec[k + tt] = u + two_q - v;
            }
        }
    }
}
#else

/**
Performs a negacyclic in-place NTT using the Harvey butterfly. Only used if SE_NTT_FAST is
not defined.

If SE_NTT_REG or SE_NTT_ONE_SHOT is defined, will use regular NTT computation.
Else, (SE_NTT_OTF is defined), will use truly "on-the-fly" NTT computation. In this last
case, 'ntt_roots' may be null (and will be ignored).

@param[in]     parms      Parameters set by ckks_setup
@param[in]     ntt_roots  NTT roots set by ntt_roots_initialize. Ignored if SE_NTT_OTF is
defined.
@param[in,out] vec        Input/output polynomial of n ZZ elements
*/
void ntt_non_lazy_inpl(const Parms *parms, const ZZ *ntt_roots, ZZ *vec)
{
    se_assert(parms && parms->curr_modulus && vec);

    size_t n = parms->coeff_count;
    size_t logn = parms->logn;
    Modulus *mod = parms->curr_modulus;

    // -- Return the NTT in scrambled order
    size_t h = 1;
    size_t tt = n / 2;

    #ifdef SE_NTT_OTF
    SE_UNUSED(ntt_roots);
    ZZ root = get_ntt_root(n, mod->value);
    #endif

    for (int i = 0; i < logn; i++, h *= 2, tt /= 2)  // rounds
    {
        for (size_t j = 0, kstart = 0; j < h; j++, kstart += 2 * tt)  // groups
        {
    #ifdef SE_NTT_OTF
            // printf("h+j: %zu\n", h+j);
            // ZZ power = bitrev(h+j, logn);
            // ZZ s     = exponentiate_uint_mod(root, power, mod);
            ZZ power = h + j;
            ZZ s = exponentiate_uint_mod_bitrev(root, power, logn, mod);
    #else
            se_assert(ntt_roots);
            ZZ s = ntt_roots[h + j];
    #endif
            // -- The Harvey butterfly. Assume val1, val2 in [0, 2p)
            // -- Return vec[k], vec[k+tt] in [0, 4p)
            for (size_t k = kstart; k < (kstart + tt); k++)  // pairs
            {
                ZZ u = vec[k];
                ZZ v = mul_mod(vec[k + tt], s, mod);
                vec[k] = add_mod(u, v, mod);       // vec[k]    = u + v;
                vec[k + tt] = sub_mod(u, v, mod);  // vec[k+tt] = u - v;
            }
        }
    }
}
#endif

void ntt_inpl(const Parms *parms, const ZZ *ntt_roots, ZZ *vec)
{
    se_assert(parms && parms->curr_modulus && vec);
#ifdef SE_NTT_FAST
    se_assert(ntt_roots);
    ntt_lazy_inpl(parms, (MUMO *)ntt_roots, vec);
    // print_poly_full("vec", vec, parms->coeff_count);

    // -- Finally, we might need to reduce coefficients modulo q, but we know each
    //    coefficient is in the range [0, 4q). Since word size is controlled, this
    //    should be fast.
    ZZ q     = parms->curr_modulus->value;
    ZZ two_q = q << 1;
    for (size_t i = 0; i < parms->coeff_count; i++)
    {
        if (vec[i] >= two_q) vec[i] -= two_q;
        if (vec[i] >= q) vec[i] -= q;
    }
#else
    ntt_non_lazy_inpl(parms, ntt_roots, vec);
#endif
}

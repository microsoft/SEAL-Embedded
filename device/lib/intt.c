// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file intt.c
*/

#include "intt.h"

#include <stdio.h>

#include "defines.h"
#include "fft.h"
#include "fileops.h"
#include "parameters.h"
#include "polymodarith.h"
#include "uintmodarith.h"
#include "util_print.h"

#if defined(SE_INTT_OTF) || defined(SE_INTT_ONE_SHOT)

/**
Helper function to return root for certain modulus prime values if SE_INTT_OTF or
SE_INTT_ONE_SHOT is used. Implemented as a table lookup.

@param[in] n  Transform size (i.e. polynomial ring degree)
@param[in] q  Modulus value
*/
ZZ get_intt_root(size_t n, ZZ q)
{
    // TODO: Add more primes
    ZZ inv_root;
    switch (n)
    {
        case 16:
            switch (q)
            {
                case 1071415297: inv_root = 615162833; break;
                case 1071513601: inv_root = 208824698; break;
                case 1072496641: inv_root = 750395333; break;
                default: {
                    printf("Error! Need first power of root for intt, n = 16\n");
                    print_zz("Modulus value", q);
                    exit(1);
                }
            }
            break;
        case 4096:
            switch (q)
            {
                case 1071415297: inv_root = 710091420; break;
                case 1071513601: inv_root = 711691669; break;
                case 1072496641: inv_root = 85699917; break;
                default: {
                    printf("Error! Need first power of root for intt, n = 4K\n");
                    print_zz("Modulus value", q);
                    exit(1);
                }
            }
            break;
        default: {
            printf("Error! Need first power of root for intt\n");
            print_zz("Modulus value", q);
            exit(1);
        }
    }
    return inv_root;
}
#endif

void intt_roots_initialize(const Parms *parms, ZZ *intt_roots)
{
#ifdef SE_INTT_OTF
    SE_UNUSED(parms);
    SE_UNUSED(intt_roots);
    return;
#endif

    se_assert(parms && parms->curr_modulus && intt_roots);

#ifdef SE_INTT_ONE_SHOT
    size_t n     = parms->coeff_count;
    size_t logn  = parms->logn;
    Modulus *mod = parms->curr_modulus;

    ZZ inv_root   = get_intt_root(n, mod->value);
    ZZ power      = inv_root;
    intt_roots[0] = 1;  // Not necessary but set anyway
    for (size_t i = 1; i < n; i++)
    {
        intt_roots[bitrev(i - 1, logn) + 1] = power;
        power                               = mul_mod(power, inv_root, mod);
    }
#elif defined(SE_INTT_FAST)
    load_intt_fast_roots(parms, (MUMO *)intt_roots);
#elif defined(SE_INTT_REG)
    load_intt_roots(parms, intt_roots);
#else
    se_assert(0);
#endif
}

#ifdef SE_INTT_FAST
/**
Performs a "fast" (a.k.a. "lazy") negacyclic in-place inverse NTT using the Harvey
butterfly, using "fast" INTT roots. Only used if "SE_INTT_FAST" is defined. See
SEAL_Embedded paper for a more detailed description. "Lazy"-ness refers to fact that we
here we lazily opt to reduce values only at the very end.

@param[in]     parms            Parameters set by ckks_setup
@param[in]     intt_fast_roots  INTT roots set by intt_roots_initialize
@param[in]     inv_n            Inverse of n mod q, where q is the value of the current
modulus prime
@param[in]     inv_n_w
@param[in,out] vec              Input/output polynomial of n ZZ elements
*/
void intt_lazy_inpl(const Parms *parms, const MUMO *intt_fast_roots, const MUMO *inv_n,
                    const MUMO *inv_n_w, ZZ *vec)
{
    se_assert(parms && ntt_lazy_roots && vec);
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;
    ZZ two_q     = mod->value << 1;

    size_t tt       = 1;      // size of butterflies
    size_t h        = n / 2;  // number of groups
    size_t root_idx = 1;      // We technically don't need to store the 0th one...

    // print_poly_full("vec", vec, n);
    for (size_t i = 0; i < (parms->logn - 1); i++, tt *= 2, h /= 2)  // rounds
    {
        for (size_t j = 0, kstart = 0; j < h; j++, kstart += 2 * tt)  // groups
        {
            const MUMO *s = &(intt_fast_roots[root_idx++]);

            for (size_t k = kstart; k < (kstart + tt); k++)  // pairs
            {
                ZZ u = vec[k];
                ZZ v = vec[k + tt];

                ZZ val1 = u + v;
                ZZ val2 = u + two_q - v;

                vec[k]      = val1 - (two_q & (ZZ)(-(ZZsign)(val1 >= two_q)));
                vec[k + tt] = mul_mod_mumo_lazy(val2, s, mod);
            }
        }
        // print_poly_full("vec", vec, n);
    }

    // print_poly_full("vec", vec, n);
    for (size_t j = 0; j < n / 2; j++)
    {
        ZZ u = vec[j];
        ZZ v = vec[j + n / 2];

        ZZ val1 = u + v;
        ZZ val2 = u + two_q - v;

        ZZ tval1 = val1 - (two_q & (ZZ)(-(ZZsign)(val1 >= two_q)));

        vec[j]         = mul_mod_mumo_lazy(tval1, inv_n, mod);
        vec[j + n / 2] = mul_mod_mumo_lazy(val2, inv_n_w, mod);
    }
    // print_poly_full("vec", vec, n);
}
#else
/**
Performs a negacyclic in-place inverse NTT using the Harvey butterfly. Only used if
SE_INTT_FAST is not defined.

If SE_INTT_REG or SE_INTT_ONE_SHOT is defined, will use regular INTT computation.
Else, (SE_INTT_OTF is defined), will use truly "on-the-fly" INTT computation. In this last
case, 'intt_roots' may be null (and will be ignored).

@param[in]     parms       Parameters set by ckks_setup
@param[in]     intt_roots  INTT roots set by intt_roots_initialize. Ignored if SE_INTT_OTF
is defined.
@param[in]     inv_n       Inverse of n mod q, where q is the value of the current modulus
prime
@param[in]     inv_n_w
@param[in,out] vec         Input/output polynomial of n ZZ elements
*/
void intt_non_lazy_inpl(const Parms *parms, const ZZ *intt_roots, ZZ inv_n, ZZ inv_n_w,
                        ZZ *vec)
{
    // -- See ntt.c for a more detailed explaination of algorithm

    se_assert(parms && parms->curr_modulus && vec);

    size_t n = parms->coeff_count;
    size_t logn = parms->logn;
    Modulus *mod = parms->curr_modulus;

    size_t tt = 1;     // size of butterflies
    size_t h = n / 2;  // number of groups

    #ifdef SE_INTT_OTF
    SE_UNUSED(intt_roots);
    ZZ root = get_intt_root(n, mod->value);
    #elif defined(SE_INTT_ONE_SHOT) || defined(SE_INTT_REG)
    size_t root_idx = 1;  // We technically don't need to store the 0th one...
    #endif

    for (size_t i = 0; i < (logn - 1); i++, tt *= 2, h /= 2)  // rounds
    {
        for (size_t j = 0, kstart = 0; j < h; j++, kstart += 2 * tt)  // groups
        {
    #ifdef SE_INTT_OTF
            // ZZ power = bitrev(h + j, logn);
            // ZZ s     = exponentiate_uint_mod(root, power, mod);
            ZZ power = h + j;
            ZZ s = exponentiate_uint_mod_bitrev(root, power, logn, mod);
    #elif defined(SE_INTT_ONE_SHOT) || defined(SE_INTT_REG)
            se_assert(intt_roots);
            ZZ s = intt_roots[root_idx++];
    #endif
            for (size_t k = kstart; k < (kstart + tt); k++)  // pairs
            {
                ZZ u = vec[k];
                ZZ v = vec[k + tt];
                vec[k] = add_mod(u, v, mod);
                vec[k + tt] = mul_mod(sub_mod(u, v, mod), s, mod);
            }
        }
        // print_poly_full("vec", vec, n);
    }
    // print_poly_full("vec", vec, n);

    // -- Finally, need to multiply by n^{-1}
    for (size_t i = 0; i < n / 2; i++)
    {
        ///*
        ZZ u = vec[i];
        ZZ v = vec[i + n / 2];
        vec[i] = mul_mod(add_mod(u, v, mod), inv_n, mod);
        vec[i + n / 2] = mul_mod(sub_mod(u, v, mod), inv_n_w, mod);
        //*/
        // mul_mod_inpl(&(vec[i]),         inv_n, mod);
        // mul_mod_inpl(&(vec[i + n / 2]), inv_n, mod);
    }
}
#endif

void intt_inpl(const Parms *parms, const ZZ *intt_roots, ZZ *vec)
{
    se_assert(parms && parms->curr_modulus);
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;

    ZZ inv_n, inv_n_w;

    // -- Note: inv_n equals n^(-1) mod qi
    //    and inv_n_w equals (n * w)^(-1) mod qi
    //    where w = first (non-inverse) ntt root
    switch (mod->value)
    {
        case 1071415297:
            switch (n)
            {
                // w = 595045951
                case 16:
                    inv_n   = 1004451841;
                    inv_n_w = 967261469;
                    break;
                case 1024: inv_n = 1070368993; inv_n_w = 601043701;
                case 2048: inv_n = 1070892145; inv_n_w = 836229499;
                case 4096:
                    inv_n   = 1071153721;
                    inv_n_w = 953822398;
                    break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;

        case 1071513601:
            switch (n)
            {
                // w = 677515442
                case 16:
                    inv_n   = 1004544001;
                    inv_n_w = 91594485;
                    break;
                case 1024:
                    inv_n   = 1070467201;
                    inv_n_w = 46399391;
                    break;
                case 2048:
                    inv_n   = 1070990401;
                    inv_n_w = 92798782;
                    break;
                case 4096:
                    inv_n   = 1071252001;
                    inv_n_w = 46399391;
                    break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;

        case 1072496641:
            switch (n)
            {
                // w = 652017833
                case 16:
                    inv_n   = 1005465601;
                    inv_n_w = 562528246;
                    break;
                case 1024:
                    inv_n   = 1071449281;
                    inv_n_w = 176367104;
                    break;
                case 2048:
                    inv_n   = 1071972961;
                    inv_n_w = 88183552;
                    break;
                case 4096:
                    inv_n   = 1072234801;
                    inv_n_w = 44091776;
                    break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;
        default: printf("Error with intt inv_n\n"); exit(1);
    }
#ifdef SE_INTT_FAST
    se_assert(intt_roots);
    MUMO inv_n_mumo, inv_n_w_mumo;
    inv_n_mumo.operand   = inv_n;
    inv_n_w_mumo.operand = inv_n_w;
    switch (n)
    {
        case 16: inv_n_mumo.quotient = 4026531840; break;
        case 4096: inv_n_mumo.quotient = 4293918720; break;
        default: printf("Error with intt inv_n\n"); exit(1);
    }
    switch (mod->value)
    {
        case 1071415297:
            switch (n)
            {
                case 16: inv_n_w_mumo.quotient = 417519972; break;
                case 4096: inv_n_w_mumo.quotient = 3823574310; break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;
        case 1071513601:
            switch (n)
            {
                case 16: inv_n_w_mumo.quotient = 3927827469; break;
                case 4096: inv_n_w_mumo.quotient = 185983515; break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;
        case 1072496641:
            switch (n)
            {
                case 16: inv_n_w_mumo.quotient = 2252725395; break;
                case 4096: inv_n_w_mumo.quotient = 176571868; break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;
        default: printf("Error with intt inv_n_w\n"); exit(1);
    }

    intt_lazy_inpl(parms, (MUMO *)intt_roots, &inv_n_mumo, &inv_n_w_mumo, vec);

    // -- Final adjustments: compute a[j] = a[j] * n^{-1} mod q.
    // -- We incorporated mult by n inverse in the butterfly. Only need to reduce here.
    ZZ q = mod->value;
    for (size_t i = 0; i < n; i++)
    {
        if (vec[i] >= q) vec[i] -= q;
    }
#else
    // -- Note: intt_roots will be ignored if SE_INTT_OTF is defined
    intt_non_lazy_inpl(parms, intt_roots, inv_n, inv_n_w, vec);
#endif
}

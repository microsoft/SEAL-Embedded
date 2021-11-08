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

#ifndef SE_DISABLE_TESTING_CAPABILITY  // INTT is only needed for on-device testing

#if defined(SE_INTT_OTF) || defined(SE_INTT_ONE_SHOT)
ZZ get_intt_root(size_t n, ZZ q);  // defined below
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
        // -- For intt, the roots are not in *exactly* bit-reversed order. This is correct.
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
Performs a "fast" (a.k.a. "lazy") negacyclic in-place inverse NTT using the Harvey butterfly, using
"fast" INTT roots. Only used if "SE_INTT_FAST" is defined. See SEAL_Embedded paper for a more
detailed description. "Lazy"-ness refers to fact that we here we lazily opt to reduce values only at
the very end.

@param[in]     parms            Parameters set by ckks_setup
@param[in]     intt_fast_roots  INTT roots set by intt_roots_initialize
@param[in]     inv_n            Inverse of n mod q, where q = value of the current modulus prime
@param[in]     last_inv_sn
@param[in,out] vec              Input/output polynomial of n ZZ elements
*/
void intt_lazy_inpl(const Parms *parms, const MUMO *intt_fast_roots, const MUMO *inv_n,
                    const MUMO *last_inv_sn, ZZ *vec)
{
    se_assert(parms && ntt_lazy_roots && vec);
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;
    ZZ two_q     = mod->value << 1;

    size_t tt       = 1;      // size of butterflies
    size_t h        = n / 2;  // number of groups
    size_t root_idx = 1;      // We technically don't need to store the 0th one...

    // print_poly_full("vec", vec, n);
    // -- Do everything except the last round
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

    print_zz("tt", tt);
    print_zz("h", h);
    print_zz("root_idx", root_idx);

    // print_poly_full("vec", vec, n);
    // -- Finally, need to multiply by n^{-1}
    // -- Merge this step with the last round we would have performed in the above loop.
    for (size_t j = 0; j < n / 2; j++)
    {
        ZZ u = vec[j];
        ZZ v = vec[j + n / 2];

        ZZ val1 = u + v;
        ZZ val2 = u + two_q - v;

        ZZ tval1 = val1 - (two_q & (ZZ)(-(ZZsign)(val1 >= two_q)));

        vec[j]         = mul_mod_mumo_lazy(tval1, inv_n, mod);
        vec[j + n / 2] = mul_mod_mumo_lazy(val2, last_inv_sn, mod);
    }
    // print_poly_full("vec", vec, n);
}
#else
/**
Performs a negacyclic in-place inverse NTT using the Harvey butterfly. Only used if SE_INTT_FAST is
not defined.

If SE_INTT_REG or SE_INTT_ONE_SHOT is defined, will use regular INTT computation.
Else, (SE_INTT_OTF is defined), will use truly "on-the-fly" INTT computation. In this last case,
'intt_roots' may be null (and will be ignored).

@param[in]     parms        Parameters set by ckks_setup
@param[in]     intt_roots   Roots set by intt_roots_initialize. Ignored if SE_INTT_OTF is defined.
@param[in]     inv_n        (n)^-1 mod q_i, where q_i is the value of the current modulus prime
@param[in]     last_inv_sn
@param[in,out] vec          Input/output polynomial of n ZZ elements
*/
void intt_non_lazy_inpl(const Parms *parms, const ZZ *intt_roots, ZZ inv_n, ZZ last_inv_sn, ZZ *vec)
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

    // -- Option 1: Do everything except the last round
    for (size_t i = 0; i < (logn - 1); i++, tt *= 2, h /= 2)  // rounds
    // -- Option 2: Do everything incluing the last round
    // for (size_t i = 0; i < logn; i++, tt *= 2, h /= 2) // rounds
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
            /*
            if (i == logn - 1)
            {
                printf("\tinverse_tt,  last j: %zu\n", j);
                printf("\tinverse_tt,  last h: %zu\n", h);
                print_zz("\tinverse_ntt, last s", s);
            }
            */
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

    // print_zz("\tinverse ntt: tt", tt);
    // print_zz("\tinverse ntt: h", h);
#if defined(SE_INTT_ONE_SHOT) || defined(SE_INTT_REG)
    print_zz("inverse ntt: root_idx", root_idx);
#endif

    // -- Finally, need to multiply by n^{-1}
    // -- Merge this step with the last round we would have performed in the above loop.
    for (size_t i = 0; i < n / 2; i++)
    {
        ZZ u = vec[i];
        ZZ v = vec[i + n / 2];
        ///*
        // -- Use this if we do one less round above (Option 1), and merge with that last round
        vec[i] = mul_mod(add_mod(u, v, mod), inv_n, mod);
        vec[i + n / 2] = mul_mod(sub_mod(u, v, mod), last_inv_sn, mod);
        //*/
        // -- This is just normal mult by n-inverse (Option 2)
        /*
        mul_mod_inpl(&(vec[i]),         inv_n, mod);
        mul_mod_inpl(&(vec[i + n / 2]), inv_n, mod);
        */
    }
}
#endif

void intt_inpl(const Parms *parms, const ZZ *intt_roots, ZZ *vec)
{
    se_assert(parms && parms->curr_modulus);
    size_t n     = parms->coeff_count;
    Modulus *mod = parms->curr_modulus;

    ZZ inv_n, last_inv_sn;

    /**
        For custom primes:
        Add cases for custom primes at the indicated locations.
        Note that this is only required if:
            1) paramaters (specifically, primes) desired are of custom (i.e., non-default) type
            2) on-device testing is desired

        If any of the above conditions are false, no modifications to this function are needed.
        Note: wolframalpha is a good tool to use to calculate constants.
    */

    // -- Note: inv_n       = n^(-1) mod qi
    //          last_inv_sn = (s*n)^(-1) mod qi
    //    where s = last root in the loop (see intt code)
    // -- Example calculations for n = 4096:
    //     1072496641: last_inv_s  = 420478808  ---> last_ii_s = 420478808^(-1) mod 1072496641 =
    //     652017833
    //                 last_inv_sn = (last_inv_s * inv_n     ) mod 1072496641
    //                             = (420478808  * 1072234801) mod 1072496641 = 44091776
    //                 check:      = (last_ii_s  * n   )^(-1)  mod 1072496641
    //                             = (652017833  * 4096)^(-1)  mod 1072496641 = 44091776 --> Correct
    //     1071513601: last_inv_s  = 393998159  ---> last_ii_s = 393998159^(-1) mod 1071513601 =
    //     677515442
    //                 last_inv_sn = (last_inv_s * inv_n     ) mod 1071513601
    //                             = (393998159  * 1071252001) mod 1071513601 = 46399391
    //                 check:      = (last_ii_s  * n   )^(-1)  mod 1071513601
    //                             = (677515442  * 4096)^(-1)  mod 1071513601 = 46399391 --> Correct
    //     1071415297: last_inv_s  = 476369346  ---> last_ii_s = 476369346^(-1) mod 1071415297 =
    //     595045951
    //                 last_inv_sn = (last_inv_s * inv_n     ) mod 1071415297
    //                             = (476369346  * 1071153721) mod 1071415297 = 953822398
    //                 check:      = (last_ii_s  * n   )^(-1)  mod 1071415297
    //                             = (595045951  * 4096)^(-1)  mod 1071415297 = 953822398 -->
    //                             Correct
    //     These values are also output by the SEAL-Embedded adapter

    switch (n)
    {
        case 1024:
            // -- Add cases for custom primes here
            se_assert(mod->value == 134012929);
            inv_n       = 133882057;
            last_inv_sn = 133898800;
            break;
        case 2048:
            // -- Add cases for custom primes here
            se_assert(mod->value == 134012929);
            inv_n       = 133947493;
            last_inv_sn = 66949400;
            break;
        case 4096:
            switch (mod->value)
            {
                // -- Add cases for custom primes here
                // -- 27 bit
                case 134012929:
                    inv_n       = 133980211;
                    last_inv_sn = 33474700;
                    break;
                case 134111233:
                    inv_n       = 134078491;
                    last_inv_sn = 87415839;
                    break;
                case 134176769:
                    inv_n       = 134144011;
                    last_inv_sn = 23517844;
                    break;
                // -- 30 bit
                case 1053818881:
                    inv_n       = 1053561601;
                    last_inv_sn = 543463427;
                    break;
                case 1054015489:
                    inv_n       = 1053758161;
                    last_inv_sn = 67611149;
                    break;
                case 1054212097:
                    inv_n       = 1053954721;
                    last_inv_sn = 1050429792;
                    break;
                default: printf("Error with intt inv_n or last_inv_sn\n"); exit(1);
            }
            break;
        case 8192:
            switch (mod->value)
            {
                // -- Add cases for custom primes here
                case 1053818881:
                    inv_n       = 1053690241;
                    last_inv_sn = 255177727;
                    break;
                case 1054015489:
                    inv_n       = 1053886825;
                    last_inv_sn = 493202170;
                    break;
                case 1054212097:
                    inv_n       = 1054083409;
                    last_inv_sn = 528997201;
                    break;
                case 1055260673:
                    inv_n       = 1055131857;
                    last_inv_sn = 794790938;
                    break;
                case 1056178177:
                    inv_n       = 1056049249;
                    last_inv_sn = 417300148;
                    break;
                case 1056440321:
                    inv_n       = 1056311361;
                    last_inv_sn = 565858923;
                    break;
                default: printf("Error with intt inv_n or last_inv_sn\n"); exit(1);
            }
            break;
        case 16384:
            switch (mod->value)
            {
                // -- Add cases for custom primes here
                case 1053818881:
                    inv_n       = 1053754561;
                    last_inv_sn = 399320577;
                    break;
                case 1054015489:
                    inv_n       = 1053951157;
                    last_inv_sn = 246601085;
                    break;
                case 1054212097:
                    inv_n       = 1054147753;
                    last_inv_sn = 791604649;
                    break;
                case 1055260673:
                    inv_n       = 1055196265;
                    last_inv_sn = 657865204;
                    break;
                case 1056178177:
                    inv_n       = 1056113713;
                    last_inv_sn = 208650074;
                    break;
                case 1056440321:
                    inv_n       = 1056375841;
                    last_inv_sn = 811149622;
                    break;
                case 1058209793:
                    inv_n       = 1058145205;
                    last_inv_sn = 471841522;
                    break;
                case 1060175873:
                    inv_n       = 1060111165;
                    last_inv_sn = 791157060;
                    break;
                case 1060700161:
                    inv_n       = 1060635421;
                    last_inv_sn = 122051856;
                    break;
                case 1060765697:
                    inv_n       = 1060700953;
                    last_inv_sn = 79059697;
                    break;
                case 1061093377:
                    inv_n       = 1061028613;
                    last_inv_sn = 82969774;
                    break;
                case 1062469633:
                    inv_n       = 1062404785;
                    last_inv_sn = 513630557;
                    break;
                case 1062535169:
                    inv_n       = 1062470317;
                    last_inv_sn = 693696473;
                    break;
                default: printf("Error with intt inv_n or last_inv_sn\n"); exit(1);
            }
            break;
        default: printf("Error with intt inv_n\n"); exit(1);
    }

#ifdef SE_INTT_FAST
    se_assert(intt_roots);
    MUMO inv_n_mumo, last_inv_sn_mumo;
    inv_n_mumo.operand       = inv_n;
    last_inv_sn_mumo.operand = last_inv_sn;
    switch (n)
    {
        case 1024: inv_n_mumo.quotient = 4290772992; break;
        case 2048: inv_n_mumo.quotient = 4292870144; break;
        case 4096: inv_n_mumo.quotient = 4293918720; break;
        case 8192: inv_n_mumo.quotient = 4294443008; break;
        case 16384: inv_n_mumo.quotient = 4294705152; break;
        default: printf("Error with intt inv_n\n"); exit(1);
    }
    switch (n)
    {
        case 1024:
            // -- Add cases for custom primes here
            se_assert(mod->value == 134012929);
            last_inv_sn_mumo.quotient = 4291309586;
            break;
        case 2048:
            // -- Add cases for custom primes here
            se_assert(mod->value == 134012929);
            last_inv_sn_mumo.quotient = 2145654793;
            break;
        case 4096: {
            switch (mod->value)
            {
                // -- Add cases for custom primes here
                // -- 27 bit
                case 134012929: last_inv_sn_mumo.quotient = 1072827396; break;
                case 134111233: last_inv_sn_mumo.quotient = 2799528132; break;
                case 134176769: last_inv_sn_mumo.quotient = 752800738; break;
                // -- 30 bit
                case 1053818881: last_inv_sn_mumo.quotient = 2214951437; break;
                case 1054015489: last_inv_sn_mumo.quotient = 275506078; break;
                case 1054212097: last_inv_sn_mumo.quotient = 4279557800; break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;
        }
        case 8192:
            switch (mod->value)
            {
                // -- Add cases for custom primes here
                case 1053818881: last_inv_sn_mumo.quotient = 1040007929; break;
                case 1054015489: last_inv_sn_mumo.quotient = 2009730608; break;
                case 1054212097: last_inv_sn_mumo.quotient = 2155188395; break;
                case 1055260673: last_inv_sn_mumo.quotient = 3234841563; break;
                case 1056178177: last_inv_sn_mumo.quotient = 1696958455; break;
                case 1056440321: last_inv_sn_mumo.quotient = 2300504363; break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;
        case 16384:
            switch (mod->value)
            {
                // -- Add cases for custom primes here
                case 1053818881: last_inv_sn_mumo.quotient = 1627479683; break;
                case 1054015489: last_inv_sn_mumo.quotient = 1004865304; break;
                case 1054212097: last_inv_sn_mumo.quotient = 3225077845; break;
                case 1055260673: last_inv_sn_mumo.quotient = 2677546514; break;
                case 1056178177: last_inv_sn_mumo.quotient = 848479227; break;
                case 1056440321: last_inv_sn_mumo.quotient = 3297735829; break;
                case 1058209793: last_inv_sn_mumo.quotient = 1915068183; break;
                case 1060175873: last_inv_sn_mumo.quotient = 3205122645; break;
                case 1060700161: last_inv_sn_mumo.quotient = 494210097; break;
                case 1060765697: last_inv_sn_mumo.quotient = 320107271; break;
                case 1061093377: last_inv_sn_mumo.quotient = 335835161; break;
                case 1062469633: last_inv_sn_mumo.quotient = 2076319525; break;
                case 1062535169: last_inv_sn_mumo.quotient = 2804051810; break;
                default: printf("Error with intt inv_n\n"); exit(1);
            }
            break;
        default: printf("Error with intt inv_n\n"); exit(1);
    }

    intt_lazy_inpl(parms, (MUMO *)intt_roots, &inv_n_mumo, &last_inv_sn_mumo, vec);

    // -- Final adjustments: compute a[j] = a[j] * n^{-1} mod q.
    // -- We incorporated mult by n inverse in the butterfly. Only need to reduce here.
    ZZ q = mod->value;
    for (size_t i = 0; i < n; i++)
    {
        if (vec[i] >= q) vec[i] -= q;
    }
#else
    // -- Note: intt_roots will be ignored if SE_INTT_OTF is defined
    intt_non_lazy_inpl(parms, intt_roots, inv_n, last_inv_sn, vec);
#endif
}

#if defined(SE_INTT_OTF) || defined(SE_INTT_ONE_SHOT)
/**
Helper function to return root for certain modulus prime values if SE_INTT_OTF or SE_INTT_ONE_SHOT
is used. Implemented as a table lookup.

@param[in] n  Transform size (i.e. polynomial ring degree)
@param[in] q  Modulus value
*/
ZZ get_intt_root(size_t n, ZZ q)
{
    /**
        For custom primes:
        Add cases for custom primes at the indicated locations.
        Note that this is only required if:
            1) paramaters (specifically, primes) desired are of custom (i.e., non-default) type
            2) INTT type is of compute ("on-the-fly" or "one-shot") type
            3) on-device testing is desired

        If any of the above conditions are false, no modifications to this function are needed.
        Note: wolframalpha is a good tool to use to calculate constants.
    */

    ZZ inv_root;  // i.e. w^(-1) = first power of INTT root
    switch (n)
    {
        case 1024:
            // -- Add cases for custom primes here
            se_assert(q == 134012929);
            inv_root = 131483387;
            break;
        case 2048:
            // -- Add cases for custom primes here
            se_assert(q == 134012929);
            inv_root = 83050288;
            break;
        case 4096:
            switch (q)
            {
                // -- Add cases for custom primes here
                // -- 27 bit
                case 134012929: inv_root = 92230317; break;
                case 134111233: inv_root = 106809024; break;
                case 134176769: inv_root = 113035413; break;
                // -- 30 bit
                case 1053818881: inv_root = 18959119; break;
                case 1054015489: inv_root = 450508648; break;
                case 1054212097: inv_root = 82547477; break;
                default: {
                    printf("Error! Need first power of root for intt, n = 4K\n");
                    print_zz("Modulus value", q);
                    exit(1);
                }
            }
            break;
        case 8192:
            switch (q)
            {
                // -- Add cases for custom primes here
                case 1053818881: inv_root = 303911105; break;
                case 1054015489: inv_root = 552874754; break;
                case 1054212097: inv_root = 85757512; break;
                case 1055260673: inv_root = 566657253; break;
                case 1056178177: inv_root = 18375283; break;
                case 1056440321: inv_root = 939847932; break;
                default: {
                    printf("Error! Need first power of root for intt, n = 8K\n");
                    print_zz("Modulus value", q);
                    exit(1);
                }
            }
            break;
        case 16384:
            switch (q)
            {
                // -- Add cases for custom primes here
                case 1053818881: inv_root = 232664460; break;
                case 1054015489: inv_root = 752571217; break;
                case 1054212097: inv_root = 797764264; break;
                case 1055260673: inv_root = 572000669; break;
                case 1056178177: inv_root = 174597629; break;
                case 1056440321: inv_root = 252935303; break;
                case 1058209793: inv_root = 440137408; break;
                case 1060175873: inv_root = 309560567; break;
                case 1060700161: inv_root = 351709685; break;
                case 1060765697: inv_root = 759856646; break;
                case 1061093377: inv_root = 729599158; break;
                case 1062469633: inv_root = 677791800; break;
                case 1062535169: inv_root = 943827998; break;
                default: {
                    printf("Error! Need first power of root for intt, n = 16K\n");
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

#endif  // if testing is not disabled

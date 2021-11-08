// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file ckks_tests_common.c
*/

#include "ckks_tests_common.h"

#include <complex.h>
#include <stdbool.h>
#include <stdio.h>

#include "ckks_common.h"
#include "defines.h"
#include "fft.h"
#include "fileops.h"
#include "intt.h"
#include "ntt.h"
#include "polymodarith.h"
#include "polymodmult.h"
#include "test_common.h"
#include "util_print.h"

void set_encode_encrypt_test(size_t testnum, size_t vlen, flpt *v)
{
    se_assert(v);
    clear_flpt(v, vlen);
    if (testnum > 8) testnum = 8;  // currently only have 9 tests
    switch (testnum)
    {
        case 0: v[0] = 1; break;
        case 1: v[0] = 2; break;
        case 2: set_flpt(v, vlen, 1); break;
        case 3: set_flpt(v, vlen, 2); break;
        case 4: set_flpt(v, vlen, (flpt)1.1); break;
        case 5: set_flpt(v, vlen, (flpt)-2.1); break;
        case 6:
            set_flpt(v, vlen, (flpt)1);
            for (size_t i = 0; i < vlen; i += 2)
            {
                v[i] = 0;
                if ((i + 1) < vlen) v[i + 1] = 1;
            }
            break;
        case 7: gen_flpt_eighth_poly(v, -100, vlen); break;
        case 8: gen_flpt_quarter_poly(v, -10, vlen); break;

        // -- This test works when n = 1024 with -1000
        // -- But not for most other cases
        // case 9: gen_flpt_half_poly(v, 512, vlen); break;

        // -- This test does not work with 1000
        // case 10:	gen_flpt_poly(v, 10000000, vlen); break;
        default: break;
    }
}

void ckks_decode(const ZZ *pt, size_t values_len, uint16_t *index_map, const Parms *parms,
                 double complex *temp, flpt *values_decoded)
{
    se_assert(pt && parms && parms->curr_modulus && temp && values_decoded);
    size_t n     = parms->coeff_count;
    size_t logn  = parms->logn;
    ZZ q         = parms->curr_modulus->value;
    double scale = parms->scale;

    double complex *res = temp;
    // printf("scale: %0.5f\n", scale);
    print_poly("pt", pt, n);

    for (size_t i = 0; i < n; i++)
    {
        // -- Get representative element in (-q/2, q/2] from element in [0, q).
        // -- Note: q, val are both unsigned, so dval = (double)(val - q) won't work.
        ZZ val      = pt[i];
        double dval = (val > q / 2) ? -(double)(q - val) : (double)val;

        // -- We scale by 1/scale here
        res[i] = (double complex)_complex(dval / scale, (double)0);
    }
    print_poly_double_complex("res           ", res, n);

    fft_inpl(res, n, logn, NULL);

    print_poly_double_complex("res           ", res, n);

#ifdef SE_INDEX_MAP_OTF
    // -- Here we are making room for calculating the index map. Since we are just
    //    testing, we can just load in the values all at once, assuming we have space.
    //    A more accurate test would be to calculate the index map truly on-the-fly.
    se_assert(!index_map);

    // -- We no longer need the second half of res
    double *res_double = (double *)res;
    for (size_t i = 0; i < n; i++) { res_double[i] = se_creal(res[i]); }
    print_poly_double("res double    ", res_double, n);

    uint16_t *index_map_ = (uint16_t *)(&(res[n / 2]));
    ckks_calc_index_map(parms, index_map_);

    for (size_t i = 0; i < values_len; i++)
    { values_decoded[i] = (flpt)(res_double[index_map_[i]]); }
    // print_poly_flpt("decoded", values_decoded, n);
#else
    se_assert(index_map);
#ifdef SE_INDEX_MAP_LOAD
    // -- Load or setup here, doesn't matter, since we are just testing...
    load_index_map(parms, index_map);
#elif defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
    if (parms->is_asymmetric) load_index_map(parms, index_map);
#endif
    for (size_t i = 0; i < values_len; i++)
    { values_decoded[i] = (flpt)se_creal(res[index_map[i]]); }
#endif
}

void check_decode_inpl(ZZ *pt, const flpt *values, size_t values_len, uint16_t *index_map,
                       const Parms *parms, ZZ *temp)
{
    size_t n = parms->coeff_count;
    se_assert(values_len <= n / 2 && values_len > 0);

    // -- We don't need enough space for the ifft_roots in this version since we don't
    //    care how long it takes
    // -- Check: original values should be close to:
    //      Decode(Decrypt(Encrypt(Encode(original values))))
    const char *n1 = "values        ";
    const char *n2 = "values_decoded";
    ckks_decode_inpl(pt, values_len, index_map, parms, (double complex *)temp);
    print_poly_flpt(n2, (flpt *)pt, n);
    bool err = compare_poly_flpt(n1, values, n2, (flpt *)pt, values_len, (flpt)0.1);
    se_assert(!err);
}

void ckks_decrypt(const ZZ *c0, const ZZ *c1, const ZZ *s, bool small_s, const Parms *parms, ZZ *pt)
{
    // -- c0 = [-a*s + e + pt]_Rq ; c1 = a
    //    Encryption is correct if [c0 + c1*s]_Rq = ([-a*s + e + pt]_Rq) + [(a)*s]_Rq
    //                                            = [a*s - a*s + pt + e]_Rq
    //                                            = [pt + e]_Rq
    PolySizeType n = parms->coeff_count;
    Modulus *mod   = parms->curr_modulus;

    se_assert(!small_s);

    // -- Step 1: pt = [c1 * s]_Rq
    poly_mult_mod_ntt_form(c1, s, n, mod, pt);
    // print_poly("c1*s          ", pt, n);

    // -- Step 2: pt = [c0 + c1*s]_Rq
    poly_add_mod_inpl(pt, c0, n, mod);
}

void ckks_decrypt_inpl(ZZ *c0, ZZ *c1, const ZZ *s, bool small_s, const Parms *parms)
{
    // -- c0 = [-a*s + e + pt]_Rq ; c1 = a
    //    Encryption is correct if c0 + c1*s == pt + errors  (all operations mod q)
    //    because c0 + c1*s = ([-a*s + e + pt]_Rq) + [(a)*s]_Rq
    //                      = [a*s - a*s + pt + e]_Rq
    //                      = [pt +e]_Rq                    (in the symmetric case)
    PolySizeType n = parms->coeff_count;
    Modulus *mod   = parms->curr_modulus;

    // -- Step 1: pt = [c1 * s]_Rq
    se_assert(!small_s);
    poly_mult_mod_ntt_form_inpl(c1, s, n, mod);

    // -- Step 2: pt = [c0 + c1*s]_Rq
    poly_add_mod_inpl(c0, c1, n, mod);
}

void check_decode_decrypt_inpl(ZZ *c0, ZZ *c1, const flpt *values, size_t values_len, const ZZ *s,
                               bool small_s, const ZZ *pte_calc, uint16_t *index_map,
                               const Parms *parms, ZZ *temp)
{
    se_assert(c0 && c1 && values && s && pte_calc && parms && temp);
    se_assert(!small_s);
    print_poly_flpt("values", values, values_len);

    size_t n = parms->coeff_count;
    se_assert(values_len <= n / 2 && values_len > 0);

    print_poly("c0", c0, n);
    print_poly("c1", c1, n);
    print_poly("s ", s, n);

    // -- Calculate: c0 := [c0 + c1*s]
    ckks_decrypt_inpl(c0, c1, s, small_s, parms);

    // -- Check that decrypt works by comparing to pterr
    const char *s1 = "pte calculated";
    const char *s2 = "pte decrypted ";
    print_poly(s1, pte_calc, n);
    print_poly(s2, c0, n);
    /*
    // -- Debugging
    intt_roots_initialize(parms, temp);
    //print_poly_full("     c0    ", c0, n);
    intt(parms, temp, c0);
    intt(parms, temp, pterr);
    const char *s1 = "pte calculated"; print_poly(s1, pte, n);
    const char *s2 = "pte decrypted "; print_poly(s2, c0, n);
    */

    compare_poly(s1, pte_calc, s2, c0, n);

    // -- Make sure we didn't accidentally check an all-zeros vector
    if (n > 16) se_assert(!all_zeros(c0, n));

    // -- Then, test decode if requested
    if (values)
    {
        print_poly("c0            ", c0, n);
        intt_roots_initialize(parms, temp);
        se_assert(temp);
        intt_inpl(parms, temp, c0);
        print_poly("pt = intt(c0) ", c0, n);

        // -- We don't need enough space for the ifft_roots in this version since we don't
        //    care how long it takes
        // -- Check: original values should be close to:
        //      Decode(Decrypt(Encrypt(Encode(original values)))
        const char *n1 = "values        ";
        const char *n2 = "values_decoded";
        ckks_decode_inpl(c0, values_len, index_map, parms, (double complex *)temp);
        print_poly_flpt(n2, (flpt *)c0, n);
        bool err = compare_poly_flpt(n1, values, n2, (flpt *)c0, values_len, (flpt)0.1);
        se_assert(!err);
    }
}

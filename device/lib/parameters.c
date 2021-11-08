// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file parameters.c
*/

#include "parameters.h"

#include <math.h>  // log2

#include "defines.h"
#include "util_print.h"

/**
Helper function to check if a value is a power of 2.

@param[in] val  Value to verify if is a power of 2
@returns        True if val is a power of 2, false otherwise
*/
static bool is_power_of_2(size_t val)
{
    return val && (!(val & (val - 1)));
}

void delete_parameters(Parms *parms)
{
#ifdef SE_USE_MALLOC
    se_assert(parms);
    if (parms->moduli)
    {
        free(parms->moduli);
        parms->moduli = 0;
    }
    parms = NULL;
#else
    SE_UNUSED(parms);
#endif
}

void reset_primes(Parms *parms)
{
    se_assert(parms);
#ifdef SE_REVERSE_CT_GEN_ENABLED
    parms->curr_param_direction = 0;
    parms->skip_ntt_load        = 0;
#endif
    parms->curr_modulus_idx = 0;
    parms->curr_modulus     = &(parms->moduli[0]);
}

bool next_modulus(Parms *parms)
{
    se_assert(parms);
    bool ret_val = 1;  // success
#ifdef SE_REVERSE_CT_GEN_ENABLED
    if (parms->curr_param_direction == 0)  // forwards
    {
        if ((parms->curr_modulus_idx + 1) >= parms->nprimes)
        {
            parms->curr_param_direction = 1;
            parms->skip_ntt_load        = 1;
            return 0;
        }
        parms->curr_modulus_idx++;
    }
    else
    {
        if (parms->curr_modulus_idx == 0)
        {
            parms->curr_param_direction = 0;
            parms->skip_ntt_load        = 1;
            return 0;
        }
        parms->curr_modulus_idx--;
    }
    parms->skip_ntt_load = 0;
#else
    if ((parms->curr_modulus_idx + 1) >= parms->nprimes)
    {
        parms->curr_modulus_idx = 0;
        ret_val                 = 0;
    }
    else
        parms->curr_modulus_idx++;
#endif
    parms->curr_modulus = &(parms->moduli[parms->curr_modulus_idx]);
    return ret_val;
}

/**
Helper function to set the Parameters instance.

@param[in]  degree   Polynomial ring degree
@param[in]  nprimes  Number of prime moduli
@param[out] parms    Parameters instance to set
*/
static void set_params_base(size_t degree, size_t nprimes, Parms *parms)
{
    se_assert(degree >= 1024 && degree <= 16384);
    se_assert(is_power_of_2(degree));
    se_assert(parms);
    se_assert(nprimes >= 1);

    parms->coeff_count = degree;
    parms->logn        = (size_t)log2(degree);
    parms->nprimes     = nprimes;
#ifdef SE_USE_MALLOC
    se_assert(parms && parms->nprimes);
    parms->moduli = calloc(parms->nprimes, sizeof(Modulus));
    se_assert(parms->moduli);
#endif

    // -- Set curr_modulus to first prime
    parms->curr_modulus_idx = 0;
    parms->curr_modulus     = &(parms->moduli[0]);
    se_assert(parms->curr_modulus);
#ifdef SE_REVERSE_CT_GEN_ENABLED
    parms->curr_param_direction = 0;  // 0 = forward, 1 = reverse
    parms->skip_ntt_load        = 0;
#endif
}

/**
Helper function to set 27-bit primes

@param[in, out] parms  Parameters object to set
*/
void set_parms_ckks_27bit_helper(Parms *parms)
{
    se_assert(parms);
    se_assert(parms->nprimes >= 1);
    se_assert(parms->coeff_count == 1024 || parms->coeff_count == 2048 ||
              parms->coeff_count == 4096);
    switch (parms->nprimes)
    {
        // -- 27-bit primes equal to 1 mod 8192
        case 3: set_modulus(134176769, &(parms->moduli[2]));
        case 2: set_modulus(134111233, &(parms->moduli[1]));
        case 1: set_modulus(134012929, &(parms->moduli[0]));
    }
}

/**
Helper function to set 30-bit primes

@param[in, out] parms  Parameters object to set
*/
void set_parms_ckks_30bit_helper(Parms *parms)
{
    se_assert(parms);
    se_assert(parms->nprimes >= 1);
    switch (parms->nprimes)
    {
        // -- 30-bits primes equal to 1 mod 65536?
        // TODO: are these pragmas ignored when necessary?
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
        case 13: set_modulus(1062535169, &(parms->moduli[12]));
        case 12: set_modulus(1062469633, &(parms->moduli[11]));
        case 11: set_modulus(1061093377, &(parms->moduli[10]));
        case 10: set_modulus(1060765697, &(parms->moduli[9]));
        case 9: set_modulus(1060700161, &(parms->moduli[8]));
        case 8: set_modulus(1060175873, &(parms->moduli[7]));
        case 7: set_modulus(1058209793, &(parms->moduli[6]));
        case 6: set_modulus(1056440321, &(parms->moduli[5]));
        case 5: set_modulus(1056178177, &(parms->moduli[4]));
        case 4: set_modulus(1055260673, &(parms->moduli[3]));
        case 3: set_modulus(1054212097, &(parms->moduli[2]));
        case 2: set_modulus(1054015489, &(parms->moduli[1]));
        case 1: set_modulus(1053818881, &(parms->moduli[0]));
#pragma GCC diagnostic pop
    }
}

void set_parms_ckks(size_t degree, size_t nprimes, Parms *parms)
{
    se_assert(parms);
    se_assert(nprimes >= 1);
    set_params_base(degree, nprimes, parms);

    // -- These precomputed values of q satisfy q = 1 (mod m), for m = 2*n and
    //    n = polynomial degree = a power of 2.
    // -- We know that any prime q that satisfies:
    //         q = 1 (mod m)   (m = 2^k, k = a positive integer)
    //    also satisfies:
    //         q = 1 (mod m')  (m' = 2^(k-i), i = a positive integer < k)
    //    and we can use the same modulus q for all powers of 2 < m. Note that the
    //    reverse is not true. That is, a prime q that satisfies q = 1 (mod m')
    //    does not necessarily satisfy q = 1 (mod m).
    switch (degree)
    {
        // -- Add cases for custom primes here or use custom parms API
        case 1024:
            se_assert(parms->nprimes == 1);
            set_parms_ckks_27bit_helper(parms);
            parms->scale = pow(2, 20);
            break;
        case 2048:
            se_assert(parms->nprimes == 1);
            set_parms_ckks_27bit_helper(parms);
            parms->scale = pow(2, 25);
            break;
#ifdef SE_DEFAULT_4K_27BIT
        case 4096:
            se_assert(parms->nprimes <= 3);
            set_parms_ckks_27bit_helper(parms);
            parms->scale = pow(2, 20);
            break;
#else
        case 4096:
            se_assert(parms->nprimes <= 3);
            set_parms_ckks_30bit_helper(parms);
            parms->scale = pow(2, 25);
            break;
#endif
        case 8192:
            se_assert(parms->nprimes <= 6);
            set_parms_ckks_30bit_helper(parms);
            parms->scale = pow(2, 25);
            break;
        case 16384:
            se_assert(parms->nprimes <= 13);
            set_parms_ckks_30bit_helper(parms);
            parms->scale = pow(2, 25);
            break;
    }
    // parms.scale = pow(2, 40);
    // parms.scale = n * n;
}

void set_custom_parms_ckks(size_t degree, double scale, size_t nprimes, const ZZ *modulus_vals, const ZZ *ratios,
                           Parms *parms)
{
    if (!modulus_vals || !ratios)
    {
        set_parms_ckks(degree, nprimes, parms);
        parms->scale = scale;
        return;
    }

    set_params_base(degree, nprimes, parms);
    for (size_t i = 0; i < nprimes; i++)
    {
        se_assert(modulus_vals[i]);  // Should never be 0
        set_modulus_custom(modulus_vals[i], ratios[i], ratios[i + 1], &(parms->moduli[i]));
    }
    parms->scale = scale;
}

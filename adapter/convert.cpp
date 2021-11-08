// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file convert.cpp
*/

#include "convert.h"

#include <cassert>

#include "generate.h"
#include "seal/seal.h"
#include "utils.h"

using namespace std;
using namespace seal;
using namespace seal::util;

void sk_to_ntt_form(const SEALContext &context, SecretKey &sk)
{
    bool is_ntt = sk.data().is_ntt_form();
    if (is_ntt == true) return;

    // -- This is kind of clunky but we need to do it this way
    auto &parms_id = (sk.parms_id() == parms_id_zero) ? context.key_parms_id() : sk.parms_id();

    auto sk_context_data_ptr = context.get_context_data(parms_id);
    auto &parms              = sk_context_data_ptr->parms();
    auto &coeff_modulus      = parms.coeff_modulus();
    size_t nprimes           = coeff_modulus.size();
    size_t n                 = parms.poly_modulus_degree();
    auto ntt_tables_ptr      = sk_context_data_ptr->small_ntt_tables();

    RNSIter sk_iter(sk.data().data(), n);
    ntt_negacyclic_harvey(sk_iter, nprimes, ntt_tables_ptr);

    sk.data().parms_id() = context.key_parms_id();
    assert(sk.data().is_ntt_form() == true);
}

void sk_to_non_ntt_form(const SEALContext &context, SecretKey &sk)
{
    bool is_ntt = sk.data().is_ntt_form();
    if (is_ntt == false) return;

    // -- This is kind of clunky but we need to do it this way
    auto &parms_id = (sk.parms_id() == parms_id_zero) ? context.key_parms_id() : sk.parms_id();

    auto sk_context_data_ptr = context.get_context_data(parms_id);
    auto &parms              = sk_context_data_ptr->parms();
    auto &coeff_modulus      = parms.coeff_modulus();
    size_t nprimes           = coeff_modulus.size();
    size_t n                 = parms.poly_modulus_degree();
    auto ntt_tables_ptr      = sk_context_data_ptr->small_ntt_tables();

    assert(sk_context_data_ptr == context.key_context_data());

    RNSIter sk_itr(sk.data().data(), n);
    inverse_ntt_negacyclic_harvey(sk_itr, nprimes, ntt_tables_ptr);

    sk.data().parms_id() = parms_id_zero;
    assert(sk.data().is_ntt_form() == false);
}

void pk_to_ntt_form(const SEALContext &context, PublicKeyWrapper &pk_wr)
{
    if (pk_wr.is_ntt == true) return;

    auto pk_context_data_ptr = context.get_context_data(pk_wr.pk->parms_id());
    auto &parms              = pk_context_data_ptr->parms();
    auto &coeff_modulus      = parms.coeff_modulus();
    size_t nprimes           = coeff_modulus.size();
    size_t n                 = parms.poly_modulus_degree();
    auto ntt_tables_ptr      = pk_context_data_ptr->small_ntt_tables();

    assert(pk_context_data_ptr == context.key_context_data());
    assert(pk_wr.pk->data().size() == 2);

    PolyIter pk_iter(pk_wr.pk->data().data(), n, nprimes);
    ntt_negacyclic_harvey(pk_iter, pk_wr.pk->data().size(), ntt_tables_ptr);

    pk_wr.is_ntt = true;
    // assert(pk_wr.pk->data().is_ntt_form() == true);
    // -- Note: We cannot use pk_wr.pk->data().is_ntt_form() directly because this value
    //    is not directly changeable for ciphertexts (and therefore public keys).
    //    It is only changeable by calling evaluator.transform_to_ntt(), but this
    //    function throws an exception when you try to give it a public key.
}

void pk_to_non_ntt_form(const SEALContext &context, PublicKeyWrapper &pk_wr)
{
    if (pk_wr.is_ntt == false) return;

    auto pk_context_data_ptr = context.get_context_data(pk_wr.pk->parms_id());
    auto &parms              = pk_context_data_ptr->parms();
    auto &coeff_modulus      = parms.coeff_modulus();
    size_t nprimes           = coeff_modulus.size();
    size_t n                 = parms.poly_modulus_degree();
    auto ntt_tables_ptr      = pk_context_data_ptr->small_ntt_tables();

    assert(pk_context_data_ptr == context.key_context_data());
    assert(pk_wr.pk->data().size() == 2);

    PolyIter pk_iter(pk_wr.pk->data().data(), n, nprimes);
    inverse_ntt_negacyclic_harvey(pk_iter, pk_wr.pk->data().size(), ntt_tables_ptr);

    pk_wr.is_ntt = false;
    // assert(pk_wr.pk->data().is_ntt_form() == false);
    // -- Note: We cannot use pk_wr.pk->data().is_ntt_form() directly because this value
    //    is not directly changeable for ciphertexts (and therefore public keys).
    //    It is only changeable by calling evaluator.transform_to_ntt(), but this
    //    function throws an exception when you try to give it a public key.
}

void ct_to_ntt_form(Evaluator &evaluator, Ciphertext &c_in)
{
    if (c_in.is_ntt_form() == true) return;
    evaluator.transform_to_ntt_inplace(c_in);
    assert(c_in.is_ntt_form() == true);
}

void ct_to_non_ntt_form(Evaluator &evaluator, Ciphertext &c_in)
{
    if (c_in.is_ntt_form() == false) return;
    evaluator.transform_from_ntt_inplace(c_in);
    assert(c_in.is_ntt_form() == false);
}

void pt_to_ntt_form(const SEALContext &context, Plaintext &pt)
{
    bool is_ntt = pt.is_ntt_form();
    if (is_ntt == true) return;

    // auto context_data_ptr = context.key_context_data();
    auto context_data_ptr = context.first_context_data();
    auto &parms           = context_data_ptr->parms();
    auto &coeff_modulus   = parms.coeff_modulus();
    size_t nprimes        = coeff_modulus.size();
    size_t n              = parms.poly_modulus_degree();
    auto ntt_tables_ptr   = context_data_ptr->small_ntt_tables();

    RNSIter plaintext(pt.data(), n);
    ntt_negacyclic_harvey(plaintext, nprimes, ntt_tables_ptr);

    pt.parms_id() = context_data_ptr->parms_id();
    assert(pt.is_ntt_form() == true);
}

void pt_to_non_ntt_form(const SEALContext &context, Plaintext &pt)
{
    bool is_ntt = pt.is_ntt_form();
    if (is_ntt == false) return;

    auto context_data_ptr = context.first_context_data();
    auto &parms           = context_data_ptr->parms();
    auto &coeff_modulus   = parms.coeff_modulus();
    size_t nprimes        = coeff_modulus.size();
    size_t n              = parms.poly_modulus_degree();
    auto ntt_tables_ptr   = context_data_ptr->small_ntt_tables();

    RNSIter plaintext(pt.data(), n);
    inverse_ntt_negacyclic_harvey(plaintext, nprimes, ntt_tables_ptr);

    pt.parms_id() = parms_id_zero;
    assert(pt.is_ntt_form() == false);
}

void compare_sk(const SEALContext &context, SecretKey &sk1, SecretKey &sk2, bool incl_sp,
                bool should_match)
{
    assert(sk1.data().is_ntt_form() && sk2.data().is_ntt_form());

    size_t print_size = 8;

    for (int i = 0; i < 2; i++)
    {
        print_sk_compare("sk2", sk2, "sk1", sk1, context, print_size, incl_sp);

        if (should_match)
            cout << "(Above: Values should match)" << endl;
        else
            cout << "(Above: Values should differ)" << endl;

        bool match_result = same_sk(sk2, sk1, context, incl_sp);
        assert(match_result == should_match);

        if (i == 0)
        {
            sk_to_non_ntt_form(context, sk2);
            sk_to_non_ntt_form(context, sk1);
        }
        else
        {
            sk_to_ntt_form(context, sk2);
            sk_to_ntt_form(context, sk1);
        }
    }
}

void compare_pk(const SEALContext &context, PublicKeyWrapper &pk1_wr, PublicKeyWrapper &pk2_wr,
                bool incl_sp, bool should_match)
{
    assert(pk1_wr.is_ntt && pk2_wr.is_ntt);

    size_t print_size       = 8;
    bool compare_both_forms = 1;  // Compare both forms just in case

    for (int i = 0; i < 2; i++)
    {
        print_pk_compare("pk2", pk2_wr, "pk1", pk1_wr, print_size, incl_sp);

        if (should_match)
            cout << "(Above: These should be the same)" << endl;
        else
            cout << "(Above: These should be different)" << endl;

        bool match_result = same_pk(pk1_wr, pk2_wr, incl_sp);
        assert(match_result == should_match);

        if (compare_both_forms)
        {
            if (i == 0)
            {
                pk_to_non_ntt_form(context, pk2_wr);
                pk_to_non_ntt_form(context, pk1_wr);
            }
            else
            {
                pk_to_ntt_form(context, pk2_wr);
                pk_to_ntt_form(context, pk1_wr);
            }
        }
        else
        {
            break;
        }
    }
}

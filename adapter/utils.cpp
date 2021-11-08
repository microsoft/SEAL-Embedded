// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file utils.cpp
*/

#include "utils.h"

#include <cassert>
#include <iostream>
#include <string>

#include "convert.h"
#include "seal/seal.h"

using namespace std;
using namespace seal;
using namespace seal::util;

// ---------------- Setup ------------------

void add_27bit_moduli(size_t nprimes, vector<Modulus> &vec)
{
    if (!nprimes) return;
    vec.clear();

    // -- Note: We create the vector in reverse first
    switch (nprimes)
    {
        // -- These primes are all 1 mod 32768
        case 3: vec.push_back(Modulus(134176769)); [[fallthrough]];
        case 2: vec.push_back(Modulus(134111233)); [[fallthrough]];
        case 1: vec.push_back(Modulus(134012929)); [[fallthrough]];
        // TODO: This is the SEAL default. Why?
        // case 1: vec.push_back(Modulus(132120577)); [[fallthrough]];
        default: break;
    }
    reverse(vec.begin(), vec.end());
}

void add_30bit_moduli(size_t nprimes, vector<Modulus> &vec)
{
    if (!nprimes) return;

    vec.clear();

    // -- Note: We create the vector in reverse first
    switch (nprimes)
    {
        // -- These primes are all 1 mod 32768
        /*
        // -- These are for n = 32K
        case 28: vec.push_back(Modulus(1073479681)); [[fallthrough]];
        case 27: vec.push_back(Modulus(1072496641)); [[fallthrough]];
        case 26: vec.push_back(Modulus(1071513601)); [[fallthrough]];
        case 25: vec.push_back(Modulus(1070727169)); [[fallthrough]];
        case 24: vec.push_back(Modulus(1069219841)); [[fallthrough]];
        case 23: vec.push_back(Modulus(1068564481)); [[fallthrough]];
        case 22: vec.push_back(Modulus(1068433409)); [[fallthrough]];
        case 21: vec.push_back(Modulus(1068236801)); [[fallthrough]];
        case 20: vec.push_back(Modulus(1065811969)); [[fallthrough]];
        case 19: vec.push_back(Modulus(1065484289)); [[fallthrough]];
        case 18: vec.push_back(Modulus(1064697857)); [[fallthrough]];
        case 17: vec.push_back(Modulus(1063452673)); [[fallthrough]];
        case 16: vec.push_back(Modulus(1063321601)); [[fallthrough]];
        case 15: vec.push_back(Modulus(1063059457)); [[fallthrough]];
        case 14: vec.push_back(Modulus(1062862849)); [[fallthrough]];
        */
        case 13: vec.push_back(Modulus(1062535169)); [[fallthrough]];
        case 12: vec.push_back(Modulus(1062469633)); [[fallthrough]];
        case 11: vec.push_back(Modulus(1061093377)); [[fallthrough]];
        case 10: vec.push_back(Modulus(1060765697)); [[fallthrough]];
        case 9: vec.push_back(Modulus(1060700161)); [[fallthrough]];
        case 8: vec.push_back(Modulus(1060175873)); [[fallthrough]];
        case 7: vec.push_back(Modulus(1058209793)); [[fallthrough]];
        case 6: vec.push_back(Modulus(1056440321)); [[fallthrough]];
        case 5: vec.push_back(Modulus(1056178177)); [[fallthrough]];
        case 4: vec.push_back(Modulus(1055260673)); [[fallthrough]];
        case 3: vec.push_back(Modulus(1054212097)); [[fallthrough]];
        case 2: vec.push_back(Modulus(1054015489)); [[fallthrough]];
        case 1: vec.push_back(Modulus(1053818881)); [[fallthrough]];
        default: break;
    }
    reverse(vec.begin(), vec.end());
}

seal::SEALContext setup_seale_custom(size_t degree, const vector<Modulus> &moduli,
                                     EncryptionParameters &parms)
{
    for (size_t i = 0; i < moduli.size() - 1; i++)
    {
        // -- Required for compatibility with SEAL-embedded.
        assert(moduli[i].bit_count() <= 30);
    }
    parms.set_poly_modulus_degree(degree);
    parms.set_coeff_modulus(moduli);
    seal::SEALContext context(parms);
    print_parameters(context);
    print_all_moduli(parms);
    cout << endl;
    return context;
}

seal::SEALContext setup_seale_prime_default(size_t degree, EncryptionParameters &parms)
{
    vector<Modulus> moduli;
    switch (degree)
    {
        case 1024: add_27bit_moduli(1, moduli); break;
        case 2048:
            add_27bit_moduli(1, moduli);
            moduli.push_back((CoeffModulus::Create(degree, {27}))[0]);
            break;
#ifdef SEALE_DEFAULT_4K_27BIT
        case 4096:
            add_27bit_moduli(3, moduli);
            moduli.push_back((CoeffModulus::Create(degree, {28}))[0]);
            break;
#else
        case 4096:
            add_30bit_moduli(3, moduli);
            moduli.push_back((CoeffModulus::Create(degree, {19}))[0]);
            break;
#endif
        case 8192:
            add_30bit_moduli(6, moduli);
            moduli.push_back((CoeffModulus::Create(degree, {38}))[0]);
            break;
        case 16384:
            add_30bit_moduli(13, moduli);
            moduli.push_back((CoeffModulus::Create(degree, {48}))[0]);
            break;
        default:
            throw runtime_error(
                "Please use a different setup function "
                "(setup_seal_api or setup_seale_custom)");
            break;
    }
    return setup_seale_custom(degree, moduli, parms);
}

seal::SEALContext setup_seal_api(size_t degree, const vector<int> &bit_lengths,
                                 EncryptionParameters &parms)
{
    return setup_seale_custom(degree, CoeffModulus::Create(degree, bit_lengths), parms);
}

// ---------------- Size functions ------------------

size_t get_sk_num_bytes(const SecretKey &sk, const seal::SEALContext &context, bool incl_sp)
{
    auto &sk_parms      = context.key_context_data()->parms();
    auto &coeff_modulus = sk_parms.coeff_modulus();
    size_t n            = sk_parms.poly_modulus_degree();
    size_t nprimes      = coeff_modulus.size();
    size_t type_size    = sizeof(sk.data().data()[0]);
    assert(incl_sp || nprimes > 1);

    size_t nprimes_count = incl_sp ? nprimes : nprimes - 1;
    return n * nprimes_count * type_size;
}

size_t get_pk_num_bytes(const PublicKey &pk, bool incl_sp)
{
    size_t n              = pk.data().poly_modulus_degree();
    size_t nprimes        = pk.data().coeff_modulus_size();
    size_t type_size      = sizeof(pk.data().data()[0]);
    size_t num_components = pk.data().size();
    assert(num_components == 2);
    assert(incl_sp || nprimes > 1);

    size_t nprimes_count = incl_sp ? nprimes : nprimes - 1;
    return n * nprimes_count * type_size * 2;
}

// ---------------- Data pointers ------------------

uint64_t *get_pt_arr_ptr(Plaintext &pt)
{
    return reinterpret_cast<uint64_t *>(pt.data());
}

uint64_t *get_ct_arr_ptr(Ciphertext &ct, bool second_element)
{
    if (second_element)
    {
        size_t n       = ct.poly_modulus_degree();
        size_t nprimes = ct.coeff_modulus_size();
        return reinterpret_cast<uint64_t *>(&(ct[n * nprimes]));
    }
    else
    {
        return reinterpret_cast<uint64_t *>(ct.data());
    }
}

uint64_t *get_sk_arr_ptr(SecretKey &sk)
{
    return reinterpret_cast<uint64_t *>(sk.data().data());
}

uint64_t *get_pk_arr_ptr(PublicKey &pk, bool second_element)
{
    if (second_element)
    {
        size_t n       = pk.data().poly_modulus_degree();
        size_t nprimes = pk.data().coeff_modulus_size();
        return reinterpret_cast<uint64_t *>(&(pk.data()[n * nprimes]));
    }
    else
    {
        return reinterpret_cast<uint64_t *>(pk.data().data());
    }
}

uint64_t *get_pk_arr_ptr(const PublicKeyWrapper &pk_wr, bool second_element)
{
    return get_pk_arr_ptr(*(pk_wr.pk), second_element);
}

// ---------------- Clearing functions ------------------

void clear_pk(PublicKey &pk)
{
    bool incl_sp     = true;
    size_t num_bytes = get_pk_num_bytes(pk, incl_sp);
    memset(reinterpret_cast<char *>(get_pk_arr_ptr(pk)), 0, num_bytes);
}

void clear_sk(const seal::SEALContext &context, SecretKey &sk)
{
    bool incl_sp     = true;
    size_t num_bytes = get_sk_num_bytes(sk, context, incl_sp);
    memset(reinterpret_cast<char *>(get_sk_arr_ptr(sk)), 0, num_bytes);
}

// ---------------- Comparison ------------------

bool same_pk(const PublicKeyWrapper &pk1_wr, const PublicKeyWrapper &pk2_wr, bool compare_sp)
{
    assert(pk1_wr.pk);
    assert(pk2_wr.pk);
    assert(pk1_wr.pk->data().data() == get_pk_arr_ptr(pk1_wr, 0));
    assert(pk2_wr.pk->data().data() == get_pk_arr_ptr(pk2_wr, 0));

    if (pk1_wr.is_ntt != pk2_wr.is_ntt) return false;

    auto &data1 = pk1_wr.pk->data();  // backing ciphertext
    auto &data2 = pk2_wr.pk->data();  // backing ciphertext

    // -- Compare polynomial degree, # of primes, # of components
    if (data1.poly_modulus_degree() != data2.poly_modulus_degree()) return false;
    if (data1.coeff_modulus_size() != data2.coeff_modulus_size()) return false;
    if (data1.size() != data2.size()) return false;
    assert(compare_sp || data1.coeff_modulus_size() > 1);

    size_t num_bytes = get_pk_num_bytes(*(pk1_wr.pk), compare_sp) / 2;
    cout << "num bytes: " << num_bytes << endl;

    bool same_pk1 = !memcmp(reinterpret_cast<const char *>(data1.data()),
                            reinterpret_cast<const char *>(data2.data()), num_bytes);

    bool same_pk2 = !memcmp(reinterpret_cast<const char *>(get_pk_arr_ptr(pk1_wr, 1)),
                            reinterpret_cast<const char *>(get_pk_arr_ptr(pk2_wr, 1)), num_bytes);

    // -- For debugging
    /*
    if (!same_pk1)
    {
        auto data1_ptr = reinterpret_cast<const char *>(data1.data());
        auto data2_ptr = reinterpret_cast<const char *>(data2.data());
        for (size_t i = 0; i < num_bytes; i++)
        {
            const char d1 = data1_ptr[i];
            const char d2 = data2_ptr[i];
            if (d1 != d2)
            {
                cout << "mismatched index: " << i << endl;
                cout << "d1: " << d1 << endl;
                cout << "d2: " << d2 << endl;
                break;
            }
        }
    }
    */

    return same_pk1 && same_pk2;
}

bool same_sk(const SecretKey &sk1, const SecretKey &sk2, const seal::SEALContext &context,
             bool compare_sp)
{
    size_t num_bytes1 = get_sk_num_bytes(sk1, context);
    size_t num_bytes2 = get_sk_num_bytes(sk2, context);
    assert(num_bytes1 == num_bytes2);

    auto &data1 = sk1.data();  // backing plaintext
    auto &data2 = sk2.data();  // backing plaintext

    if (data1.is_ntt_form() != data2.is_ntt_form())
    {
        cout << "secret keys are not in the same form " << endl;
        return false;
    }

    if (compare_sp) { return data1 == data2; }
    else
    {
        return (!memcmp(reinterpret_cast<const char *>(data1.data()),
                        reinterpret_cast<const char *>(data2.data()), num_bytes1));
    }
}

// ---------------- Printing ------------------

void print_all_moduli(EncryptionParameters &parms)
{
    cout << "Primes and const_ratio hw/lw: " << endl;
    for (size_t i = 0; i < parms.coeff_modulus().size(); i++)
    {
        cout << " coeff_modulus[";
        if (i < 10) cout << " ";
        cout << i << "]: ";
        cout << parms.coeff_modulus()[i].value();
        cout << "  (";
        double const_ratio_double = floor(pow(2, 64) / parms.coeff_modulus()[i].value());
        uint64_t const_ratio      = static_cast<uint64_t>(const_ratio_double);
        auto high_word            = const_ratio >> 32;
        auto low_word             = const_ratio & (0xFFFFFFFF);
        cout << std::hex << "hw = 0x" << high_word << ", lw = 0x" << low_word;
        cout << std::dec << ")" << endl;
    }
}

void print_ct(Ciphertext &ct, size_t print_size)
{
    size_t n          = ct.poly_modulus_degree();
    size_t ct_nprimes = ct.coeff_modulus_size();
    size_t ct_size    = ct.size();  // should be 2 for freshly encrypted
    bool is_ntt       = ct.is_ntt_form();
    assert(ct_nprimes);
    assert(ct_size >= 2);

    string ct_name_base = is_ntt ? "(ntt) ct" : "      ct";
    cout << endl;
    for (size_t i = 0; i < ct_size; i++)
    {
        string ct_name_base_i = ct_name_base + to_string(i) + "[";
        for (size_t j = 0; j < ct_nprimes; j++)
        {
            string ct_name_ij = ct_name_base_i + to_string(j) + "]";
            size_t offset     = n * (ct_nprimes * i + j);
            print_poly(ct_name_ij, get_ct_arr_ptr(ct) + offset, print_size);
        }
    }
}

void print_pk(string name, PublicKeyWrapper &pk_wr, size_t print_size, bool print_sp)
{
    size_t n       = pk_wr.pk->data().poly_modulus_degree();
    size_t nprimes = pk_wr.pk->data().coeff_modulus_size();
    bool is_ntt    = pk_wr.is_ntt;
    assert(pk_wr.pk->data().size() == 2);
    assert(print_sp || nprimes > 1);

    string base = is_ntt ? "(ntt form)     " : "(regular form) ";
    cout << endl;
    for (size_t t = 0; t < nprimes; t++)
    {
        for (size_t k = 0; k < 2; k++)
        {
            string pk_name_kt = base + name;
            pk_name_kt += "[" + to_string(k) + "][" + to_string(t) + "]";

            print_poly(pk_name_kt, get_pk_arr_ptr(pk_wr, k) + (t * n), print_size);
        }
        if (!print_sp && t == (nprimes - 2)) break;
    }
}

void print_sk_compare(string name1, SecretKey &sk1, string name2, SecretKey &sk2,
                      const seal::SEALContext &context, size_t print_size, bool print_sp)
{
    auto &parms_id1 = (sk1.parms_id() == parms_id_zero) ? context.key_parms_id() : sk1.parms_id();
    auto sk1_context_data_ptr = context.get_context_data(parms_id1);
    auto &parms1              = sk1_context_data_ptr->parms();
    size_t n                  = parms1.poly_modulus_degree();
    size_t nprimes            = parms1.coeff_modulus().size();
    bool is_ntt               = sk1.data().is_ntt_form();
    assert(print_sp || nprimes > 1);

    auto &parms_id2 = (sk2.parms_id() == parms_id_zero) ? context.key_parms_id() : sk2.parms_id();
    auto sk2_context_data_ptr = context.get_context_data(parms_id2);
    auto &parms2              = sk2_context_data_ptr->parms();
    assert(n == parms2.poly_modulus_degree());
    assert(nprimes == parms2.coeff_modulus().size());
    assert(is_ntt == sk2.data().is_ntt_form());

    string base = is_ntt ? "(ntt form)     " : "(regular form) ";
    cout << endl << endl;
    for (size_t t = 0; t < nprimes; t++)
    {
        string idx = "[" + to_string(t) + "]";
        print_poly(base + name1 + idx, get_sk_arr_ptr(sk1) + t * n, print_size);
        print_poly(base + name2 + idx, get_sk_arr_ptr(sk2) + t * n, print_size);
        if (!print_sp && t == (nprimes - 2)) break;
    }
}

void print_pk_compare(string name1, const PublicKeyWrapper &pk1_wr, string name2,
                      const PublicKeyWrapper &pk2_wr, size_t print_size, bool print_sp)
{
    size_t n       = pk1_wr.pk->data().poly_modulus_degree();
    size_t nprimes = pk1_wr.pk->data().coeff_modulus_size();
    bool is_ntt    = pk1_wr.is_ntt;

    assert(pk1_wr.pk->data().size() == 2);
    assert(pk1_wr.pk->data().size() == pk2_wr.pk->data().size());
    assert(n == pk2_wr.pk->data().poly_modulus_degree());
    assert(nprimes == pk2_wr.pk->data().coeff_modulus_size());
    assert(print_sp || nprimes > 1);
    assert(is_ntt == pk2_wr.is_ntt);

    string base = is_ntt ? "(ntt form)     " : "(regular form) ";
    cout << endl << endl;
    for (size_t t = 0; t < nprimes; t++)
    {
        for (size_t k = 0; k < 2; k++)
        {
            string idx = "[" + to_string(k) + "][" + to_string(t) + "]";
            print_poly(base + name1 + idx, get_pk_arr_ptr(pk1_wr, k) + (t * n), print_size);
            print_poly(base + name2 + idx, get_pk_arr_ptr(pk2_wr, k) + (t * n), print_size);
        }
        if (!print_sp && t == (nprimes - 2)) break;
    }
}

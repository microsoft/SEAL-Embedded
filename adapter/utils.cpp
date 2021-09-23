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

void set_modulus_testing_values(size_t nprimes, vector<Modulus> &vec)
{
    vec.clear();

    // -- Note: We create the vector in reverse first
    switch (nprimes)
    {
        case 8: vec.push_back(Modulus(1073643521)); [[fallthrough]];
        case 7: vec.push_back(Modulus(1073479681)); [[fallthrough]];
        case 6: vec.push_back(Modulus(1073184769)); [[fallthrough]];
        case 5: vec.push_back(Modulus(1073053697)); [[fallthrough]];
        case 4: vec.push_back(Modulus(1072857089)); [[fallthrough]];
        case 3: vec.push_back(Modulus(1072496641)); [[fallthrough]];
        case 2: vec.push_back(Modulus(1071513601)); [[fallthrough]];
        case 1: vec.push_back(Modulus(1071415297)); [[fallthrough]];
        default: break;
    }
    reverse(vec.begin(), vec.end());
}

SEALContext setup_seale_custom(size_t degree, const vector<Modulus> &moduli,
                               EncryptionParameters &parms)
{
    for (size_t i = 0; i < moduli.size() - 1; i++)
    {
        // -- Required for compatibility with SEAL-embedded.
        assert(moduli[i].bit_count() <= 30);
    }
    parms.set_poly_modulus_degree(degree);
    parms.set_coeff_modulus(moduli);
    SEALContext context(parms);
    print_parameters(context);
    cout << endl;
    return context;
}

SEALContext setup_seale_30bitprime_default(size_t degree, EncryptionParameters &parms)
{
    int n_30bit_primes;
    int special_prime_bitcount;
    switch (degree)
    {
        case 2048:
            n_30bit_primes         = 1;
            special_prime_bitcount = 24;
            break;
        case 4096:
            n_30bit_primes         = 3;
            special_prime_bitcount = 19;
            break;
        case 8192:
            n_30bit_primes         = 6;
            special_prime_bitcount = 38;
            break;
        default: throw runtime_error("Please use a different setup function"); break;
    }
    vector<Modulus> moduli;
    set_modulus_testing_values(n_30bit_primes, moduli);
    vector<Modulus> special_prime = CoeffModulus::Create(degree, {special_prime_bitcount});
    moduli.push_back(special_prime[0]);
    return setup_seale_custom(degree, moduli, parms);
}

SEALContext setup_seal_api(size_t degree, const vector<int> &bit_lengths,
                           EncryptionParameters &parms)
{
    return setup_seale_custom(degree, CoeffModulus::Create(degree, bit_lengths), parms);
}

// ---------------- Size functions ------------------

size_t get_sk_num_bytes(const SecretKey &sk, const SEALContext &context, bool incl_sp)
{
    auto &sk_parms            = context.key_context_data()->parms();
    auto &coeff_modulus       = sk_parms.coeff_modulus();
    size_t n                  = sk_parms.poly_modulus_degree();
    size_t coeff_modulus_size = coeff_modulus.size();
    size_t type_size          = sizeof(sk.data().data()[0]);

    size_t num_primes = incl_sp ? coeff_modulus_size : coeff_modulus_size - 1;
    return n * num_primes * type_size;
}

size_t get_pk_num_bytes(const PublicKey &pk, bool incl_sp)
{
    size_t n                  = pk.data().poly_modulus_degree();
    size_t coeff_modulus_size = pk.data().coeff_modulus_size();
    size_t type_size          = sizeof(pk.data().data()[0]);
    size_t num_components     = pk.data().size();
    assert(num_components == 2);

    size_t num_primes = incl_sp ? coeff_modulus_size : coeff_modulus_size - 1;
    return n * num_primes * type_size * 2;
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
        size_t n                  = ct.poly_modulus_degree();
        size_t coeff_modulus_size = ct.coeff_modulus_size();
        return reinterpret_cast<uint64_t *>(&(ct[n * coeff_modulus_size]));
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
        size_t n                  = pk.data().poly_modulus_degree();
        size_t coeff_modulus_size = pk.data().coeff_modulus_size();
        return reinterpret_cast<uint64_t *>(&(pk.data()[n * coeff_modulus_size]));
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
    bool incl_special_prime = true;
    size_t num_bytes        = get_pk_num_bytes(pk, incl_special_prime);
    memset(reinterpret_cast<char *>(get_pk_arr_ptr(pk)), 0, num_bytes);
}

void clear_sk(const SEALContext &context, SecretKey &sk)
{
    bool incl_special_prime = true;
    size_t num_bytes        = get_sk_num_bytes(sk, context, incl_special_prime);
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

    auto &data1 = pk1_wr.pk->data();  // This is actually the backing ciphertext
    auto &data2 = pk2_wr.pk->data();  // This is actually the backing ciphertext

    // -- Compare polynomial degree
    size_t n1 = data1.poly_modulus_degree();
    size_t n2 = data2.poly_modulus_degree();
    if (n1 != n2) return false;

    // -- Compare number of primes
    size_t coeff_modulus_size1 = data1.coeff_modulus_size();
    size_t coeff_modulus_size2 = data2.coeff_modulus_size();
    if (coeff_modulus_size1 != coeff_modulus_size2) return false;

    // -- Compare number of components
    size_t num_components1 = data1.size();
    size_t num_components2 = data2.size();
    if (num_components1 != num_components2) return false;

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

bool same_sk(const SecretKey &sk1, const SecretKey &sk2, const SEALContext &context,
             bool compare_sp)
{
    bool sk1_is_ntt = sk1.data().is_ntt_form();
    bool sk2_is_ntt = sk2.data().is_ntt_form();

    if (sk1_is_ntt != sk2_is_ntt)
    {
        cout << "secret keys are not in the same form " << endl;
        return false;
    }

    auto &data1 = sk1.data();  // This is actually the backing plaintext
    auto &data2 = sk2.data();  // This is actually the backing plaintext

    size_t num_bytes1 = get_sk_num_bytes(sk1, context);
    size_t num_bytes2 = get_sk_num_bytes(sk2, context);
    assert(num_bytes1 == num_bytes2);

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
    cout << "Primes: " << endl;
    for (size_t i = 0; i < parms.coeff_modulus().size(); i++)
    {
        cout << " coeff_modulus[" << i << "]: ";
        cout << parms.coeff_modulus()[i].value() << endl;
    }
}

void print_ct(Ciphertext &ct, size_t print_size)
{
    // total mod size >= 2, so ct_mod_size >=1
    size_t ct_mod_size = ct.coeff_modulus_size();
    size_t ct_size     = ct.size();  // should be 2 for freshly encrypted
    size_t n           = ct.poly_modulus_degree();
    bool is_ntt        = ct.is_ntt_form();
    assert(ct_mod_size >= 1);
    assert(ct_size >= 2);

    string ct_name_base = is_ntt ? "(ntt) ct" : "      ct";
    cout << endl;
    for (size_t i = 0; i < ct_size; i++)
    {
        string ct_name_base_i = ct_name_base + to_string(i) + "[";
        for (size_t j = 0; j < ct_mod_size; j++)
        {
            string ct_name_ij = ct_name_base_i + to_string(j) + "]";
            size_t offset     = n * (ct_mod_size * i + j);
            print_poly(ct_name_ij, get_ct_arr_ptr(ct) + offset, print_size);
        }
    }
}

void print_pk(string name, PublicKeyWrapper &pk_wr, size_t print_size, bool print_sp)
{
    size_t n       = pk_wr.pk->data().poly_modulus_degree();
    size_t nprimes = pk_wr.pk->data().coeff_modulus_size();
    bool is_ntt    = pk_wr.is_ntt;
    assert(nprimes >= 2);
    assert(pk_wr.pk->data().size() == 2);

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
                      const SEALContext &context, size_t print_size, bool print_sp)
{
    auto &parms_id1 = (sk1.parms_id() == parms_id_zero) ? context.key_parms_id() : sk1.parms_id();
    auto sk1_context_data_ptr = context.get_context_data(parms_id1);
    auto &parms1              = sk1_context_data_ptr->parms();
    size_t n                  = parms1.poly_modulus_degree();
    size_t nprimes            = parms1.coeff_modulus().size();
    bool is_ntt               = sk1.data().is_ntt_form();
    assert(nprimes >= 2);

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
    assert(nprimes >= 2);  // minimum is 2
    assert(nprimes == pk2_wr.pk->data().coeff_modulus_size());
    assert(n == pk2_wr.pk->data().poly_modulus_degree());
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

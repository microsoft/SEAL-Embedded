// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file fileops.cpp
*/

#include "fileops.h"

#include <cassert>
#include <fstream>
#include <iostream>

#include "convert.h"
#include "seal/seal.h"
#include "utils.h"

using namespace std;
using namespace seal;
using namespace seal::util;

// #define HIGH_BYTE_FIRST // big endian
// #define DEBUG_EASYMOD // See: SEAL-Embedded for explanation

// ===============================================================
//                     Binary file save/load
//                     (SEAL-Embedded format)
// ===============================================================

void sk_bin_file_save(string fpath, string str_fpath, const SEALContext &context,
                      bool use_str_fpath, SecretKey &sk)
{
    auto &sk_parms = context.key_context_data()->parms();
    size_t n       = sk_parms.poly_modulus_degree();
    auto sk_ptr    = get_sk_arr_ptr(sk);
    bool is_ntt    = sk.data().is_ntt_form();

    if (is_ntt)
    {
        sk_to_non_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == false);
    }

    fstream outfile(fpath.c_str(), ios::out | ios::binary | ios::trunc);
    fstream outfile2;
    if (use_str_fpath)
    {
        outfile2.open(str_fpath.c_str(), ios::out | ios::trunc);
        outfile2 << "#pragma once\n\n#include \"defines.h\"\n\n";
        outfile2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                    "defined(SE_DATA_FROM_CODE_DIRECT)\n";
        outfile2 << "\n#include <stdint.h>\n\n";
        // -- # bytes = n vals * (2 bits / val) * (1 byte / 8 bits)
        size_t nbytes = n / 4;
        outfile2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
        outfile2 << "// -- Secret key for polynomial ring degree = " << n << "\n";
        outfile2 << "uint8_t secret_key[" << nbytes << "] = { ";
    }
    for (size_t i = 0; i < n; i += 4)
    {
        uint8_t byte = 0;
        for (size_t j = 0; ((i + j) < n) && (j < 4); j++)
        {
            uint64_t sk_val = sk_ptr[i + j];
            if (i < 8) cout << "sk_val: " << sk_val << endl;
#ifdef DEBUG_EASYMOD
            // -- Mapping: 0 --> 0, 1 --> 1, q-1 --> 2
            uint8_t val = (sk_val > 1) ? 2 : (uint8_t)sk_val;
#else
            // -- Mapping: q-1 --> 0, 0 --> 1, 1 --> 2
            uint8_t val = (sk_val > 1) ? 0 : (uint8_t)(sk_val + 1);
            // if (i < 8) cout << "val: " << (uint64_t)val << endl;
#endif
            byte |= val << (6 - 2 * j);
        }
        // TODO: make this more compact?
        outfile.put(static_cast<char>(byte));
        if (use_str_fpath)
        {
            // cout << "byte: " << to_string(byte) << endl;
            string next_str = ((i + 4) < n) ? ", " : "};\n";
            outfile2 << to_string(byte) + next_str;
            if (!(i % 64) && i) outfile2 << "\n";
        }
    }
    outfile.close();
    if (use_str_fpath)
    {
        outfile2 << "#endif" << endl;
        outfile2.close();
    }

    if (is_ntt)
    {
        sk_to_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == true);
    }
}

void sk_bin_file_load(string fpath, const SEALContext &context, SecretKey &sk)
{
    auto &sk_parms            = context.key_context_data()->parms();
    auto &coeff_modulus       = sk_parms.coeff_modulus();
    size_t coeff_modulus_size = coeff_modulus.size();
    size_t n                  = sk_parms.poly_modulus_degree();
    auto sk_ptr               = get_sk_arr_ptr(sk);
    bool is_ntt               = sk.data().is_ntt_form();

    // -- Make sure sk is not in NTT form
    if (is_ntt)
    {
        sk_to_non_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == false);
    }

    // print_poly("sk_before  ", sk_ptr, 8);
    fstream outfile(fpath.c_str(), ios::in | ios::binary);
    for (size_t i = 0; i < n; i += 4)
    {
        char byte;
        outfile.get(byte);
        // cout << "byte[" << i << "]: " << hex << unsigned(byte & 0xFF) << dec << endl;
        for (size_t j = 0; ((i + j) < n) && (j < 4); j++)
        {
            uint8_t val = ((uint8_t)byte >> (6 - 2 * j)) & 0b11;
            // cout << "val[" << j << "]: " << hex << unsigned(val) << dec << endl;
#ifdef DEBUG_EASYMOD
            // -- Mapping: 0 --> 0, 1 --> 1, 2 --> q-1
            if (val > 1)
            {
                for (size_t k = 0; k < coeff_modulus_size; k++)
                {
                    sk_ptr[(i + j) + k * n] = coeff_modulus[k].value() - 1;
                }
            }
            else
            {
                for (size_t k = 0; k < coeff_modulus_size; k++) { sk_ptr[(i + j) + k * n] = val; }
            }
#else
            // -- Mapping: 0 --> q-1, 1 --> 0, 2 --> 1
            if (val > 0)
            {
                for (size_t k = 0; k < coeff_modulus_size; k++)
                {
                    sk_ptr[(i + j) + k * n] = val - 1;
                }
            }
            else
            {
                for (size_t k = 0; k < coeff_modulus_size; k++)
                {
                    sk_ptr[(i + j) + k * n] = coeff_modulus[k].value() - 1;
                }
            }
#endif
        }
    }
    outfile.close();
    // print_poly("sk_after   ", sk_ptr, n*4);
    // print_poly("sk_after   ", sk_ptr, 8);
    // print_poly("sk_after   ", sk_ptr + n, 8);
    // print_poly("sk_after   ", sk_ptr + 2 * n, 8);
    // print_poly("sk_after   ", sk_ptr + 3 * n, 8);

    // -- Convert sk back to NTT form
    if (is_ntt)
    {
        sk_to_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == true);
    }
}

void pk_bin_file_save(string dirpath, const SEALContext &context, PublicKeyWrapper &pk_wr,
                      bool incl_sp, bool append)
{
    bool large_modulus = 0;
    bool is_ntt        = pk_wr.is_ntt;
    assert(is_ntt);  // Make sure we start out in ntt form
    assert(pk_wr.pk);

    bool incl_sp_sf = false;  // No need to include the special prime for the headers

    size_t n                  = pk_wr.pk->data().poly_modulus_degree();
    size_t coeff_modulus_size = pk_wr.pk->data().coeff_modulus_size();
    assert(coeff_modulus_size >= 2);

    string fpath3 = dirpath + "str_pk_addr_array.h";
    fstream outfile3(fpath3.c_str(), ios::out | ios::trunc);

    outfile3 << "#pragma once\n\n#include \"defines.h\"\n\n";
    outfile3 << "#if defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT)\n\n";

    stringstream pk_addr_str;

    size_t string_file_coeff_modulus_size =
        incl_sp_sf ? coeff_modulus_size : coeff_modulus_size - 1;
    pk_addr_str << "ZZ* pk_prime_addr[" << string_file_coeff_modulus_size << "][2] = \n{\n";

    for (size_t outer = 0; outer < 2; outer++)
    {
        assert(pk_wr.pk->data().size() == 2);
        assert(sizeof(pk_wr.pk->data().data()[0]) == sizeof(uint64_t));

        for (size_t t = 0; t < coeff_modulus_size; t++)
        {
            for (size_t k = 0; k < 2; k++)  // pk0, pk1
            {
                string fpath_common = "pk" + to_string(k) + "_";
                if (pk_wr.is_ntt) fpath_common += "ntt_";

                auto coeff_modulus = context.key_context_data()->parms().coeff_modulus()[t].value();
                fpath_common += to_string(n) + "_" + to_string(coeff_modulus);

                string fpath  = dirpath + fpath_common + ".dat";
                string fpath2 = dirpath + "str_" + fpath_common + ".h";
                cout << "writing to files: " << fpath << ", " << fpath2 << endl;

                if (outer == 0 && t < string_file_coeff_modulus_size)
                {
                    outfile3 << "   #include \"str_" << fpath_common + ".h\"" << endl;
                }

                auto open_mode_type = ios::out | ios::binary | (append ? ios::app : ios::trunc);
                fstream outfile(fpath.c_str(), open_mode_type);

                fstream outfile2(fpath2.c_str(), ios::out | ios::trunc);
                size_t num_ZZ_elements = n;
                outfile2 << "#pragma once\n\n#include \"defines.h\"\n\n";
                outfile2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                            "defined(SE_DATA_FROM_CODE_DIRECT)\n";
                outfile2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
                outfile2 << "ZZ pk" + to_string(k) + "_prime" << to_string(t);
                outfile2 << "[" << num_ZZ_elements << "] = { \n";

                uint64_t *ptr = get_pk_arr_ptr(pk_wr, k);
                for (size_t i = 0; i < n; i++)
                {
                    uint64_t data = ptr[i + t * n];
                    size_t stop_j = (large_modulus) ? 8 : 4;
                    for (size_t j = 0; j < stop_j; j++)  // write one byte at a time
                    {
                        // -- & with 0xFF to prevent 'byte' from being sign extended
#ifdef HIGH_BYTE_FIRST
                        uint8_t byte = (data >> (56 - 8 * j)) & 0xFF;
#else
                        uint8_t byte = (data >> (8 * j)) & 0xFF;
                        outfile.put((char)byte);
#endif
                    }

                    // -- Write to string file
                    uint32_t val    = data & 0xFFFFFFFF;
                    string next_str = ((i + 1) < n) ? ", " : "};\n";
                    outfile2 << hex << "0x" << (large_modulus ? data : val) << next_str;
                    size_t row_break = large_modulus ? 4 : 8;
                    if (!(i % row_break)) outfile2 << "\n";
                }
                outfile.close();
                outfile2 << "#endif" << endl;
                outfile2.close();
            }
            // TODO: get rid of special prime stuff since this must be included as
            // part of the three primes...
            // -- No need to include special prime in string files
            if (outer == 0 && t < string_file_coeff_modulus_size)
            {
                pk_addr_str << "    {&(pk0_prime" << to_string(t) << "[0]),";
                pk_addr_str << " &(pk1_prime" << to_string(t) << "[0])}";
                if (t == string_file_coeff_modulus_size - 1)
                    pk_addr_str << "\n};" << endl;
                else
                    pk_addr_str << "," << endl;
            }
            if (!incl_sp && t == (coeff_modulus_size - 2)) break;
        }

        if (pk_wr.is_ntt)  // Convert and write in non-ntt form
        {
            pk_to_non_ntt_form(context, pk_wr);
            assert(pk_wr.is_ntt == false);
        }
        if (outer == 0) { outfile3 << "\n"; }
    }

    outfile3 << pk_addr_str.str();
    outfile3 << "#endif" << endl;
    outfile3.close();

    if (is_ntt)
    {
        pk_to_ntt_form(context, pk_wr);
        assert(pk_wr.is_ntt == true);
    }
}

void pk_bin_file_load(string dirpath, const SEALContext &context, PublicKeyWrapper &pk_wr,
                      bool incl_sp)
{
    bool is_ntt        = pk_wr.is_ntt;
    bool large_modulus = 0;
    assert(pk_wr.pk);

    if (is_ntt)
    {
        pk_to_non_ntt_form(context, pk_wr);
        assert(pk_wr.is_ntt == false);
    }

    assert(pk_wr.pk->data().size() == 2);
    assert(sizeof(pk_wr.pk->data().data()[0]) == sizeof(uint64_t));

    size_t n                  = pk_wr.pk->data().poly_modulus_degree();
    size_t coeff_modulus_size = pk_wr.pk->data().coeff_modulus_size();
    assert(coeff_modulus_size >= 2);

    for (size_t t = 0; t < coeff_modulus_size; t++)
    {
        for (size_t k = 0; k < 2; k++)
        {
            string fpath = dirpath + "pk" + to_string(k) + "_";
            if (pk_wr.is_ntt) fpath += "ntt_";

            auto coeff_modulus = context.key_context_data()->parms().coeff_modulus()[t].value();
            fpath += to_string(n) + "_" + to_string(coeff_modulus) + ".dat";
            cout << "reading from file: " << fpath << endl;

            fstream infile(fpath.c_str(), ios::in | ios::binary);

            uint64_t *ptr = get_pk_arr_ptr(pk_wr, k);
            for (size_t i = 0; i < n; i++)  // # of elements in pk0, pk1
            {
                uint64_t data = 0;
                size_t stop_j = (large_modulus) ? 8 : 4;
                for (size_t j = 0; j < stop_j; j++)  // write one byte at a time
                {
                    char byte;
                    infile.get(byte);

                    // -- & with 0xFF to prevent 'byte' from being sign extended
#ifdef HIGH_BYTE_FIRST
                    data <<= 8;
                    data |= (byte & 0xFF);
#else
                    data |= ((uint64_t)(byte & 0xFF) << 8 * j);
#endif
                }
                ptr[i + t * n] = data;
            }
            infile.close();
        }
        if (!incl_sp && t == (coeff_modulus_size - 2)) break;
    }

    if (is_ntt)
    {
        pk_to_ntt_form(context, pk_wr);
        assert(pk_wr.is_ntt == true);
    }
}

// ==============================================================
//                      Binary file save/load
//                         (SEAL format)
// ==============================================================

void sk_seal_save(string fpath, SecretKey &sk, bool compress)
{
    string action = "Saving secret key to file at \"" + fpath + "\"";

    fstream file(fpath, fstream::binary | fstream::out | fstream::trunc);
    exit_on_err_file(file, action, 0);

    cerr << action << " ..." << endl;
    if (compress)
        sk.save(file, compr_mode_type::zstd);
    else
        sk.save(file, compr_mode_type::none);
    file.close();
}

void sk_seal_load(string fpath, const SEALContext &context, SecretKey &sk)
{
    string action = "Loading secret key from file at \"" + fpath + "\"";

    fstream file(fpath, fstream::binary | fstream::in);
    exit_on_err_file(file, action, 1);

    cerr << action << " ..." << endl;
    sk.load(context, file);
    file.close();
}

void pk_seal_save(string fpath, PublicKey &pk, bool compress)
{
    string action = "Saving public key to file at \"" + fpath + "\"";

    fstream file(fpath, fstream::binary | fstream::out | fstream::trunc);
    exit_on_err_file(file, action, 0);

    cerr << action << " ..." << endl;
    if (compress)
        pk.save(file, compr_mode_type::zstd);
    else
        pk.save(file, compr_mode_type::none);
    file.close();
}

void pk_seal_load(string fpath, const SEALContext &context, PublicKey &pk)
{
    string action = "Loading public key from file at \"" + fpath + "\"";

    fstream file(fpath, fstream::binary | fstream::in);
    exit_on_err_file(file, action, 1);
    cerr << action << " ..." << endl;
    pk.load(context, file);
    file.close();
}

// ==============================================================
//                    String file save/load
//                    (Mainly for debugging)
// ==============================================================

streampos sk_string_file_load(string fpath, const SEALContext &context, SecretKey &sk)
{
    auto &sk_parms            = context.key_context_data()->parms();
    auto &coeff_modulus       = sk_parms.coeff_modulus();
    size_t coeff_modulus_size = coeff_modulus.size();
    size_t n                  = sk_parms.poly_modulus_degree();
    auto sk_ptr               = get_sk_arr_ptr(sk);
    bool is_ntt               = sk.data().is_ntt_form();

    // -- Make sure sk is not in NTT form
    // -- Note: this shouldn't run if we don't generate sk using keygenerator
    if (is_ntt)
    {
        sk_to_non_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == false);
    }

    auto filepos = poly_string_file_load(fpath, sk_ptr, 1);

    for (size_t i = 0; i < n; i++)
    {
        uint64_t val = sk_ptr[i];
#ifdef DEBUG_EASYMOD
        if (val > 1)
        {
            for (size_t j = 0; j < coeff_modulus_size; j++)
            {
                sk_ptr[i + j * n] = coeff_modulus[j].value() - 1;
            }
        }
        else
        {
            for (size_t j = 0; j < coeff_modulus_size; j++) { sk_ptr[i + j * n] = val; }
        }
#else
        if (val > 0)
        {
            for (size_t j = 0; j < coeff_modulus_size; j++) { sk_ptr[i + j * n] = val - 1; }
        }
        else
        {
            for (size_t j = 0; j < coeff_modulus_size; j++)
            {
                sk_ptr[i + j * n] = coeff_modulus[j].value() - 1;
            }
        }
#endif
    }

    // -- Convert back to NTT form if necessary
    if (is_ntt)
    {
        sk_to_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == true);
    }

    return filepos;
}

streampos ct_string_file_load(string fpath, const SEALContext &context, Evaluator &evaluator,
                              Ciphertext &ct, streampos filepos_in)
{
    auto &ct_parms            = context.first_context_data()->parms();
    auto &ct_parms_id         = context.first_parms_id();
    size_t coeff_modulus_size = ct_parms.coeff_modulus().size();
    size_t n                  = ct_parms.poly_modulus_degree();
    bool is_ntt               = ct.is_ntt_form();

    assert(is_ntt);
    assert(coeff_modulus_size == 3);

    // -- Ciphertext has two components
    ct.resize(context, ct_parms_id, 2);

    // -- Make sure ciphertext is in NTT form
    if (!is_ntt)
    {
        ct_to_non_ntt_form(evaluator, ct);
        assert(ct.is_ntt_form() == false);
    }

    vector<uint64_t> ct_temp_1p(2 * n);

    streampos filepos = filepos_in;
    for (size_t j = 0; j < coeff_modulus_size; j++)
    {
        // -- Load in two components at a time
        filepos     = poly_string_file_load(fpath, ct_temp_1p, 2, filepos);
        auto ct_ptr = get_ct_arr_ptr(ct);

        // -- Set c0 and c1 terms
        for (size_t i = 0; i < n; i++)
        {
            ct_ptr[i + j * n]                          = ct_temp_1p[i];
            ct_ptr[i + j * n + coeff_modulus_size * n] = ct_temp_1p[i + n];
        }
    }
    print_ct(ct, 8);

    // -- Convert ciphertext back to non-NTT form if necessary
    if (!is_ntt)
    {
        ct_to_ntt_form(evaluator, ct);
        assert(ct.is_ntt_form() == true);
    }

    return filepos;
}

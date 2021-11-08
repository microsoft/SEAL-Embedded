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

    fstream file(fpath.c_str(), ios::out | ios::binary | ios::trunc);
    fstream file2;
    if (use_str_fpath)
    {
        file2.open(str_fpath.c_str(), ios::out | ios::trunc);
        file2 << "#pragma once\n\n#include \"defines.h\"\n\n";
        file2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                 "defined(SE_DATA_FROM_CODE_DIRECT)\n";
        file2 << "\n#include <stdint.h>\n\n";
        // -- # bytes = n vals * (2 bits / val) * (1 byte / 8 bits)
        size_t nbytes = n / 4;
        file2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
        file2 << "// -- Secret key for polynomial ring degree = " << n << "\n";
        file2 << "uint8_t secret_key[" << nbytes << "] = { ";
    }
    for (size_t i = 0; i < n; i += 4)
    {
        uint8_t byte = 0;
        for (size_t j = 0; ((i + j) < n) && (j < 4); j++)
        {
            uint64_t sk_val = sk_ptr[i + j];
            // if (i < 8) cout << "sk_val: " << sk_val << endl;
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
        file.put(static_cast<char>(byte));
        if (use_str_fpath)
        {
            // cout << "byte: " << to_string(byte) << endl;
            string next_str = ((i + 4) < n) ? ", " : "};\n";
            assert((byte & 0b11) != 0b11);
            assert(((byte >> 2) & 0b11) != 0b11);
            assert(((byte >> 4) & 0b11) != 0b11);
            assert(((byte >> 6) & 0b11) != 0b11);
            if (byte < 10)
                file2 << "  ";
            else if (byte < 100)
                file2 << " ";
            file2 << to_string(byte) + next_str;
            if (!(i % 64)) file2 << "\n";
        }
    }
    file.close();
    if (use_str_fpath)
    {
        file2 << "#endif" << endl;
        file2.close();
    }

    if (is_ntt)
    {
        sk_to_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == true);
    }
}

void sk_bin_file_load(string fpath, const SEALContext &context, SecretKey &sk)
{
    auto &sk_parms      = context.key_context_data()->parms();
    auto &coeff_modulus = sk_parms.coeff_modulus();
    size_t nprimes      = coeff_modulus.size();
    size_t n            = sk_parms.poly_modulus_degree();
    auto sk_ptr         = get_sk_arr_ptr(sk);
    bool is_ntt         = sk.data().is_ntt_form();

    // -- Make sure sk is not in NTT form
    if (is_ntt)
    {
        sk_to_non_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == false);
    }

    // print_poly("sk_before  ", sk_ptr, 8);
    fstream file(fpath.c_str(), ios::in | ios::binary);
    for (size_t i = 0; i < n; i += 4)
    {
        char byte;
        file.get(byte);
        // cout << "byte[" << i << "]: " << hex << unsigned(byte & 0xFF) << dec << endl;
        for (size_t j = 0; ((i + j) < n) && (j < 4); j++)
        {
            uint8_t val = ((uint8_t)byte >> (6 - 2 * j)) & 0b11;
            // cout << "val[" << j << "]: " << hex << unsigned(val) << dec << endl;
#ifdef DEBUG_EASYMOD
            // -- Mapping: 0 --> 0, 1 --> 1, 2 --> q-1
            if (val > 1)
            {
                for (size_t k = 0; k < nprimes; k++)
                { sk_ptr[(i + j) + k * n] = coeff_modulus[k].value() - 1; }
            }
            else
            {
                for (size_t k = 0; k < nprimes; k++) { sk_ptr[(i + j) + k * n] = val; }
            }
#else
            // -- Mapping: 0 --> q-1, 1 --> 0, 2 --> 1
            if (val > 0)
            {
                for (size_t k = 0; k < nprimes; k++) { sk_ptr[(i + j) + k * n] = val - 1; }
            }
            else
            {
                for (size_t k = 0; k < nprimes; k++)
                { sk_ptr[(i + j) + k * n] = coeff_modulus[k].value() - 1; }
            }
#endif
        }
    }
    file.close();
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
                      bool incl_sp, bool high_byte_first, bool append)
{
    bool is_ntt = pk_wr.is_ntt;
    assert(is_ntt);  // Make sure we start out in ntt form
    assert(pk_wr.pk);

    size_t n       = pk_wr.pk->data().poly_modulus_degree();
    size_t nprimes = pk_wr.pk->data().coeff_modulus_size();

    // -- No need to include the special prime for the headers
    bool incl_sp_sf = (nprimes == 1) ? true : false;

    string fpath3 = dirpath + "str_pk_addr_array.h";
    fstream file3(fpath3.c_str(), ios::out | ios::trunc);
    size_t string_file_nprimes = incl_sp_sf ? nprimes : nprimes - 1;

    file3 << "#pragma once\n\n#include \"defines.h\"\n\n";
    file3 << "#if defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT)\n\n";

    stringstream pk_addr_str;
    pk_addr_str << "ZZ* pk_prime_addr[" << string_file_nprimes << "][2] = \n{\n";

    for (size_t outer = 0; outer < 2; outer++)
    {
        assert(pk_wr.pk->data().size() == 2);
        assert(sizeof(pk_wr.pk->data().data()[0]) == sizeof(uint64_t));

        for (size_t t = 0; t < nprimes; t++)
        {
            auto q             = context.key_context_data()->parms().coeff_modulus()[t].value();
            bool large_modulus = (log2(q) > 32) ? true : false;

            assert(((t <= (nprimes - 1)) && (log2(q) <= 30)) ||
                   ((t == nprimes - 1) && (log2(q) <= 64) && (nprimes != 1)));

            for (size_t k = 0; k < 2; k++)  // pk0, pk1
            {
                string fpath_common = "pk" + to_string(k) + "_";
                if (pk_wr.is_ntt) fpath_common += "ntt_";
                fpath_common += to_string(n) + "_" + to_string(q);

                string fpath  = dirpath + fpath_common + ".dat";
                string fpath2 = dirpath + "str_" + fpath_common + ".h";
                cout << "writing to files: " << fpath << ", " << fpath2 << endl;

                if (outer == 0 && t < string_file_nprimes)
                { file3 << "   #include \"str_" << fpath_common + ".h\"" << endl; }

                auto open_mode_type = ios::out | ios::binary | (append ? ios::app : ios::trunc);
                fstream file(fpath.c_str(), open_mode_type);

                fstream file2(fpath2.c_str(), ios::out | ios::trunc);
                size_t nvals = n;
                file2 << "#pragma once\n\n#include \"defines.h\"\n\n";
                if (large_modulus)
                {
                    file2 << "// -- Note: This file uses >30-bit primes and cannot";
                    file2 << " be used with the SEAL-Embedded device library." << endl;
                }
                file2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                         "defined(SE_DATA_FROM_CODE_DIRECT)\n";
                file2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
                if (large_modulus)
                    file2 << "uint64_t ";
                else
                    file2 << "ZZ ";
                file2 << "pk" + to_string(k) + "_prime" << to_string(t);
                file2 << "[" << nvals << "] = { \n";

                uint64_t *ptr = get_pk_arr_ptr(pk_wr, k);
                for (size_t i = 0; i < n; i++)
                {
                    uint64_t data = ptr[i + t * n];
                    size_t stop_j = (large_modulus) ? 8 : 4;
                    for (size_t j = 0; j < stop_j; j++)  // write one byte at a time
                    {
                        // -- & with 0xFF to prevent 'byte' from being sign extended
                        uint8_t byte = 0;
                        if (high_byte_first) { byte = (data >> (56 - 8 * j)) & 0xFF; }
                        else
                        {
                            byte = (data >> (8 * j)) & 0xFF;
                            file.put((char)byte);
                        }
                    }

                    // -- Write to string file
                    uint32_t val    = data & 0xFFFFFFFF;
                    string next_str = ((i + 1) < n) ? ", " : "};\n";
                    file2 << std::hex << "0x" << (large_modulus ? data : val) << std::dec
                          << next_str;
                    size_t row_break = large_modulus ? 4 : 8;
                    if (!(i % row_break)) file2 << "\n";
                }
                file.close();
                file2 << "#endif" << endl;
                file2.close();
            }
            // -- No need to include special prime in string files
            if (outer == 0 && t < string_file_nprimes)
            {
                pk_addr_str << "    {&(pk0_prime" << to_string(t) << "[0]),";
                pk_addr_str << " &(pk1_prime" << to_string(t) << "[0])}";
                if (t == string_file_nprimes - 1)
                    pk_addr_str << "\n};" << endl;
                else
                    pk_addr_str << "," << endl;
            }
            if (!incl_sp && t == (nprimes - 2)) break;
        }

        if (pk_wr.is_ntt)  // Convert and write in non-ntt form
        {
            pk_to_non_ntt_form(context, pk_wr);
            assert(pk_wr.is_ntt == false);
        }
        if (outer == 0) file3 << "\n";
    }

    file3 << pk_addr_str.str();
    file3 << "#endif" << endl;
    file3.close();

    if (is_ntt)
    {
        pk_to_ntt_form(context, pk_wr);
        assert(pk_wr.is_ntt == true);
    }
}

void pk_bin_file_load(string dirpath, const SEALContext &context, PublicKeyWrapper &pk_wr,
                      bool incl_sp, bool high_byte_first)
{
    bool is_ntt = pk_wr.is_ntt;
    assert(pk_wr.pk);

    if (is_ntt)
    {
        pk_to_non_ntt_form(context, pk_wr);
        assert(pk_wr.is_ntt == false);
    }

    assert(pk_wr.pk->data().size() == 2);
    assert(sizeof(pk_wr.pk->data().data()[0]) == sizeof(uint64_t));

    size_t n       = pk_wr.pk->data().poly_modulus_degree();
    size_t nprimes = pk_wr.pk->data().coeff_modulus_size();

    for (size_t t = 0; t < nprimes; t++)
    {
        auto q             = context.key_context_data()->parms().coeff_modulus()[t].value();
        bool large_modulus = (log2(q) > 32) ? true : false;

        assert(((t <= (nprimes - 1)) && (log2(q) <= 30)) ||
               ((t == nprimes - 1) && (log2(q) <= 64) && (nprimes != 1)));

        for (size_t k = 0; k < 2; k++)
        {
            string fpath = dirpath + "pk" + to_string(k) + "_";
            if (pk_wr.is_ntt) fpath += "ntt_";
            fpath += to_string(n) + "_" + to_string(q) + ".dat";
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
                    if (high_byte_first)
                    {
                        data <<= 8;
                        data |= (byte & 0xFF);
                    }
                    else
                    {
                        data |= ((uint64_t)(byte & 0xFF) << 8 * j);
                    }
                }
                ptr[i + t * n] = data;
            }
            infile.close();
        }
        if (!incl_sp && t == (nprimes - 2)) break;
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
    auto &sk_parms      = context.key_context_data()->parms();
    auto &coeff_modulus = sk_parms.coeff_modulus();
    size_t nprimes      = coeff_modulus.size();
    size_t n            = sk_parms.poly_modulus_degree();
    auto sk_ptr         = get_sk_arr_ptr(sk);
    bool is_ntt         = sk.data().is_ntt_form();

    // -- Make sure sk is not in NTT form
    // -- Note: this shouldn't run if we don't generate sk using keygenerator
    if (is_ntt)
    {
        sk_to_non_ntt_form(context, sk);
        assert(sk.data().is_ntt_form() == false);
    }

    auto filepos = poly_string_file_load(fpath, 1, sk_ptr);

    for (size_t i = 0; i < n; i++)
    {
        uint64_t val = sk_ptr[i];
#ifdef DEBUG_EASYMOD
        if (val > 1)
        {
            for (size_t j = 0; j < nprimes; j++)
            { sk_ptr[i + j * n] = coeff_modulus[j].value() - 1; }
        }
        else
        {
            for (size_t j = 0; j < nprimes; j++) { sk_ptr[i + j * n] = val; }
        }
#else
        if (val > 0)
        {
            for (size_t j = 0; j < nprimes; j++) { sk_ptr[i + j * n] = val - 1; }
        }
        else
        {
            for (size_t j = 0; j < nprimes; j++)
            { sk_ptr[i + j * n] = coeff_modulus[j].value() - 1; }
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
    auto &ct_parms    = context.first_context_data()->parms();
    auto &ct_parms_id = context.first_parms_id();
    size_t ct_nprimes = ct_parms.coeff_modulus().size();
    size_t n          = ct_parms.poly_modulus_degree();
    bool is_ntt       = ct.is_ntt_form();
    assert(is_ntt);

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
    for (size_t j = 0; j < ct_nprimes; j++)
    {
        // -- Load in two components at a time
        filepos     = poly_string_file_load(fpath, 2, ct_temp_1p, filepos);
        auto ct_ptr = get_ct_arr_ptr(ct);

        // -- Set c0 and c1 terms
        for (size_t i = 0; i < n; i++)
        {
            ct_ptr[i + j * n]                  = ct_temp_1p[i];
            ct_ptr[i + j * n + ct_nprimes * n] = ct_temp_1p[i + n];
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

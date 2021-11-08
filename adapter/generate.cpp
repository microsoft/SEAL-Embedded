// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file generate.cpp
*/

#include "generate.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>

#include "convert.h"
#include "fileops.h"
#include "seal/seal.h"
#include "utils.h"

using namespace std;
using namespace seal;
using namespace seal::util;

uint64_t endian_flip(uint64_t a)
{
    uint64_t b1 = a & 0xFF;
    uint64_t b2 = (a >> 8) & 0xFF;
    uint64_t b3 = (a >> 16) & 0xFF;
    uint64_t b4 = (a >> 24) & 0xFF;
    uint64_t b5 = (a >> 32) & 0xFF;
    uint64_t b6 = (a >> 40) & 0xFF;
    uint64_t b7 = (a >> 48) & 0xFF;
    uint64_t b8 = (a >> 56) & 0xFF;

    uint64_t a_back = (b1 << 56) | (b2 << 48) | (b3 << 40) | (b4 << 32) | (b5 << 24) | (b6 << 16) |
                      (b7 << 8) | b8;
    return a_back;
}

void gen_save_secret_key(string sk_fpath, string str_sk_fpath, string seal_sk_fpath,
                         const seal::SEALContext &context)
{
    KeyGenerator keygen(context);
    SecretKey sk1 = keygen.secret_key();
    assert(sk1.data().is_ntt_form());

    sk_bin_file_save(sk_fpath, str_sk_fpath, context, true, sk1);
    sk_seal_save(seal_sk_fpath, sk1);

    // -- Check to make sure we can read back secret key correctly.
    // -- First, generate a new secret key. This should be the same as the
    //    old secret key since it will be created from keygen's copy.
    bool incl_sp  = true;  // include special prime
    SecretKey sk2 = sk1;
    compare_sk(context, sk1, sk2, incl_sp, true);

    // -- Next, clear sk2 so we can read in values from file.
    clear_sk(context, sk2);  // overwrite internal keygen copy of sk
    compare_sk(context, sk1, sk2, incl_sp, false);

    // -- Finally, read in the secret key from file. The objects should now match.
    cout << "\nAbout to read secret key from binary file at \" " << sk_fpath << "\" ..." << endl;
    sk_bin_file_load(sk_fpath, context, sk2);
    compare_sk(context, sk1, sk2, incl_sp, true);
}

void gen_save_public_key(string dirpath, string seal_pk_fpath, string sk_fpath,
                         string seal_sk_fpath, const seal::SEALContext &context,
                         bool use_seal_sk_fpath)
{
    SecretKey sk;
    if (use_seal_sk_fpath)
        sk_seal_load(seal_sk_fpath, context, sk);
    else
    {
        auto &sk_parms      = context.key_context_data()->parms();
        auto &coeff_modulus = sk_parms.coeff_modulus();
        size_t nprimes      = coeff_modulus.size();
        size_t n            = sk_parms.poly_modulus_degree();
        sk.data().resize(mul_safe(n, nprimes));
        sk_bin_file_load(sk_fpath, context, sk);
        sk.data().parms_id() = context.key_parms_id();
    }

    KeyGenerator keygen(context, sk);

    PublicKey pk1;
    PublicKeyWrapper pk1_wr;
    keygen.create_public_key(pk1);
    pk1_wr.pk     = &pk1;
    pk1_wr.is_ntt = pk1.data().is_ntt_form();  // Must manually track this
    assert(pk1_wr.is_ntt == true);             // Should start out being true
    pk_seal_save(seal_pk_fpath, pk1);

    // -- Write the public key, prime by prime, across multiple files
    //    Make sure to write special prime to file as well.
    bool incl_sp         = true;
    bool high_byte_first = false;
    pk_bin_file_save(dirpath, context, pk1_wr, incl_sp, high_byte_first);

    // -- Check to make sure we can read back public key correctly. --

    // -- Instantiate new pk. Special prime will not be the same throughout
    PublicKey pk2;
    PublicKeyWrapper pk2_wr;
    keygen.create_public_key(pk2);  // A new public key
    pk2_wr.pk     = &pk2;
    pk2_wr.is_ntt = pk2.data().is_ntt_form();  // Must manually track this
    assert(pk2_wr.is_ntt == true);             // Should start out being true

    // -- At this point, the public keys should be different
    compare_pk(context, pk1_wr, pk2_wr, incl_sp, false);

    // -- Read in saved public key from file
    pk_bin_file_load(dirpath, context, pk2_wr, incl_sp, high_byte_first);
    compare_pk(context, pk1_wr, pk2_wr, incl_sp, true);
}

void gen_save_ifft_roots(string dirpath, const SEALContext &context, bool high_byte_first,
                         bool string_roots)
{
    size_t n = context.key_context_data()->parms().poly_modulus_degree();
    vector<complex<double>> ifft_roots(n);

    std::shared_ptr<util::ComplexRoots> croots =
        make_shared<ComplexRoots>(ComplexRoots(2 * n, MemoryPoolHandle::Global()));

    int logn          = static_cast<int>(log2(n));
    bool better_order = true;
    if (better_order)
    {
        for (size_t i = 0; i < n; i++)
        {
            ifft_roots[i] =
                conj(croots->get_root(static_cast<size_t>(reverse_bits(i - 1, logn) + 1)));
        }
    }
    else
    {
        for (size_t i = 0; i < n; i++)
        { ifft_roots[i] = conj(croots->get_root(static_cast<size_t>(reverse_bits(i, logn)))); }
    }

    string fname = dirpath + "ifft_roots_" + to_string(n) + +".dat";
    fstream file(fname.c_str(), ios::out | ios::binary | ios::trunc);

    fstream file2;
    if (string_roots)
    {
        string fname2 = dirpath + "str_ifft_roots.h";
        file2.open(fname2, ios::out | ios::trunc);
        size_t num_uint64_elements = n * 2;  // real part + imag part
        file2 << "#pragma once\n\n#include \"defines.h\"\n\n#include <stdint.h>\n\n";
        file2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                 "defined(SE_DATA_FROM_CODE_DIRECT)\n";
        file2 << "#ifdef SE_IFFT_LOAD_FULL\n";
        file2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
        file2 << "// -- IFFT roots for polynomial ring degree = " << n << "\n";
        // -- Save using uint64_t instead of double to maintain full precision
        file2 << "uint64_t ifft_roots_save[" << num_uint64_elements << "] = { ";
    }
    for (size_t i = 0; i < n; i++)
    {
        // -- Debugging info
        // cout << "ifft_roots[" << i << "]: " << "real: " << real(ifft_roots[i]);
        // cout << " imag: " << imag(ifft_roots[i]) << endl;

        for (size_t k = 0; k < 2; k++)
        {
            double data_d = (!k) ? real(ifft_roots[i]) : imag(ifft_roots[i]);
            uint64_t data = *((uint64_t *)(&data_d));  // we can only shift an int type

            // -- Debugging info
            // if (i < 10) cout << "data_d: " << data_d << endl;
            // if (i < 5) cout << "data  : " << std::hex << data << std::dec << endl;

            for (size_t j = 0; j < 8; j++)  // write one byte at a time
            {
                size_t shift_amt = (high_byte_first) ? 7 - j : j;
                uint8_t byte     = (data >> (8 * shift_amt)) & 0xFF;
                file.put(static_cast<char>(byte));
            }
            if (string_roots)
            {
                string next_str = (((i + 1) < n) || !k) ? ", " : "};\n";
                uint64_t data_s = high_byte_first ? endian_flip(data) : data;
                file2 << "0x" << std::hex << data_s << std::dec << next_str;
                if (!(i % 64) && i && k) file2 << "\n";
            }
        }
    }
    file.close();
    if (string_roots)
    {
        file2 << "\n#endif\n#endif" << endl;
        file2.close();
    }
}

void gen_save_ntt_roots_header(string dirpath, const SEALContext &context, bool inverse)
{
    auto &key_parms = context.key_context_data()->parms();
    size_t nprimes  = key_parms.coeff_modulus().size();
    size_t n        = key_parms.poly_modulus_degree();

    string ntt_str      = inverse ? "intt" : "ntt";
    string ntt_str_caps = inverse ? "INTT" : "NTT";

    string fpath = dirpath + "str_" + ntt_str + "_roots_addr_array.h";
    fstream file(fpath.c_str(), ios::out | ios::trunc);

    // -- No need to include the special prime for the headers
    bool incl_sp_sf            = (nprimes == 1) ? true : false;
    size_t string_file_nprimes = incl_sp_sf ? nprimes : nprimes - 1;

    file << "#pragma once\n\n#include \"defines.h\"\n\n";
    file << "#if defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT)\n\n";
    file << "#include <stdint.h>\n\n";

    for (size_t outer = 0; outer < 2; outer++)
    {
        if (outer == 0)
            file << "#ifdef SE_" << ntt_str_caps << "_REG\n";
        else
            file << "#elif defined(SE_" << ntt_str_caps << "_FAST)\n";
        for (size_t t = 0; t < string_file_nprimes; t++)
        {
            auto q = context.key_context_data()->parms().coeff_modulus()[t].value();
            assert(log2(q) <= 30);
            string fpath = "str_" + ntt_str;
            if (outer == 1) fpath += "_fast";
            fpath += "_roots_" + to_string(n) + "_" + to_string(q) + ".h";
            cout << "writing to file: " << dirpath + fpath << endl;
            file << "   #include \"" << fpath << "\"" << endl;
        }
        if (outer == 1) file << "#endif\n";
    }
    file << "\nZZ* " << ntt_str << "_roots_addr[" << string_file_nprimes << "] =\n{\n";

    // -- No need to include special prime in string files
    for (size_t t = 0; t < string_file_nprimes; t++)
    {
        file << "  &(((ZZ*)(" << ntt_str << "_roots_save_prime" << to_string(t) << "))[0])";
        if (t == string_file_nprimes - 1)
            file << "\n};" << endl;
        else
            file << "," << endl;
    }
    file << "\n#endif\n";
    file.close();
}

void gen_save_ntt_roots(string dirpath, const SEALContext &context, bool lazy, bool inverse,
                        bool high_byte_first, bool string_roots)
{
    // -- Note: SEAL stores the NTT tables in bit-rev form
    auto ntt_tables_ptr = context.key_context_data()->small_ntt_tables();
    auto &key_parms     = context.key_context_data()->parms();
    size_t n            = key_parms.poly_modulus_degree();
    size_t nprimes      = key_parms.coeff_modulus().size();

    const bool large_primes = false;

    string ntt_str      = inverse ? "intt" : "ntt";
    string ntt_str_caps = inverse ? "INTT" : "NTT";

    for (size_t t = 0; t < nprimes; t++)
    {
        const NTTTables *tables = &(ntt_tables_ptr[t]);
        assert(tables);
        Modulus modulus = tables->modulus();

        auto q             = modulus.value();
        bool large_modulus = (log2(q) > 32) ? true : false;

        assert(((t <= (nprimes - 1)) && (log2(q) <= 30)) ||
               ((t == nprimes - 1) && (log2(q) <= 64) && (nprimes != 1)));

        /*
        SEAL-Embedded needs the following constant values related to the NTT/INTT
        - If NTT compute type is "on-the-fly" or "one-shot":
            >     w = Value of the first power of the NTT root, for each prime qi
        - If INTT compute type is "on-the-fly" or "one-shot" (and on-device testing is desired):
            > inv_w = Value of the first power of the INTT root, for each prime qi
                - This should be equal to w^(-1) mod qi
        - If INTT compute type is non-fast (and on-device testing is desired):
            > inv_n       = n^(-1) mod qi
            > last_inv_sn = (last_inv_s * inv_n) mod qi
                - Where last_inv_s is the last inverse NTT root power used in an INTT loop
                 (i.e., is last power stored in inv_root_powers)
        */
        auto logn = seal::util::get_power_of_two(n);
        cout << "logn: " << logn << endl;
        auto bit_rev_1               = seal::util::reverse_bits((uint64_t)1, logn);
        MultiplyUIntModOperand w     = tables->get_from_root_powers(bit_rev_1);
        MultiplyUIntModOperand inv_w = tables->get_from_inv_root_powers(1);
        {
            uint64_t inv_w_check;
            if (!try_invert_uint_mod(w.operand, modulus, inv_w_check))
                throw invalid_argument("invalid modulus");
            if (inv_w.operand != inv_w_check)
            {
                cout << "inv_w_check:   " << inv_w_check << endl;
                cout << "inv_w.operand: " << inv_w.operand << endl;
                throw;
            }
        }
        MultiplyUIntModOperand inv_n      = tables->inv_degree_modulo();  // n^(-1) mod qi
        MultiplyUIntModOperand last_inv_s = tables->get_from_inv_root_powers(n - 1);
        MultiplyUIntModOperand last_inv_sn;
        last_inv_sn.set(multiply_uint_mod(inv_n.operand, last_inv_s, modulus), modulus);
        {
            // -- Check
            uint64_t last_ii_s;
            if (!try_invert_uint_mod(last_inv_s.operand, modulus, last_ii_s))
                throw invalid_argument("invalid modulus");

            uint64_t last_inv_sn_check;
            auto n_last_ii_s = multiply_uint_mod(n, last_ii_s, modulus);
            if (!try_invert_uint_mod(n_last_ii_s, modulus, last_inv_sn_check))
                throw invalid_argument("invalid modulus");
            if (last_inv_sn.operand != last_inv_sn_check)
            {
                cout << "last_ii_s:           " << last_ii_s << endl;
                cout << "n_last_ii_s:         " << n_last_ii_s << endl;
                cout << "last_inv_sn_check:   " << last_inv_sn_check << endl;
                cout << "last_inv_sn.operand: " << last_inv_sn.operand << endl;
                throw;
            }
        }

        // -- Debugging / For filling in SEAL-Embedded constants
        cout << "\n--- Printing constants for n = " << n;
        cout << ", q = " << q << " ---\n" << endl;
        cout << "\t(w = first power of NTT root)" << endl;
        cout << "\t w.operand            : " << w.operand << endl;
        cout << "\t w.quotient           : " << upper32(w.quotient) << " (small) = ";
        cout << w.quotient << " (large)\n" << endl;
        cout << "\t(inv_w = w^(-1) mod qi = first power of INTT root)" << endl;
        cout << "\t inv_w.operand        : " << inv_w.operand << endl;
        cout << "\t inv_w.quotient       : " << upper32(inv_w.quotient) << " (small) = ";
        cout << inv_w.quotient << " (large)\n" << endl;
        cout << "\t(inv_n = n^(-1) mod qi)" << endl;
        cout << "\t inv_n.operand        : " << inv_n.operand << endl;
        cout << "\t inv_n.quotient       : " << upper32(inv_n.quotient) << " (small) = ";
        cout << inv_n.quotient << " (large)\n" << endl;
        cout << "\t(last_inv_sn = (last_inv_s * inv_n)  mod qi)" << endl;
        cout << "\t last_inv_sn.operand  : " << last_inv_sn.operand << endl;
        cout << "\t last_inv_sn.quotient : " << upper32(last_inv_sn.quotient) << " (small) = ";
        cout << last_inv_sn.quotient << " (large)\n\n" << endl;

        // -- Create file
        string fname = dirpath + ntt_str;
        if (lazy) fname += "_fast";
        fname += "_roots_" + to_string(n) + "_" + to_string(q) + ".dat";
        fstream file(fname.c_str(), ios::out | ios::binary | ios::trunc);
        cout << "Writing to " << fname << endl;

        fstream file2;
        if (string_roots)
        {
            string fname2 = dirpath + "str_" + ntt_str;
            if (lazy) fname2 += "_fast";
            fname2 += "_roots_" + to_string(n) + "_" + to_string(q) + ".h";
            file2.open(fname2, ios::out | ios::trunc);
            size_t num_elements = lazy ? n * 2 : n;  // real part + imag part
            file2 << "#pragma once\n\n#include \"defines.h\"\n\n";
            file2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                     "defined(SE_DATA_FROM_CODE_DIRECT)\n";
            file2 << "#include <stdint.h>\n\n";  // uint32_t
            file2 << "#ifdef SE_" << ntt_str_caps;
            if (lazy)
                file2 << "_FAST\n";
            else
                file2 << "_REG\n";
            if (large_modulus)
            {
                file2 << "// -- Note: This file uses >30-bit primes and cannot";
                file2 << " be used with the SEAL-Embedded device library." << endl;
            }
            file2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
            if (large_modulus)
                file2 << "uint64_t ";
            else
                file2 << "ZZ ";
            file2 << ntt_str << "_";
            file2 << "roots_save_prime" << to_string(t);
            file2 << "[" << num_elements << "] = { ";
        }

        for (size_t i = 0; i < n; i++)
        {
            // -- Quotient stores floor(operand * 2^64 / q).
            //    If SEAL-Embedded is using uint32_t datatypes, we actually
            //    need floor(operand * 2^32 / q). This happens to be
            //    floor(quotient / 2^32), i.e. the upper 32-bits of quotient.
            //    (In this case, operand = root and quotient = root_prime.)
            const MultiplyUIntModOperand w =
                (!inverse) ? tables->get_from_root_powers(i) : tables->get_from_inv_root_powers(i);

            size_t logn = seal::util::get_power_of_two(n);
            assert(logn != -1);
            size_t actual_idx = reverse_bits(i, logn);

            // -- Debugging / For filling in SEAL-Embedded constants
            if (actual_idx == 1)
            {
                if (inverse) cout << "inverse_";
                cout << "root[" << i << "]: operand = " << w.operand << " , quotient = ";
                if (large_modulus)
                    cout << w.quotient << endl;
                else
                    cout << upper32(w.quotient) << endl;
            }

            for (size_t k = 0; k < 1 + static_cast<size_t>(lazy); k++)
            {
                uint64_t data = (!k) ? w.operand : w.quotient;
                if (k && !large_modulus) { data = static_cast<uint64_t>(upper32(data)); }
                size_t primesize = large_modulus ? 8 : 4;  // size in bytes
                for (size_t j = 0; j < primesize; j++)     // write one byte at a time
                {
                    size_t shift_amt = high_byte_first ? primesize - 1 - j : j;
                    uint8_t byte     = (data >> (8 * shift_amt)) & 0xFF;
                    file.put(static_cast<char>(byte));
                }
                if (string_roots)
                {
                    string next_str  = (((i + 1) < n) || (!k && lazy)) ? ", " : "};\n";
                    string ulong_str = large_modulus ? "ULL" : "";
                    uint64_t data_s  = high_byte_first ? endian_flip(data) : data;
                    file2 << to_string(data_s) + ulong_str << next_str;
                    if (!(i % 64) && i && !k) file2 << "\n";
                }
            }
        }
        file.close();
        if (string_roots)
        {
            file2 << "\n#endif\n#endif\n" << endl;
            file2.close();
        }
    }
    gen_save_ntt_roots_header(dirpath, context, inverse);
}

/**
Helper function for gen_save_index_map. Generates the bit-reversed value of the input.
Requires numbits to be at most 16.

@param[in] input    Value to be bit-reversed
@param[in] numbits  Number of bits to reverse
*/
static size_t bit_rev_16bits(size_t input, size_t numbits)
{
    assert(numbits <= 16);
    size_t t = (((input & 0xaaaa) >> 1) | ((input & 0x5555) << 1));
    t        = (((t & 0xcccc) >> 2) | ((t & 0x3333) << 2));
    t        = (((t & 0xf0f0) >> 4) | ((t & 0x0f0f) << 4));
    t        = (((t & 0xff00) >> 8) | ((t & 0x00ff) << 8));
    return numbits == 0 ? 0 : t >> (16 - (size_t)(numbits));
}

void gen_save_index_map(string dirpath, const SEALContext &context, bool high_byte_first)
{
    auto &key_parms   = context.key_context_data()->parms();
    size_t n          = key_parms.poly_modulus_degree();
    uint64_t m        = (uint64_t)n * 2;               // m = 2n
    size_t slot_count = n / 2;                         // slot_count = n/2
    size_t logn       = static_cast<size_t>(log2(n));  // number of bits to represent n
    assert(logn <= 16);

    // -- 3 generates a multiplicative group mod 2^n with order n/2
    // (Note: 5 would work as well, but 3 matches SEAL and SEAL-Embedded)
    uint64_t gen = 3;
    uint64_t pos = 1;

    uint16_t *index_map = (uint16_t *)calloc(n, sizeof(uint16_t));
    for (size_t i = 0; i < n / 2; i++)
    {
        // -- We want index1 + index2 to equal n-1
        size_t index1 = ((size_t)pos - 1) / 2;
        size_t index2 = n - index1 - 1;

        assert(index1 <= 0xFFFF && index2 <= 0xFFFF);  // less than max uint16_t

        // -- Merge index mapping step w/ bitrev step req. for later application of
        //    IFFT/NTT
        index_map[i]              = (uint16_t)bit_rev_16bits(index1, logn);
        index_map[i + slot_count] = (uint16_t)bit_rev_16bits(index2, logn);

        // -- Next root
        pos *= gen;

        // -- Since m is a power of 2, m-1 sets all bits less significant than the '1'
        //    bit in the value m. A bit-wise 'and' with (m-1) is therefore essentially
        //    a reduction moduluo m. Ex:  m = 4 = 0100, m-1 = 3 = 0011 --> if pos = 21
        //    = 10101: 21 % 4 = 1 = 10101 & 0011
        pos &= (m - 1);
    }

    // -- Save to binary file
    {
        string fname = dirpath + "index_map_" + to_string(n) + ".dat";
        fstream file(fname.c_str(), ios::out | ios::binary | ios::trunc);
        cout << "Writing to " << fname << endl;
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < 2; j++)  // write one byte at a time
            {
                size_t shift_amt = (high_byte_first) ? 1 - j : j;
                uint8_t byte     = (index_map[i] >> (8 * shift_amt)) & 0xFF;
                file.put(static_cast<char>(byte));
            }
        }
        file.close();
    }

    // -- Hard-code in code file
    {
        string fname = dirpath + "str_index_map.h";
        fstream file(fname.c_str(), ios::out | ios::binary | ios::trunc);
        cout << "Writing to " << fname << endl;

        file << "#pragma once\n\n#include \"defines.h\"\n\n";
        file << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                "defined(SE_DATA_FROM_CODE_DIRECT)\n";
        file << "#if defined(SE_INDEX_MAP_LOAD) || "
                "defined(SE_INDEX_MAP_LOAD_PERSIST) || "
                "defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)\n";
        file << "#include <stdint.h>\n\n";  // uint64_t ?
        file << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
        file << "// -- index map indices for polynomial ring degree = " << n << "\n";
        file << "uint32_t index_map_store[" << n / 2 << "] = { ";
        uint32_t *index_map_str_save = (uint32_t *)index_map;

        size_t i_stop = n / 2;  // half since we are storing as uint32 instead of uint16
        for (size_t i = 0; i < i_stop; i++)
        {
            string next_str = ((i + 1) < i_stop) ? ", " : "};\n";
            file << "0x" << std::hex << index_map_str_save[i] << next_str;
            if (!(i % 13)) file << "\n";
        }
        file << "\n#endif\n#endif" << endl;
        file.close();
    }
    if (index_map)
    {
        free(index_map);
        index_map = 0;
    }
}

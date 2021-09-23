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

void gen_save_secret_key(string sk_fpath, string str_sk_fpath, string seal_sk_fpath,
                         const SEALContext &context)
{
    KeyGenerator keygen(context);
    SecretKey sk1 = keygen.secret_key();
    assert(sk1.data().is_ntt_form());

    sk_bin_file_save(sk_fpath, str_sk_fpath, context, true, sk1);
    sk_seal_save(seal_sk_fpath, sk1);

    // -- Check to make sure we can read back secret key correctly.
    // -- First, generate a new secret key. This should be the same as the
    //    old secret key since it will be created from keygen's copy.
    bool incl_sp  = 1;  // include special prime
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
                         string seal_sk_fpath, const SEALContext &context, bool use_seal_sk_fpath)
{
    SecretKey sk;
    if (use_seal_sk_fpath)
        sk_seal_load(seal_sk_fpath, context, sk);
    else
    {
        auto &sk_parms            = context.key_context_data()->parms();
        auto &coeff_modulus       = sk_parms.coeff_modulus();
        size_t coeff_modulus_size = coeff_modulus.size();
        size_t n                  = sk_parms.poly_modulus_degree();
        sk.data().resize(mul_safe(n, coeff_modulus_size));
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
    bool incl_sp = 1;
    pk_bin_file_save(dirpath, context, pk1_wr, incl_sp);

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
    pk_bin_file_load(dirpath, context, pk2_wr, incl_sp);
    compare_pk(context, pk1_wr, pk2_wr, incl_sp, true);
}

void gen_save_ifft_roots(string dirpath, const SEALContext &context, bool high_byte_first,
                         bool string_roots)
{
    size_t n = context.key_context_data()->parms().poly_modulus_degree();
    vector<complex<double>> ifft_roots(n);

    shared_ptr<util::ComplexRoots> complex_roots;
    complex_roots = make_shared<ComplexRoots>(ComplexRoots(2 * n, MemoryPoolHandle::Global()));

    int logn          = static_cast<int>(log2(n));
    bool better_order = 1;
    if (better_order)
    {
        for (size_t i = 0; i < n; i++)
        {
            ifft_roots[i] =
                conj(complex_roots->get_root(static_cast<size_t>(reverse_bits(i - 1, logn) + 1)));
        }
    }
    else
    {
        for (size_t i = 0; i < n; i++)
        {
            ifft_roots[i] =
                conj(complex_roots->get_root(static_cast<size_t>(reverse_bits(i, logn))));
        }
    }

    string fname = dirpath + "ifft_roots_" + to_string(n) + +".dat";
    fstream outfile(fname.c_str(), ios::out | ios::binary | ios::trunc);

    fstream outfile2;
    if (string_roots)
    {
        string fname2 = dirpath + "str_ifft_roots.h";
        // string fname2 = dirpath + "str_ifft_roots_" + to_string(n) + ".h";
        outfile2.open(fname2, ios::out | ios::trunc);
        size_t num_uint64_elements = n * 2;  // real part + imag part
        outfile2 << "#pragma once\n\n#include \"defines.h\"\n\n#include <stdint.h>\n\n";
        outfile2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                    "defined(SE_DATA_FROM_CODE_DIRECT)\n";
        outfile2 << "#ifdef SE_IFFT_LOAD_FULL\n";
        outfile2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
        outfile2 << "// -- IFFT roots for polynomial ring degree = " << n << "\n";
        outfile2 << "uint64_t ifft_roots_save[" << num_uint64_elements << "] = { ";
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
            // if (i < 5) cout << "data_d: " << data_d << endl;
            // if (i < 5) cout << "data  : " << data << endl;

            for (size_t j = 0; j < 8; j++)  // write one byte at a time
            {
                size_t shift_amt = (high_byte_first) ? 7 - j : j;
                uint8_t byte     = (data >> (8 * shift_amt)) & 0xFF;
                outfile.put(static_cast<char>(byte));
            }
            if (string_roots)
            {
                // cout << "data string: " << to_string(data) << endl;
                string next_str = (((i + 1) < n) || !k) ? ", " : "};\n";
                outfile2 << to_string(data) + "ULL" << next_str;
                if (!(i % 64) && i && k) outfile2 << "\n";
            }
        }
    }
    outfile.close();
    if (string_roots)
    {
        outfile2 << "\n#endif\n#endif" << endl;
        outfile2.close();
    }
}

void gen_save_ntt_roots_header(string dirpath, const SEALContext &context, bool inverse)
{
    auto &key_parms           = context.key_context_data()->parms();
    size_t n                  = key_parms.poly_modulus_degree();
    size_t coeff_modulus_size = key_parms.coeff_modulus().size();
    assert(coeff_modulus_size >= 2);

    string fpath = dirpath + "str_";
    if (inverse) fpath += "i";
    fpath += "ntt_roots_addr_array.h";
    fstream outfile(fpath.c_str(), ios::out | ios::trunc);

    bool incl_sp_sf = false;  // No need to include the special prime for the headers

    // const bool large_primes = false; // Assume this

    outfile << "#pragma once\n\n#include \"defines.h\"\n\n";
    outfile << "#if defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT)\n\n";
    outfile << "#include <stdint.h>\n\n";

    stringstream pk_addr_str;

    size_t string_file_coeff_modulus_size =
        incl_sp_sf ? coeff_modulus_size : coeff_modulus_size - 1;

    for (size_t outer = 0; outer < 2; outer++)
    {
        if (inverse)
        {
            if (outer == 0)
                outfile << "#ifdef SE_INTT_REG\n";
            else
                outfile << "#elif defined(SE_INTT_FAST)\n";
        }
        else
        {
            if (outer == 0)
                outfile << "#ifdef SE_NTT_REG\n";
            else
                outfile << "#elif defined(SE_NTT_FAST)\n";
        }
        for (size_t t = 0; t < string_file_coeff_modulus_size; t++)
        {
            string fpath_common = "roots_";
            auto coeff_modulus  = context.key_context_data()->parms().coeff_modulus()[t].value();
            fpath_common += to_string(n) + "_" + to_string(coeff_modulus);

            string fpath = dirpath + "str_";
            if (inverse) fpath += "i";
            fpath += "ntt" + fpath_common + ".h";
            cout << "writing to file: " << fpath << endl;

            outfile << "   #include \"str_";
            if (inverse) outfile << "i";
            outfile << "ntt_";
            if (outer) outfile << "fast_";
            outfile << fpath_common + ".h\"" << endl;
        }
        if (outer == 1) outfile << "#endif\n";
    }

    outfile << "\nZZ* ";
    if (inverse) outfile << "i";
    outfile << "ntt_roots_addr[" << string_file_coeff_modulus_size << "] =\n{\n";

    // -- No need to include special prime in string files
    for (size_t t = 0; t < string_file_coeff_modulus_size; t++)
    {
        outfile << "  &(((ZZ*)(";
        if (inverse) outfile << "i";
        outfile << "ntt_roots_save_prime" << to_string(t) << "))[0])";
        if (t == string_file_coeff_modulus_size - 1)
            outfile << "\n};" << endl;
        else
            outfile << "," << endl;
    }
    outfile << "\n#endif\n";
    outfile.close();
}

void gen_save_ntt_roots(string dirpath, const SEALContext &context, bool lazy, bool inverse,
                        bool high_byte_first, bool string_roots)
{
    auto ntt_tables_ptr = context.key_context_data()->small_ntt_tables();
    auto &key_parms     = context.key_context_data()->parms();
    size_t n            = key_parms.poly_modulus_degree();

    const bool large_primes = false;
    for (size_t mod_loop = 0; mod_loop < key_parms.coeff_modulus().size(); mod_loop++)
    {
        const NTTTables *tables            = &(ntt_tables_ptr[mod_loop]);
        Modulus modulus                    = tables->modulus();
        MultiplyUIntModOperand inv_n       = tables->inv_degree_modulo();
        MultiplyUIntModOperand w_n_minus_1 = tables->get_from_inv_root_powers(n - 1);
        MultiplyUIntModOperand inv_n_w;
        inv_n_w.set(multiply_uint_mod(inv_n.operand, w_n_minus_1, modulus), modulus);

        // -- Debugging
        cout << "inv_n.operand  : " << inv_n.operand << endl;
        cout << "inv_n.quotient : large: " << inv_n.quotient;
        cout << " small: " << upper32(inv_n.quotient) << endl;
        cout << "inv_n_w.operand: " << inv_n_w.operand << endl;
        cout << "inv_n.quotient : large: " << inv_n_w.quotient;
        cout << " small: " << upper32(inv_n_w.quotient) << endl;

        // -- Create file
        string fname = dirpath;
        fname += (!inverse) ? "ntt" : "intt";
        // if (lazy) fname += "_lazy";
        if (lazy) fname += "_fast";
        fname += "_roots_" + to_string(n) + "_" + to_string(modulus.value()) + ".dat";
        fstream outfile(fname.c_str(), ios::out | ios::binary | ios::trunc);
        cout << "Writing to " << fname << endl;

        fstream outfile2;
        if (string_roots)
        {
            string fname2 = dirpath;
            fname2 += (!inverse) ? "str_ntt" : "str_intt";
            // if (lazy) fname2 += "_lazy";
            if (lazy) fname2 += "_fast";
            fname2 += "_roots_" + to_string(n) + "_" + to_string(modulus.value()) + ".h";
            outfile2.open(fname2, ios::out | ios::trunc);
            size_t num_elements = lazy ? n * 2 : n;  // real part + imag part
            outfile2 << "#pragma once\n\n#include \"defines.h\"\n\n";
            outfile2 << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                        "defined(SE_DATA_FROM_CODE_DIRECT)\n";
            outfile2 << "#include <stdint.h>\n\n";  // uint32_t
            if (lazy)
            {
                if (inverse)
                    outfile2 << "#ifdef SE_INTT_FAST\n";
                else
                    outfile2 << "#ifdef SE_NTT_FAST\n";
            }
            else
            {
                if (inverse)
                    outfile2 << "#ifdef SE_INTT_REG\n";
                else
                    outfile2 << "#ifdef SE_NTT_REG\n";
            }
            outfile2 << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
            // outfile2 << "uint64_t ";
            outfile2 << "ZZ ";
            if (inverse) outfile2 << "i";
            outfile2 << "ntt_";
            // if (lazy) outfile2 << "fast_";
            outfile2 << "roots_save_prime" << to_string(mod_loop);
            outfile2 << "[" << num_elements << "] = { ";
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

            // -- Debugging info
            if (i < 5)
            {
                cout << "root[" << i << "]: operand = " << w.operand << " , quotient = ";
                if (large_primes)
                    cout << w.quotient << endl;
                else
                    cout << upper32(w.quotient) << endl;
            }

            for (size_t k = 0; k < 1 + static_cast<size_t>(lazy); k++)
            {
                uint64_t data = (!k) ? w.operand : w.quotient;
                if (k && !large_primes) { data = static_cast<uint64_t>(upper32(data)); }
                size_t primesize = large_primes ? 8 : 4;  // size in bytes
                for (size_t j = 0; j < primesize; j++)    // write one byte at a time
                {
                    size_t shift_amt = high_byte_first ? primesize - 1 - j : j;
                    uint8_t byte     = (data >> (8 * shift_amt)) & 0xFF;
                    outfile.put(static_cast<char>(byte));
                }
                if (string_roots)
                {
                    string next_str  = (((i + 1) < n) || (!k && lazy)) ? ", " : "};\n";
                    string ulong_str = large_primes ? "ULL" : "";
                    outfile2 << to_string(data) + ulong_str << next_str;
                    if (!(i % 64) && i && !k) outfile2 << "\n";
                }
            }
        }
        outfile.close();
        if (string_roots)
        {
            outfile2 << "\n#endif\n#endif\n" << endl;
            outfile2.close();
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
static size_t index_map_bit_rev(size_t input, size_t numbits)
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

    // -- If n > 16384, cannot use uint16_t
    assert(n < 16384);

    // -- 3 generates a multiplicative group mod 2^n with order n/2
    // (Note: 5 would work as well, but 3 matches SEAL and SEAL-Embedded)
    uint64_t gen = 3;
    // uint64_t gen = 5;
    uint64_t pos = 1;

    uint16_t *index_map = (uint16_t *)calloc(n, sizeof(uint16_t));
    for (size_t i = 0; i < n / 2; i++)
    {
        // -- We want index1 + index2 to equal n-1
        size_t index1 = ((size_t)pos - 1) / 2;
        size_t index2 = n - index1 - 1;

        // -- Merge index mapping step w/ bitrev step req. for later application of
        // IFFT/NTT
        index_map[i]              = (uint16_t)index_map_bit_rev(index1, logn);
        index_map[i + slot_count] = (uint16_t)index_map_bit_rev(index2, logn);

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
        fstream outfile(fname.c_str(), ios::out | ios::binary | ios::trunc);
        cout << "Writing to " << fname << endl;
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < 2; j++)  // write one byte at a time
            {
                size_t shift_amt = (high_byte_first) ? 1 - j : j;
                uint8_t byte     = (index_map[i] >> (8 * shift_amt)) & 0xFF;
                outfile.put(static_cast<char>(byte));
            }
        }
        outfile.close();
    }

    // -- Hard-code in code file
    {
        string fname = dirpath + "str_index_map.h";
        fstream outfile(fname.c_str(), ios::out | ios::binary | ios::trunc);
        cout << "Writing to " << fname << endl;

        outfile << "#pragma once\n\n#include \"defines.h\"\n\n";
        outfile << "#if defined(SE_DATA_FROM_CODE_COPY) || "
                   "defined(SE_DATA_FROM_CODE_DIRECT)\n";
        outfile << "#if defined(SE_INDEX_MAP_LOAD) || "
                   "defined(SE_INDEX_MAP_LOAD_PERSIST)\n";
        outfile << "#include <stdint.h>\n\n";  // uint64_t ?
        outfile << "#ifdef SE_DATA_FROM_CODE_COPY\nconst\n#endif" << endl;
        outfile << "// -- index map indices for polynomial ring degree = " << n << "\n";
        outfile << "uint32_t index_map_store[" << n / 2 << "] = { ";
        uint32_t *index_map_str_save = (uint32_t *)index_map;

        size_t i_stop = n / 2;  // half since we are storing as uint32 instead of uint16
        for (size_t i = 0; i < i_stop; i++)
        {
            string next_str = ((i + 1) < i_stop) ? ", " : "};\n";
            outfile << "0x" << hex << index_map[i] << next_str;
            if (!(i % 13)) outfile << "\n";
        }
        outfile << "\n#endif\n#endif" << endl;
        outfile.close();
    }
    free(index_map);
}

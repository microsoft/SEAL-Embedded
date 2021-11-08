// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file adapter.cpp
*/

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

#include "config.h"
#include "convert.h"
#include "fileops.h"
#include "generate.h"
#include "seal/seal.h"
#include "utils.h"

using namespace std;
using namespace seal;
using namespace seal::util;

// -- Set this to the path to directory to save keys, indices, and roots
static string save_dir_path = string(SE_ADAPTER_FILE_OUTPUT_DIR) + "/adapter_output_data/";
// -- Set these to the paths to the output of running seal-embedded
//    using api calls (see /device/test/api_tests.c for an example)
static string ct_str_file_path_asym = string(SE_ADAPTER_FILE_OUTPUT_DIR) + "/out_asym_api_tests";
static string ct_str_file_path_sym  = string(SE_ADAPTER_FILE_OUTPUT_DIR) + "/out_sym_api_tests";

void verify_ciphertexts(string dirpath, double scale, size_t degree, seal::SEALContext &context,
                        bool symm_enc, string ct_str_file_path, string sk_binfilename = "")
{
    auto &parms = context.key_context_data()->parms();
    size_t n    = parms.poly_modulus_degree();

    size_t print_size = 8;
    assert(print_size <= n);

    // -- We don't need to generate any new keys, but we need the format
    //    to be compatible with SEAL. Use keygen to create the secret key
    //    object, then load the value of our particular secret key.
    KeyGenerator keygen(context);
    Evaluator evaluator(context);
    CKKSEncoder encoder(context);
    size_t slot_count = encoder.slot_count();

    cout << "\nNumber of slots: " << slot_count << "\n" << endl;

    SecretKey sk = keygen.secret_key();
    if (sk_binfilename.size() == 0) sk_binfilename = dirpath + "sk_" + to_string(degree) + ".dat";
    sk_bin_file_load(sk_binfilename, context, sk);
    Decryptor decryptor(context, sk);

    Ciphertext ct;
    PublicKey pk;
    {
        // -- Set up ciphertext
        vector<double> test = {1.0, 2.0, 3.0};
        Plaintext pt;
        encoder.encode(test, scale, pt);
        if (symm_enc)
        {
            Encryptor encryptor(context, sk);
            encryptor.encrypt_symmetric(pt, ct);
        }
        else
        {
            keygen.create_public_key(pk);
            PublicKeyWrapper pk_wr;
            pk_wr.pk     = &pk;
            pk_wr.is_ntt = pk.data().is_ntt_form();

            // -- We know this should start out being true
            assert(pk_wr.is_ntt == true);

            bool incl_sp         = 1;  // Need to read back the special prime
            bool high_byte_first = 0;
            pk_bin_file_load(dirpath, context, pk_wr, incl_sp, high_byte_first);
            print_pk("pk", pk_wr, 8, incl_sp);

            pk_to_non_ntt_form(context, pk_wr);
            print_pk("pk", pk_wr, 8, incl_sp);

            pk_to_ntt_form(context, pk_wr);
            print_pk("pk", pk_wr, 8, incl_sp);

            Encryptor encryptor(context, pk);
            encryptor.encrypt(pt, ct);
        }
    }

    streampos filepos = 0;
    size_t nfailures  = 0;
    try
    {
        size_t ntest_stop = 9;
        for (size_t ntest = 0; ntest < ntest_stop; ntest++)
        {
            cout << "---------------------------------------------" << endl;
            cout << "            Test # " << ntest << endl;
            cout << "---------------------------------------------" << endl;

            // -- Find the expected message from the string file. Comment out if not needed.
            vector<double> values_orig(slot_count, 0);
            cout << "Reading values from file..." << endl;
            filepos = poly_string_file_load(ct_str_file_path, 1, values_orig, filepos);

            // -- Uncomment to check against expected plaintext output (assuming
            //    corresponding lines in seal_embedded.c are also uncommented)
            // vector<int64_t> plaintext_vals(slot_count*2, 0);
            // cout << "Reading plaintext values from file..." << endl;
            // filepos = poly_string_file_load(ct_str_file_path, 1, plaintext_vals, filepos);
            // print_poly("\npt         ", plaintext_vals, print_size);

            // -- Uncomment to check against expected plaintext output (assuming
            //    corresponding lines in seal_embedded.c are also uncommented)
            // vector<int64_t> plaintext_error_vals(slot_count*2, 0);
            // cout << "Reading plaintext error values from file..." << endl;
            // filepos = poly_string_file_load(ct_str_file_path, 1, plaintext_error_vals, filepos);
            // print_poly("\npte        ", plaintext_error_vals, print_size);

            // -- Read in the ciphertext from the string file
            cout << "Reading ciphertexts from file..." << endl;
            filepos = ct_string_file_load(ct_str_file_path, context, evaluator, ct, filepos);
            cout << "encrypted size: " << ct.size() << endl;

            // -- Decrypt and decode the ciphertext
            Plaintext pt_d;
            decryptor.decrypt(ct, pt_d);
            print_poly("\n(ntt) pt_d       ", pt_d.data(), print_size);
            pt_to_non_ntt_form(context, pt_d);
            print_poly("\n      pt_d  ", pt_d.data(), print_size);
            pt_to_ntt_form(context, pt_d);

            vector<double> msg_d(slot_count, 0);
            encoder.decode(pt_d, msg_d);

            print_poly("\nmsg_d      ", msg_d, print_size);
            cout << endl;

            // -- Uncomment to check against expected plaintext output (assuming
            //    corresponding lines in seal_embedded.c are also uncommented)
            // int precision = 4;
            // print_poly("values_orig", values_orig, print_size, precision);
            // bool are_equal = are_equal_poly(msg_d, values_orig, slot_count);
            // assert(are_equal);
            // if (!are_equal) nfailures++;

            // -- Debugging
            // Plaintext pt_test;
            // encoder.encode(values_orig, scale, pt_test);
            // print_poly("pt_test", get_pt_arr_ptr(pt_test), print_size);
        }
    } catch (...)
    {
        cout << "In adapter, verify_ciphertexts. ";
        cout << "Something went wrong or end of file reached!" << endl;
        throw;
    }

    cout << "Done running tests.";
    if (nfailures) { cout << " " << nfailures << " tests did not pass." << endl; }
    else
    {
        cout << " All tests passed!! :) :)" << endl;
    }
}

int main(int argc, char *argv[])
{
    // -- Instructions: Uncomment one of the below degrees and run
    //    or specify degree as a command line argument
    // size_t degree = 1024;
    // size_t degree = 2048;
    size_t degree = 4096;
    // size_t degree = 8192;
    // size_t degree = 16384;

    if (argc > 1)
    {
        std::istringstream ss(argv[1]);
        size_t degree_in;
        if (!(ss >> degree_in)) { std::cerr << "Invalid number: " << argv[1] << '\n'; }
        else if (!ss.eof())
        {
            std::cerr << "Trailing characters after number: " << argv[1] << '\n';
        }
        assert(degree_in == 1024 || degree_in == 2048 || degree_in == 4096 || degree_in == 8192 ||
               degree_in == 16384);
        degree = degree_in;
    }

    double scale = pow(2, 25);
    cout << "Parameters: degree " << degree << ", ntt_form, prime bit-lengths: {";
    switch (degree)
    {
        case 1024:
            cout << "27}, scale = pow(2, 20)" << endl;
            scale = pow(2, 20);
            break;
        case 2048:
            cout << "27, 27}, scale = pow(2, 25)" << endl;
            scale = pow(2, 25);
            break;
#ifdef SEALE_DEFAULT_4K_27BIT
        case 4096:
            cout << "27, 27, 27, 28}, scale = pow(2, 20)" << endl;
            scale = pow(2, 20);
            break;
#else
        case 4096:
            cout << "30, 30, 30, 19}, scale = pow(2, 25)" << endl;
            scale = pow(2, 25);
            break;
#endif
        case 8192:
            cout << "30 (x6), 38}, scale = pow(2, 25)" << endl;
            scale = pow(2, 25);
            break;
        case 16384:
            cout << "30 (x13), 48}, scale = pow(2, 25)" << endl;
            scale = pow(2, 25);
            break;
        // -- 32768 is technically possible, but we do not support it
        // case 32768: cout << "30 (x28), 41}, scale = pow(2, 25)" << endl;
        // scale = pow(2,25);
        // break;
        default: cout << "Please choose a valid degree." << endl; throw;
    }

    /*
    (From SEAL:)
    A larger coeff_modulus implies a larger noise budget, hence more encrypted
    computation capabilities. However, an upper bound for the total bit-length
    of the coeff_modulus is determined by the poly_modulus_degree, as follows:
        +----------------------------------------------------+
        | poly_modulus_degree | max coeff_modulus bit-length |
        +---------------------+------------------------------+
        | 1024                | 27                           |
        | 2048                | 54                           |
        | 4096                | 109                          |
        | 8192                | 218                          |
        | 16384               | 438                          |
        | 32768               | 881                          |
        +---------------------+------------------------------+
    */

    EncryptionParameters parms(scheme_type::ckks);
    seal::SEALContext context = setup_seale_prime_default(degree, parms);
    bool sk_generated = false, pk_generated = false;

    // -- Testing
    // setup_seal_api(1024, {27}, parms);
    // setup_seal_api(2048, {27, 27}, parms);
    // setup_seal_api(2048, {30, 24}, parms);
    // setup_seal_api(4096, {27, 27, 27, 28}, parms);
    // setup_seal_api(4096, {30, 30, 30, 19}, parms);
    /*
    {
        vector<int> bit_counts;
        for(size_t i = 0; i < 6; i++) bit_counts.push_back(30);
        bit_counts.push_back(38);
        setup_seal_api(8192, bit_counts, parms);
    }
    */
    /*
    {
        for(size_t i = 0; i < 13; i++) bit_counts.push_back(30);
        bit_counts.push_back(48);
        setup_seal_api(16384, bit_counts, parms);
    }
    */
    /*
    {
        for(size_t i = 0; i < 28; i++) bit_counts.push_back(30);
        bit_counts.push_back(41);
        setup_seal_api(32768, bit_counts, parms);
    }
    */

    // string str_sk_fpath  = save_dir_path + "str_sk_" + to_string(degree) + ".h";
    string sk_fpath      = save_dir_path + "sk_" + to_string(degree) + ".dat";
    string str_sk_fpath  = save_dir_path + "str_sk.h";
    string seal_sk_fpath = save_dir_path + "sk_" + to_string(degree) + "_seal" + ".dat";
    string seal_pk_fpath = save_dir_path + "pk_" + to_string(degree) + "_seal" + ".dat";

    SecretKey sk;

    // string err_msg1 = "This option is not yet supported. Please choose another option.";
    string err_msg2 = "This is not a valid option choice. Please choose a valid option.";

    bool is_sym = true;  // TODO: Make this command line settable

    while (1)
    {
        cout << "\nChoose an action:\n";
        cout << "  0) Quit\n";
        cout << "  1) Generate all objects\n";
        if (is_sym) { cout << "  2) Verify ciphertexts (in symmetric mode) \n"; }
        else
        {
            cout << "  2) Verify ciphertexts (in asymmetric mode)\n";
        }
        cout << "  3) Generate secret key, public key\n";
        cout << "  4) Generate IFFT roots\n";
        cout << "  5) Generate fast (a.k.a. \"lazy\")  NTT roots\n";
        cout << "  6) Generate fast (a.k.a. \"lazy\") INTT roots\n";
        cout << "  7) Generate regular  NTT roots\n";
        cout << "  8) Generate regular INTT roots\n";
        cout << "  9) Generate index map\n";
        int option;
        cin >> option;

        bool use_seal_sk_fpath = true;  // This is only used when verifying ciphertexts
        switch (option)
        {
            case 0: exit(0);
            case 2: {
                string ct_str_file_path = is_sym ? ct_str_file_path_sym : ct_str_file_path_asym;
                verify_ciphertexts(save_dir_path, scale, degree, context, is_sym, ct_str_file_path);
            }
            break;
            case 1: [[fallthrough]];
            case 3:
                cout << "Generating secret key..." << endl;
                gen_save_secret_key(sk_fpath, str_sk_fpath, seal_sk_fpath, context);
                cout << "Generating public key..." << endl;
                gen_save_public_key(save_dir_path, seal_pk_fpath, sk_fpath, seal_sk_fpath, context,
                                    use_seal_sk_fpath);
                if (option != 1) break;
            case 4:
                gen_save_ifft_roots(save_dir_path, context, 0, 1);
                if (option != 1) break;
            case 5:
                gen_save_ntt_roots(save_dir_path, context, 1, 0, 0, 1);
                if (option != 1) break;
            case 6:
                gen_save_ntt_roots(save_dir_path, context, 1, 1, 0, 1);
                if (option != 1) break;
            case 7:
                gen_save_ntt_roots(save_dir_path, context, 0, 0, 0, 1);
                if (option != 1) break;
            case 8:
                gen_save_ntt_roots(save_dir_path, context, 0, 1, 0, 1);
                if (option != 1) break;
            case 9: gen_save_index_map(save_dir_path, context, 0); break;
            default: cout << err_msg2 << endl; break;
        }
    }
    return 0;
}

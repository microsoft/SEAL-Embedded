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

void verify_ciphertexts_30bitprime_default(string dirpath, size_t degree, SEALContext &context,
                                           bool symm_enc, string ct_str_file_path,
                                           string sk_binfilename = "")
{
    // size_t n = degree;
    // EncryptionParameters parms(scheme_type::ckks);
    // SEALContext context = setup_seale_30bitprime_default(degree, parms);
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
    // double scale      = (double)(n * n);
    // double scale      = pow(2, 40);
    // double scale      = pow(2, 30);
    double scale = (n > 1024) ? pow(2, 25) : pow(2, 20);
    cout << "\nNumber of slots: " << slot_count << "\n" << endl;

    SecretKey sk = keygen.secret_key();
    if (sk_binfilename.size() == 0)
    {
        sk_binfilename = dirpath + "sk_" + to_string(degree) + ".dat";
    }
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

            bool incl_sp = 1;  // Need to read back the special prime
            pk_bin_file_load(dirpath, context, pk_wr, incl_sp);
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

            // -- Find the expected message from the string file. Comment out if not
            // needed.
            vector<double> values_orig(slot_count, 0);
            cout << "Reading values from file..." << endl;
            filepos = poly_string_file_load(ct_str_file_path, values_orig, 1, filepos);

            // -- Uncomment to check against expected plaintext output (assuming
            //    corresponding lines in seal_embedded.c are also uncommented)
            // vector<int64_t> plaintext_vals(slot_count * 2, 0);
            // cout << "Reading plaintext values from file..." << endl;
            // filepos = poly_string_file_load(ct_str_file_path, plaintext_vals, 1, filepos);
            // print_poly("\npt         ", plaintext_vals, print_size);

            // -- Uncomment to check against expected plaintext output (assuming
            //    corresponding lines in seal_embedded.c are also uncommented)
            // vector<int64_t> plaintext_error_vals(slot_count * 2, 0);
            // cout << "Reading plaintext error values from file..." << endl;
            // filepos =
            //    poly_string_file_load(ct_str_file_path, plaintext_error_vals, 1, filepos);
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

// TODO: make this non-main. Need to have a different main?
int main()
{
    cout << "Parameters: degree 4096, prime bit-lengths: {30, 30, 30, 19}, ntt_form, "
            "scale = pow(2, 25)"
         << endl;
    size_t degree = 4096;
    EncryptionParameters parms(scheme_type::ckks);
    SEALContext context = setup_seale_30bitprime_default(degree, parms);
    bool sk_generated = false, pk_generated = false;

    string sk_fpath = save_dir_path + "sk_" + to_string(degree) + ".dat";
    // string str_sk_fpath  = save_dir_path + "str_sk_" + to_string(degree) + ".h";
    string str_sk_fpath  = save_dir_path + "str_sk.h";
    string seal_sk_fpath = save_dir_path + "sk_" + to_string(degree) + "_seal" + ".dat";
    string seal_pk_fpath = save_dir_path + "pk_" + to_string(degree) + "_seal" + ".dat";

    SecretKey sk;

    // string err_msg1 = "This option is not yet supported. Please choose another
    // option.";
    string err_msg2 = "This is not a valid option choice. Please choose a valid option.";

    while (1)
    {
        cout << "\nChoose an action:\n";
        cout << "  0) Quit\n";
        cout << "  1) Generate all objects\n";
        cout << "  2) Verify ciphertexts\n";
        cout << "  3) Generate secret key, public key\n";
        cout << "  4) Generate IFFT roots\n";
        cout << "  5) Generate fast (a.k.a. \"lazy\")  NTT roots\n";
        cout << "  6) Generate fast (a.k.a. \"lazy\") INTT roots\n";
        cout << "  7) Generate regular  NTT roots\n";
        cout << "  8) Generate regular INTT roots\n";
        cout << "  9) Generate index map\n";
        int option;
        cin >> option;

        bool is_sym             = true;  // TODO: Make this command line settable
        string ct_str_file_path = is_sym ? ct_str_file_path_sym : ct_str_file_path_asym;

        bool use_seal_sk_fpath = true;
        switch (option)
        {
            case 0: exit(0);
            case 2:
                verify_ciphertexts_30bitprime_default(save_dir_path, degree, context, is_sym,
                                                      ct_str_file_path);
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
        exit(0);
    }
    return 0;
}

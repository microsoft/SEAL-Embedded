// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file fileops.h

@brief Functions to load and save keys and ciphertexts and other objects to files.
*/

#pragma once

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#include "seal/seal.h"
#include "utils.h"

// -----------------------------------------------------
// ---------------- Utility functions ------------------
// -----------------------------------------------------

/**
Returns the number of bytes in a file

@param[in] file  File to check
*/
inline std::size_t size_of_file(std::fstream &file)
{
    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    return size;
}

/**
Exits if a file is not open or on file failure. Optionally checks that the file is not empty.
If failure is detected, closes file and exits.

@param[in] file        File to check
@param[in] msg         Message to print on exit
@param[in] check_size  If true, checks that the file is not empty
*/
inline void exit_on_err_file(std::fstream &file, std::string msg, bool check_size)
{
    if (!file.is_open())
    {
        std::cerr << "Error: File is not open." << std::endl;
        goto close_and_exit;
    }
    if (file.fail())
    {
        std::cerr << "Error: File failed." << std::endl;
        goto close_and_exit;
    }
    if (check_size && !size_of_file(file))
    {
        std::cerr << "Error: File is empty." << std::endl;
        goto close_and_exit;
    }
    return;

close_and_exit:
    file.close();
    exit_on_err(1, msg);
}

// ===============================================================
//                     Binary file save/load
//                     (SEAL-Embedded format)
// ===============================================================

/**
Saves the secret key to a binary file. Optionally also creates a code file containing the hard-coded
values of the secret key. This file may then be compiled with the SEAL-Embedded library.

Note: This stores the secret key in **compressed** form.

Note: This function may modify sk in order to write to file, but should revert all changes before
returning.

@param[in] fpath          Path to file to save secret key values in binary form
@param[in] str_fpath      Path to file to hard-code secret key bytes in a code file for use with
                          SEAL-Embedded
@param[in] context        SEAL context
@param[in] use_str_fpath  If true, hard codes secret key bytes to file at str_fpath
@param[in] sk             Secret key instance
*/
void sk_bin_file_save(std::string fpath, std::string str_fpath, const seal::SEALContext &context,
                      bool use_str_fpath, seal::SecretKey &sk);

/**
Loads the secret key from a binary file generated using 'sk_bin_file_save'.

Note: This assumes the secret key was saved in **compressed** form.

@param[in]  fpath    Path to file to load the secret key values, stored in binary form
@param[in]  context  SEAL context
@param[out] sk       Secret key instance
*/
void sk_bin_file_load(std::string fpath, const seal::SEALContext &context, seal::SecretKey &sk);

/**
Saves the public key to a binary file. Also creates a code file containing the hard-coded values of
the public key to compile with the SEAL-Embedded library.

@param[in] dirpath          Path to directory to save public key file
@param[in] context          SEAL context
@param[in] pk_wr            Public key wrapper instance
@param[in] incl_sp          If true, writes bytes for special prime as well
@param[in] high_byte_first  Toggle for endianness
@param[in] append           If true, will append values to binary file. Otherwise, will overwrite
*/
void pk_bin_file_save(std::string dirpath, const seal::SEALContext &context,
                      PublicKeyWrapper &pk_wr, bool incl_sp, bool high_byte_first, bool append = 0);

/**
Loads a public key from a SEAL-Embedded-formatted binary file.

@param[in]  dirpath          Path to directory containing public key file
@param[in]  context          SEAL context
@param[out] pk_wr            Public key wrapper instance
@param[in]  incl_sp          If true, reads in bytes for special prime as well
@param[in]  high_byte_first  Toggle for endianness
*/
void pk_bin_file_load(std::string dirpath, const seal::SEALContext &context,
                      PublicKeyWrapper &pk_wr, bool incl_sp, bool high_byte_first);

// ==============================================================
//                      Binary file save/load
//                         (SEAL format)
// ==============================================================

/**
Saves a secret key to a binary file in SEAL form.

@param[in] fpath     Path to binary file to save the secret key
@param[in] sk        Secret key instance
@param[in] compress  If true, compresses the secret key w/ zstd before saving
*/
void sk_seal_save(std::string fpath, seal::SecretKey &sk, bool compress = true);

/**
Loads a secret key from a SEAL-formatted binary file.

@param[in]  fpath    Path to binary file containing secret key in SEAL form
@param[in]  context  SEAL context
@param[out] sk       Secret key instance
*/
void sk_seal_load(std::string fpath, const seal::SEALContext &context, seal::SecretKey &sk);

/**
Saves a public key to a binary file in SEAL form.

@param[in] fpath     Path to binary file to save the public key
@param[in] pk        Public key instance
@param[in] compress  If true, compresses the public key w/ zstd before saving
*/
void pk_seal_save(std::string fpath, seal::PublicKey &pk, bool compress = true);

/**
Loads a public key from a SEAL-formatted binary file.

@param[in]  fpath    Path to binary file containing public key in SEAL form
@param[in]  context  SEAL context
@param[out] pk       Public key instance
*/
void pk_seal_load(std::string fpath, const seal::SEALContext &context, seal::PublicKey &pk);

// ==============================================================
//                    String file save/load
//                    (Mainly for debugging)
// ==============================================================

/**
Load a secret key from a string file.

@param[in]  fpath    Path to string file containing secret key
@param[in]  context  SEAL context
@param[out] sk       Secret key object to store loaded values
@return Position of file pointer after reading in the secret key
*/
std::streampos sk_string_file_load(std::string fpath, const seal::SEALContext &context,
                                   seal::SecretKey &sk);

/**
Load a freshly encrypted ciphertext from a string file.
CT objects should be formatted as follows:

ct0 : { x, x, x, x, x}
ct1 : { x, x, x, x, x}
ct0 : { x, x, x, x, x} --> w.r.t. next prime
ct1 : { x, x, x, x, x} --> w.r.t. next prime

@param[in]  fpath       Path to string file containing values of ciphertext.
@param[in]  context     SEAL context
@param[in]  evaluator   SEAL evaluator
@param[out] ct          Ciphertext object to load values into
@param[in]  filepos_in  Previous returned value from calling this function
@return Position of file pointer after reading in a single ciphertext
*/
std::streampos ct_string_file_load(std::string fpath, const seal::SEALContext &context,
                                   seal::Evaluator &evaluator, seal::Ciphertext &ct,
                                   std::streampos filepos_in = 0);

/**
Load a polynomial object from a string file.
String file objects should be formatted as follows (values in [] are optional):

    <object_name> : {<value 1>, <value 2>, ...}

@param[in]  fpath        Path to string file containing values of ciphertext.
@param[in]  ncomponents  Number of components of the object (e.g. 2 for a fresh CT)
@param[out] vec          Pointer to memory to store polynomial elements
@param[in]  pos          Position of file to start reading from
@returns End position of ftell after reading 'num_elements' type-T elements of 'invec' starting from
pos
*/
template <typename T>
std::streampos poly_string_file_load(std::string fpath, std::size_t ncomponents, T *vec,
                                     std::streampos pos = 0)
{
    std::vector<T> vec_temp;
    std::fstream infile(fpath.c_str(), std::ios::in);
    std::cout << std::endl << "opening file at: " << fpath << std::endl << std::endl;
    assert(infile.is_open());

    char ch;
    std::size_t idx = 0;

    // -- In case we have multiple lines in a file
    if (pos) { infile.seekg(pos); }

    while ((idx < ncomponents) && infile.get(ch))
    {
        // -- First, find the opening bracket
        if (ch != '{') continue;

        vec_temp.clear();
        for (std::size_t i = 0; !infile.eof(); i++)
        {
            std::string val;

            // -- Read in a single byte
            infile >> val;

            // -- Break when we find the closing bracket
            if (val.find("}") != std::string::npos) break;

            val.erase(remove(val.begin(), val.end(), ','), val.end());

            T val_temp;
            if (std::is_same<T, double>::value)
            { val_temp = static_cast<T>(std::stod(val.c_str(), 0)); }
            else if (std::is_same<T, uint64_t>::value)
            {
                val_temp = static_cast<T>(std::strtoull(val.c_str(), 0, 10));
            }
            else if (std::is_same<T, int64_t>::value)
            {
                val_temp = static_cast<T>(std::strtoll(val.c_str(), 0, 10));
            }
            else
            {
                std::cout << "Error! Type not accounted for." << std::endl;
                exit(0);
            }

            // std::cout << "idx: " << i << " value: " << val_temp << std::endl;

            // -- Store found values in a vector
            vec_temp.push_back(val_temp);
        }

        // -- Copy read elements into input vector at specific location
        std::size_t N = vec_temp.size();
        // print_poly("vec_temp", vec_temp, N);
        for (std::size_t i = 0; i < N; i++) { vec[i + idx * N] = vec_temp[i]; }
        idx++;
    }
    std::streampos curr_pos = infile.tellg();
    assert(infile.is_open());
    infile.close();
    std::cout << "curr_pos: " << curr_pos << std::endl;
    return curr_pos;
}

/**
Load a polynomial object from a string file.
String file objects should be formatted as follows:

    <object_name> : {<value 1>, <value 2>, ...}

@param[in]  fpath        Path to string file containing values of ciphertext.
@param[in]  ncomponents  Number of components of the object (e.g. 2 for a fresh CT)
@param[out] vec          Vector object to store polynomial values
@param[in]  pos          Position of file to start reading from
@returns End position of ftell after reading 'num_elements' type-T elements of 'invec' starting from
pos
*/
template <typename T>
std::streampos poly_string_file_load(std::string fpath, std::size_t ncomponents,
                                     std::vector<T> &vec, std::streampos pos = 0)
{
    return poly_string_file_load(fpath, ncomponents, &(vec[0]), pos);
}

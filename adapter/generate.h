// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file generate.h

@brief Functions to generate secret keys, public keys, ifft/ntt roots, and index map (i.e.
pi-inverse) values. Useful for generating objects to load onto a SEAL-Embedded device.
*/

#pragma once

#include <string>

#include "seal/seal.h"

/**
Wrapper struct for Public key. Required to keep track of whether public key is in NTT form.

@param pk      Pointer to public key
@param is_ntt  If true, pk is in NTT form
*/
typedef struct PublicKeyWrapper
{
    seal::PublicKey *pk;
    bool is_ntt;
} PublicKeyWrapper;

/**
Returns the upper 32 bits of a 64-bit value
*/
static inline uint32_t upper32(uint64_t val)
{
    return (val >> 32) & 0xFFFFFFFF;
}

/**
Generates and saves a secret key to file.

@param[in] sk_fpath       Path to file to store the secret key in SEAL-Embedded form
@param[in] str_sk_fpath   Path to code file to hard-code the values of the secret key
@param[in] seal_sk_fpath  Path to file to store the secret key in SEAL form
@param[in] context        SEAL context
*/
void gen_save_secret_key(std::string sk_fpath, std::string str_sk_fpath, std::string seal_sk_fpath,
                         const seal::SEALContext &context);

/**
Generates and saves a public key to file.

In order to generate the public key, the key generator must read in the secret key from file. This
file may either be: 1) the file created using SEAL-Embedded's adapter save functionality
(SEAL-Embedded form). In this case, set use_seal_sk_fpath to 0 and set sk_fpath to the path to the
secret key. 2) the file created using SEAL's stream save functionality (SEAL form). In this case,
set use_seal_sk_fpath to 1 and set seal_sk_fpath to the path to the secret key.

@param[in] dirpath            Path to the directory to store file containing public key in
                              SEAL-Embedded form
@param[in] seal_pk_fpath      Path to file to store public key
@param[in] sk_fpath           Path to file storing secret key in SEAL-Embedded form
@param[in] seal_sk_fpath      Path to file storing secret key in SEAL form
@param[in] context            SEAL context
@param[in] use_seal_sk_fpath  If true, use seal_sk_fpath to load secret key. Else, use sk_fpath
*/
void gen_save_public_key(std::string dirpath, std::string seal_pk_fpath, std::string sk_fpath,
                         std::string seal_sk_fpath, const seal::SEALContext &context,
                         bool use_seal_sk_fpath);

/**
Generates and saves the IFFT roots to file for use with SEAL-Embedded.

@param[in] dirpath          Path to the directory to store file containing ifft roots
@param[in] context          SEAL context
@param[in] high_byte_first  Toggle for endianness
@param[in] string_roots     If true, generate string header file too
*/
void gen_save_ifft_roots(std::string dirpath, const seal::SEALContext &context,
                         bool high_byte_first, bool string_roots);

/**
Generates and saves the NTT roots to file for use with SEAL-Embedded.

@param[in] dirpath          Path to directory to store file containing NTT roots
@param[in] context          SEAL context
@param[in] lazy             If true, generates "fast" NTT roots (false = regular roots)
@param[in] inverse          If true, generates inverse NTT roots (false = forward roots)
@param[in] high_byte_first  Toggle for endianness
@param[in] string_roots     If true, generate string header file too
*/
void gen_save_ntt_roots(std::string dirpath, const seal::SEALContext &context, bool lazy,
                        bool inverse, bool high_byte_first, bool string_roots);

/**
Generates and saves the index map (i.e. pi-inverse) to file for use with SEAL-Embedded.
Also generates a code file with hard-coded index map values for use with SEAL-Embedded.

@param[in] dirpath          Path to directory to store file containing index map values
@param[in] context          SEAL context
@param[in] high_byte_first  Toggle for endianness
*/
void gen_save_index_map(std::string dirpath, const seal::SEALContext &context,
                        bool high_byte_first);

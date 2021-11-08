// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file convert.h

@brief Functions to convert secret keys, public keys, and ciphertexts to NTT or non-NTT form, and
compare secret key and public key instances.
*/

#pragma once

#include "generate.h"  // PublicKeyWrapper
#include "seal/seal.h"

/**
Converts a secret key to NTT form.

@param[in]     context  SEAL context
@param[in,out] sk       Secret key to convert
*/
void sk_to_ntt_form(const seal::SEALContext &context, seal::SecretKey &sk);

/**
Converts a secret key to non-NTT form.

@param[in]     context  SEAL context
@param[in,out] sk       Secret key to convert
*/
void sk_to_non_ntt_form(const seal::SEALContext &context, seal::SecretKey &sk);

/**
Converts a public key to NTT form.

@param[in]     context  SEAL context
@param[in,out] pk_wr    Wrapper containing public key to convert
*/
void pk_to_ntt_form(const seal::SEALContext &context, PublicKeyWrapper &pk_wr);

/**
Converts a public key to non-NTT form.

@param[in]     context  SEAL context
@param[in,out] pk_wr    Wrapper containing public key to convert
*/
void pk_to_non_ntt_form(const seal::SEALContext &context, PublicKeyWrapper &pk_wr);

/**
Converts a ciphertext to NTT form.

@param[in]     evaluator  SEAL evaluator
@param[in,out] c_in       Ciphertext to convert
*/
void ct_to_ntt_form(seal::Evaluator &evaluator, seal::Ciphertext &c_in);

/**
Converts a ciphertext to non-NTT form.

@param[in]     evaluator  SEAL evaluator
@param[in,out] c_in       Ciphertext to convert
*/
void ct_to_non_ntt_form(seal::Evaluator &evaluator, seal::Ciphertext &c_in);

/**
Converts a plaintext to NTT form.

@param[in]     context  SEAL context
@param[in,out] pt       Plaintext to convert
*/
void pt_to_ntt_form(const seal::SEALContext &context, seal::Plaintext &pt);

/**
Converts a plaintext to non-NTT form.

@param[in]     context  SEAL context
@param[in,out] pt       Plaintext to convert
*/
void pt_to_non_ntt_form(const seal::SEALContext &context, seal::Plaintext &pt);

/**
Compares two secret key instances to see if they match.

Note: This function may modify SecretKey instances to compare NTT and non-NTT forms of the secret
key, but should revert all changes before returning.

@param[in] context       SEAL context
@param[in] sk1           Secret key 1
@param[in] sk2           Secret key 2
@param[in] incl_sp       If true, compares "special prime" component of secret keys
@param[in] should_match  If true, throws an error if sk1 != sk2 (debugging only)
*/
void compare_sk(const seal::SEALContext &context, seal::SecretKey &sk1, seal::SecretKey &sk2,
                bool incl_sp, bool should_match);

/**
Compares two public key instances to see if they match.

Note: This function may modify PublicKeyWrapper instances to compare NTT and non-NTT forms of the
public key, but should revert all changes before returning.

@param[in] context       SEAL context
@param[in] pk1_wr        Public key 1 wrapper
@param[in] pk2_wr        Public key 2 wrapper
@param[in] incl_sp       If true, compares "special prime" component of public keys
@param[in] should_match  If true, throws an error if pk1 != pk2 (debugging only)
*/
void compare_pk(const seal::SEALContext &context, PublicKeyWrapper &pk1_wr,
                PublicKeyWrapper &pk2_wr, bool incl_sp, bool should_match);

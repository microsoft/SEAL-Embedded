// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file utils.h

@brief Various adapter utility functions.
*/

#pragma once

#include <cassert>
#include <fstream>
#include <iomanip>
#include <string>

#include "generate.h"
#include "seal/seal.h"

/**
Uncomment to use the 27-bit default for n=4K instead of the 30-bit default.
Make sure to uncomment SE_DEFAULT_4K_27BIT in the device (see: user_defines.h) as well.
*/
// #define SEALE_DEFAULT_4K_27BIT

/**
Exits the program when an error is detected.

@param[in] err  If = 0, returns. Else, exits
@param[in] msg  Message to print if an error is detected
*/
inline void exit_on_err(int err, std::string msg)
{
    if (!err) return;
    std::cerr << "Error: " << msg << "." << std::endl;
    std::cerr << "Error value: " << err << std::endl;
    exit(1);
}

// -----------------------------------------
// ---------------- Setup ------------------
// -----------------------------------------
/**
Creates SEAL context based on custom-chosen moduli. Verifies compatibility with SEAL-Embedded (i.e.,
all moduli other than the special prime must be <= 30 bits).

@param[in]     degree  Polynomial ring degree
@param[in]     moduli  Moduli
@param[in,out] parms   Encryption parameters object to set
@returns               SEAL context object
*/
seal::SEALContext setup_seale_custom(std::size_t degree, const std::vector<seal::Modulus> &moduli,
                                     seal::EncryptionParameters &parms);

/**
Creates SEAL context based on default moduli for degree for SEAL-Embedded. Verifies compatibility
with SEAL-Embedded (i.e., all moduli other than the special prime must be <= 30 bits). Note: SEAL
chooses primes based on requested degree only. We want primes that will work for all degrees to
simplify device constants. Therefore, primes chosen may not be the same as returned by the SEAL api.

For degree =  1024, chooses  1 27-bit prime
For degree =  2048, chooses  1 27-bit prime  + 1 27-bit special prime.
For degree =  4096, chooses  3 30-bit primes + 1 19-bit special prime.
(if SEALE_DEFAULT_4K_27BIT is defined, chooses  3 27-bit primes + 1 28-bit special prime instead)
For degree =  8192, chooses  6 30-bit primes + 1 38-bit special prime.
For degree = 16384, chooses 13 30-bit primes + 1 48-bit special prime.
Throws an error for all other degree choices.

Note: degree = 32768 is possible but not likely useful for embedded scenarios, so we do not support
it for now.

@param[in]     degree  Polynomial ring degree
@param[in,out] parms   Encryption parameters object to set
@returns               SEAL context object
*/
seal::SEALContext setup_seale_prime_default(std::size_t degree, seal::EncryptionParameters &parms);

/**
Creates SEAL context based on parameters, using the SEAL api to choose the modulus values based on
provided bit-lengths.

@param[in]     degree       Polynomial ring degree
@param[in]     bit_lengths  Vector of bit lengths for moduli
@param[in,out] parms        Encryption parameters object to set
@returns                    SEAL context object
*/
seal::SEALContext setup_seal_api(std::size_t degree, const std::vector<int> &bit_lengths,
                                 seal::EncryptionParameters &parms);

// --------------------------------------------------
// ---------------- Size functions ------------------
// --------------------------------------------------
/**
Returns the number of bytes in a (fully expanded) secret key instance

@param[in] sk       Secret key instance
@param[in] context  SEAL context
@param[in] incl_sp  If true, includes the special prime bytes in the count
*/
std::size_t get_sk_num_bytes(const seal::SecretKey &sk, const seal::SEALContext &context,
                             bool incl_sp = true);

/**
Returns the number of bytes in a public key instance

@param[in] pk       Public key instance
@param[in] incl_sp  If true, includes the special prime bytes in the count
*/
std::size_t get_pk_num_bytes(const seal::PublicKey &pk, bool incl_sp = true);

// -------------------------------------------------
// ---------------- Data pointers ------------------
// -------------------------------------------------
/**
Returns a pointer to the plaintext data array

@param[in] pt  Plaintext instance
@returns       A pointer to the plaintext data array
*/
uint64_t *get_pt_arr_ptr(seal::Plaintext &pt);

/**
Returns a pointer to the ciphertext data array

@param[in] ct              Ciphertext instance
@param[in] second_element  If true, returns pointer starting at second (c1) element
@returns                   A pointer to the ciphertext data array
*/
uint64_t *get_ct_arr_ptr(seal::Ciphertext &ct, bool second_element = false);

/**
Returns a pointer to the secret key data array

@param[in] sk  Secret key instance
@returns       A pointer to the secret key data array
*/
uint64_t *get_sk_arr_ptr(seal::SecretKey &sk);

/**
Returns a pointer to the public key data array

@param[in] pk              Public key instance
@param[in] second_element  If true, returns pointer starting at second element
@returns                   A pointer to the public key data array
*/
uint64_t *get_pk_arr_ptr(seal::PublicKey &pk, bool second_element = false);

/**
Returns a pointer to the public key data array

@param[in] pk_wr           Public key wrapper instance
@param[in] second_element  If true, returns pointer starting at second element
@returns                   A pointer to the public key data array
*/
uint64_t *get_pk_arr_ptr(const PublicKeyWrapper &pk_wr, bool second_element = false);

// ------------------------------------------------------
// ---------------- Clearing functions ------------------
// ------------------------------------------------------
/**
Sets all bytes in a public key instance to 0 (including special prime bytes).

@param[in,out] pk  Public key instance
*/
void clear_pk(const seal::SEALContext &context, seal::PublicKey &pk);

/**
Sets all bytes in a (fully expanded) secret key instance to 0 (including special prime
bytes).

@param[in]     context  SEAL context
@param[in,out] sk       Secret key instance
*/
void clear_sk(const seal::SEALContext &context, seal::SecretKey &sk);

// ----------------------------------------------
// ---------------- Comparison ------------------
// ----------------------------------------------
/**
Compares two public key objects to see if they have the same value.

@param[in] pk1_wr      First public key wrapper instance
@param[in] pk2_wr      Second public key wrapper instance
@param[in] compare_sp  If true, compares special prime bytes as well
@returns               True if both objects have the same values, false otherwise
*/
bool same_pk(const PublicKeyWrapper &pk1_wr, const PublicKeyWrapper &pk2_wr, bool compare_sp);

/**
Compares two secret key objects to see if they have the same value.

@param[in] sk1         First secret key wrapper instance
@param[in] sk2         Second secret key wrapper instance
@param[in] context     SEAL context
@param[in] compare_sp  If true, compares special prime bytes as well
@returns               True if both objects have the same values, false otherwise
*/
bool same_sk(const seal::SecretKey &sk1, const seal::SecretKey &sk2,
             const seal::SEALContext &context, bool compare_sp);

/**
Compares two polynomial objects to see if they have the same value.

@param[in] a      Object 1
@param[in] b      Object 2
@param[in] nvals  Number of elements of a and b to compare
@param[in] diff   Maximum allowed difference between values (only applicable for objects with
                  non-integer values)
@returns          True if a and b have the same values, False otherwise
*/
template <typename T>
bool are_equal_poly(T *a, T *b, std::size_t nvals, double diff = 0.4)
{
    bool is_error = false;
    if (std::is_same<T, double>::value)
    {
        for (std::size_t i = 0; i < nvals; i++)
        {
            double abs_val = std::fabs(a[i] - b[i]);
            if (abs_val >= diff)
            {
                std::streamsize ss = std::cout.precision();  // save original precision
                std::cout << std::setprecision(9);
                std::cout << "a[" << i << "]: " << a[i] << std::endl;
                std::cout << "b[" << i << "]: " << b[i] << std::endl;
                std::cout.precision(ss);  // restore precision
            }
            if (abs_val >= diff) is_error = true;
            assert(!is_error);
        }
    }
    else if (std::is_same<T, uint64_t>::value)
    {
        is_error = memcmp(a, b, nvals * sizeof(T));
        assert(!is_error);
    }
    else
    {
        std::cout << "Error: Have not considered that compare poly option" << std::endl;
        exit(0);
    }
    return !is_error;
}

/**
Compares two polynomial objects to see if they have the same value.

@param[in] a      Object 1
@param[in] b      Object 2
@param[in] nvals  Number of elements of a and b to compare
@param[in] diff   Maximum allowed difference between values (only applicable for objects with
                  non-integer values)
@returns          True if a and b have the same values, False otherwise
*/
template <typename T>
bool are_equal_poly(std::vector<T> &a, std::vector<T> &b, std::size_t nvals, double diff = 0.4)
{
    assert(nvals <= a.size() && nvals <= b.size());
    return are_equal_poly(&(a[0]), &(b[0]), nvals, diff);
}

// --------------------------------------------
// ---------------- Printing ------------------
// --------------------------------------------
/**
Prints all encryption moduli to stdout.

@param[in] parms  Encryption parameters
*/
void print_all_moduli(seal::EncryptionParameters &parms);

/**
Prints a ciphertext to stdout.

@param[in] ct          Ciphertext instance
@param[in] print_size  # of elements of each component to print for debug
*/
void print_ct(seal::Ciphertext &ct, std::size_t print_size);

/**
Prints a public key to stdout.

@param[in] name        Name of public key instance
@param[in] pk_wr       Public key wrapper instance
@param[in] print_size  # of elements of each component to print for debug
@param[in] print_sp    If true, also prints the special prime
*/
void print_pk(std::string name, PublicKeyWrapper &pk_wr, std::size_t print_size, bool print_sp);

/**
Prints two secret key objects to stdout for manual comparison.

@param[in] name1       Name for first secret key object
@param[in] sk1         First secret key wrapper object
@param[in] name2       Name for second secret key object
@param[in] sk2         Second secret key wrapper object
@param[in] context     SEAL context
@param[in] print_size  # of elements of each component to print for debug
@param[in] print_sp    If true, also prints special prime bytes
*/
void print_sk_compare(std::string name1, seal::SecretKey &sk1, std::string name2,
                      seal::SecretKey &sk2, const seal::SEALContext &context,
                      std::size_t print_size, bool print_sp);

/**
Prints two public key objects to stdout for manual comparison.

@param[in] name1       Name for first public key object
@param[in] pk1_wr      First public key wrapper object
@param[in] name2       Name for second public key object
@param[in] pk2_wr      Second public key wrapper object
@param[in] print_size  Number of elements of each component to print for debug
@param[in] print_sp    If true, also prints special prime bytes
*/
void print_pk_compare(std::string name1, const PublicKeyWrapper &pk1_wr, std::string name2,
                      const PublicKeyWrapper &pk2_wr, std::size_t print_size, bool print_sp);

/**
Prints a polynomial object to stdout.

@param[in] pname       Name of object
@param[in] poly        Polynomial object
@param[in] print_size  Number of elements of each component to print for debug
@param[in] prec        Precision with which to print polynomial values
*/
template <typename T>
void print_poly(std::string pname, T *poly, std::size_t print_size, int prec = 2)
{
    std::streamsize ss = std::cout.precision();  // save original precision
    bool is_double     = std::is_same<T, double>::value;
    if (is_double) { std::cout << std::setprecision(prec); }

    std::cout << pname << " : { ";
    for (std::size_t i = 0; i < print_size; i++)
    {
        std::cout << poly[i];
        if (i < (print_size - 1)) { std::cout << ", "; }
    }
    std::cout << " }" << std::endl;

    if (is_double) { std::cout.precision(ss); }  // restore precision
}

/**
Prints a polynomial object to stdout.

@param[in] pname       Name of object
@param[in] poly        Polynomial object
@param[in] print_size  Number of elements of each component to print for debug
@param[in] prec        Precision with which to print polynomial values
*/
template <typename T>
void print_poly(std::string pname, std::vector<T> &poly, std::size_t print_size, int prec = 2)
{
    print_poly(pname, &(poly[0]), print_size, prec);
}

/**
Overloaded << operator for parms_id. Prints the `parms_id' to std::ostream.
(Note: This is modified from SEAL/native/examples/examples.h.)

@param[in] parms_id  parms_id object to print
*/
inline std::ostream &operator<<(std::ostream &stream, seal::parms_id_type parms_id)
{
    // -- Save the formatting information for std::cout.
    std::ios old_fmt(nullptr);
    old_fmt.copyfmt(std::cout);

    stream << std::hex << std::setfill('0') << std::setw(16) << parms_id[0] << " " << std::setw(16)
           << parms_id[1] << " " << std::setw(16) << parms_id[2] << " " << std::setw(16)
           << parms_id[3] << " ";

    // -- Restore the old std::cout formatting.
    std::cout.copyfmt(old_fmt);

    return stream;
}

/**
Prints the parameters in a SEALContext to stdout.
(Note: This is modified from SEAL/native/examples/examples.h.)

@param[in] context  SEAL context
*/
inline void print_parameters(const seal::SEALContext &context)
{
    auto &context_data = *context.key_context_data();

    std::cout << "/" << std::endl;
    std::cout << "| Encryption parameters :" << std::endl;
    std::cout << "|   poly_modulus_degree: " << context_data.parms().poly_modulus_degree()
              << std::endl;

    // -- Print the size of the true (product) coefficient modulus.
    std::cout << "|   coeff_modulus size: ";
    std::cout << context_data.total_coeff_modulus_bit_count() << " (";
    auto coeff_modulus  = context_data.parms().coeff_modulus();
    std::size_t nprimes = coeff_modulus.size();
    for (std::size_t i = 0; i < nprimes - 1; i++)
    { std::cout << coeff_modulus[i].bit_count() << " + "; }
    std::cout << coeff_modulus.back().bit_count();
    std::cout << ") bits" << std::endl;
    std::cout << "\\" << std::endl;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file fileops.h

Load values from storage.
*/

#pragma once

#include "defines.h"
#include "parameters.h"
#include "uintmodarith.h"

#if !defined(SE_DATA_FROM_CODE_COPY) || !defined(SE_DATA_FROM_CODE_DIRECT)
void read_from_image(const char *fpath, size_t bytes_expected, void *vec);
#endif

/**
Loads the secret key from storage, where the secret key is assumed to be stored in small
(compressed) form.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the secret key should be
hard-coded in "kri_data/str_secret_key.h" in an array object called "secret_key_store". This file
can be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, the secret key is assumed to be stored in binary form in a
file called "sk_<n>.dat", where <n> is the value of the polynomial degree. This file can also be
generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 's' should contain space for 2n
bits.

@param[in]  parms  Parameters set by ckks_setup
@param[out] s      Secret key (in small form)
*/
void load_sk(const Parms *parms, ZZ *s);

/**
Loads (one component of) the public key from storage.

A full public key consists of two components per modulus prime in the modulus switching chain. For
example, if there are 3 primes in the modulus switching chain, the public key would consists of 3
per-prime public keys PK0, PK1, PK2, where each PKi conists of two polynomials {pk0, pk1}, for a
total of 6 polynomials, and therefore 6 components.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the public key should be
hard-coded in "kri_data/str_pk_addr_array.h", with the starting addresses of each component stored
in an array called 'pk_prime_addr' (contained in the same file). This file can be generated using
the SEAL-Embedded adapter.

Otherwise, if this function is called, each public key component is assumed to be stored in a
separate file and in binary form. The file should be called "pk<i>_<n>_<q>.dat", where <i> is 0 or
1, <n> is the value of the polynomial degree, and <q> is the value of the modulus prime for the
particular public key component, if the public key is in non-ntt form. If the public key is in NTT
form, the file should actually be called "pk<i>_ntt_<n>_<q>.dat". Both of these files can also be
generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'pki' should contain space for n ZZ elements.

@param[in]  i      Requested polynomial component of the public key for the current modulus prime
@param[in]  parms  Parameters set by ckks_setup
@param[out] pki    Public key component
*/
void load_pki(size_t i, const Parms *parms, ZZ *pki);

#if defined(SE_INDEX_MAP_LOAD) || defined(SE_INDEX_MAP_LOAD_PERSIST) || \
    defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
/**
Loads the values of the index map from storage.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the index map should be
hard-coded in "kri_data/str_index_map.h" in an array object called "index_map_store". This file can
be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, the index map is assumed to be stored in binary form in a
file called "index_map_<n>.dat", where <n> is the value of the polynomial degree. This file can also
be generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'index_map' should contain space for n
uint16_t values.

@param[in]  parms      Parameters set by ckks_setup
@param[out] index_map  Buffer containing index map values
*/
void load_index_map(const Parms *parms, uint16_t *index_map);
#endif

#ifdef SE_IFFT_LOAD_FULL
/**
Loads the values of the IFFT roots from storage.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the ifft roots should be
hard-coded in "kri_data/str_ifft_roots.h" in an array object called "ifft_roots_store". This file
can be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, the ifft_roots are assumed to be stored in binary form in a
file called "ifft_roots_<n>.dat", where <n> is the value of the polynomial degree. This file can
also be generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'ifft_roots' must contain space for n double
complex values.

@param[in]  n           Number of roots to load (i.e. polynomial degree)
@param[out] ifft_roots  IFFT roots
*/
void load_ifft_roots(size_t n, double complex *ifft_roots);
#endif

#ifdef SE_FFT_LOAD_FULL
/**
Loads the values of the FFT roots from storage. This is mainly useful for debugging.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the fft roots should be
hard-coded in "kri_data/str_fft_roots.h" in an array object called "fft_roots_store". This file can
be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, the fft_roots are assumed to be stored in binary form in a
file called "fft_roots_<n>.dat", where <n> is the value of the polynomial degree. This file can also
be generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'fft_roots' must contain space for n double
complex values.

@param[in]  n          Number of roots to load (i.e. polynomial degree)
@param[out] fft_roots  FFT roots
*/
void load_fft_roots(size_t n, double complex *fft_roots);
#endif

#ifdef SE_NTT_REG
/**
Loads the values of the NTT roots (in regular form) for the current modulus prime from storage.

A full set of NTT roots consists of as many components are there are modulus primes in the modulus
switching chain.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the full set of NTT roots should
be hard-coded in "kri_data/str_ntt_roots_addr_array.h", with the starting addresses of each
component of NTT roots stored in an array called 'ntt_roots_addr' (contained in the same file). This
file can be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, each NTT component is assumed to be stored in a separate file
and in binary form. The file should be called "ntt_roots_<n>_<q>.dat", where <n> is the value of the
polynomial degree, and <q> is the value of the modulus prime for the particular NTT component. Both
of these files can be generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'ntt_roots' must contain space for n ZZ
values.

@param[in]  n          Number of roots to load (i.e. polynomial degree)
@param[out] ntt_roots  NTT roots
*/
void load_ntt_roots(const Parms *parms, ZZ *ntt_roots);
#endif

#ifdef SE_INTT_REG
/**
Loads the values of the INTT roots (in regular form) for the current modulus prime from storage.

A full set of INTT roots consists of as many components are there are modulus primes in the modulus
switching chain.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the full set of INTT roots should
be hard-coded in "kri_data/str_intt_roots_addr_array.h", with the starting addresses of each
component of INTT roots stored in an array called 'intt_roots_addr' (contained in the same file).
This file can be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, each INTT component is assumed to be stored in a separate
file and in binary form. The file should be called "intt_roots_<n>_<q>.dat", where <n> is the value
of the polynomial degree, and <q> is the value of the modulus prime for the particular INTT
component. Both of these files can be generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'intt_roots' must contain space for n ZZ
values.

@param[in]  n           Number of roots to load (i.e. polynomial degree)
@param[out] intt_roots  INTT roots
*/
void load_intt_roots(const Parms *parms, ZZ *intt_roots);
#endif

#ifdef SE_NTT_FAST
/**
Loads the values of the "fast" (a.k.a. "lazy") NTT roots for the current modulus prime from storage.

A full set of "fast" NTT roots consists of as many components are there are modulus primes in the
modulus switching chain.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the full set of NTT roots should
be hard-coded in "kri_data/str_ntt_roots_addr_array.h", with the starting addresses of each
component of NTT roots stored in an array called 'ntt_fast_roots_addr' (contained in the same file).
This file can be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, each NTT component is assumed to be stored in a separate file
and in binary form. The file should be called "ntt_fast_roots_<n>_<q>.dat", where <n> is the value
of the polynomial degree, and <q> is the value of the modulus prime for the particular NTT
component. Both of these files can be generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'fast_ntt_roots' must contain space
for 2n ZZ values.

@param[in]  n               Number of roots to load (i.e. polynomial degree)
@param[out] ntt_roots_fast  "Fast" NTT roots
*/
void load_ntt_fast_roots(const Parms *parms, MUMO *ntt_fast_roots);
#endif

#ifdef SE_INTT_FAST
/**
Loads the values of the "fast" (a.k.a. "lazy") INTT roots for the current modulus prime from
storage.

A full set of "fast" INTT roots consists of as many components are there are modulus primes in the
modulus switching chain.

If SE_DATA_FROM_CODE_COPY or SE_DATA_FROM_CODE_DIRECT are defined, the full set of INTT roots should
be hard-coded in "kri_data/str_intt_roots_addr_array.h", with the starting addresses of each
component of INTT roots stored in an array called 'intt_fast_roots_addr' (contained in the same
file). This file can be generated using the SEAL-Embedded adapter.

Otherwise, if this function is called, each INTT component is assumed to be stored in a separate
file and in binary form. The file should be called "intt_fast_roots_<n>_<q>.dat", where <n> is the
value of the polynomial degree, and <q> is the value of the modulus prime for the particular INTT
component. Both of these files can be generated using the SEAL-Embedded adapter.

Space req: If SE_DATA_FROM_CODE_DIRECT is not defined, 'intt_fast_roots' must contain space for 2n
ZZ values.

@param[in]  n                Number of roots to load (i.e. polynomial degree)
@param[out] intt_fast_roots  "Fast" INTT roots
*/
void load_intt_fast_roots(const Parms *parms, MUMO *intt_fast_roots);
#endif

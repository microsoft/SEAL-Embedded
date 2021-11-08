// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file fileops.c
*/

#include "fileops.h"

#include <errno.h>  // errno
#include <stdio.h>
#include <string.h>  // memcpy

#include "defines.h"
#include "parameters.h"
#include "util_print.h"

#if defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT)
#ifdef SE_DEFINE_SK_DATA
#include "str_sk.h"
#endif
#ifdef SE_DEFINE_PK_DATA
#include "str_pk_addr_array.h"
#endif
#ifdef SE_IFFT_LOAD_FULL
#include "str_ifft_roots.h"
#endif
#ifdef SE_FFT_LOAD_FULL
#include "str_fft_roots.h"
#endif
#if defined(SE_NTT_REG) || defined(SE_NTT_FAST)
#include "str_ntt_roots_addr_array.h"
#endif
#if defined(SE_INTT_REG) || defined(SE_INTT_FAST)
#include "str_intt_roots_addr_array.h"
#endif
#if defined(SE_INDEX_MAP_LOAD) || defined(SE_INDEX_MAP_LOAD_PERSIST) || \
    defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
#include "str_index_map.h"
#endif
#elif defined(SE_ON_SPHERE_A7)
#include <applibs/storage.h>
#include <fcntl.h>
#include <unistd.h>  // required for file open, close...
#else
#include <fcntl.h>
#include <unistd.h>  // required for file open, close...
#endif

#if !(defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT))

/**
Helper function to check return value of certain file access functions.

@param[in] ret             Return value to check
@param[in] bytes_expected  Expected number of bytes to read from a file. Set to -1 to
                           skip checking if this value matches the value of 'ret'.
@param[in] fpath           Path to file to close on detected error
*/
void check_ret(ssize_t ret, ssize_t bytes_expected, const char *fpath)
{
    se_assert(fpath);
    if (ret < 0)
    {
        printf("Error: problem with ");
        if (bytes_expected == -1)
        {
            // printf("opening or closing mutable or non-mutable file\n");
            printf("opening or closing file\n");
        }
        else
        {
            // printf("reading from mutable or non-mutable storage\n");
            printf("reading from file\n");
        }

        printf("errno value: %d\n", errno);
        printf("errno message: %s\n", strerror(errno));
        printf("file path: %s\n", fpath);
        exit(1);
    }
    if (bytes_expected != -1)
    {
        if ((size_t)ret != bytes_expected)
        {
            printf("bytes read     : %zd bytes\n", ret);
            printf("bytes expected : %zu bytes\n", bytes_expected);
        }
        se_assert(ret == bytes_expected);
    }
}

/**
Reads bytes stored at the specified location. On the Azure A7, this call called the "image".

Correctness: File located at 'fpath' must contain at least 'bytes_expected' values.
Space req: 'vec' must have space for 'bytes_expected' bytes.

@param[in]  fpath           Path to file storing data to load
@param[in]  bytes_expected  Expected number of bytes to read from the file. Must be > 0.
@param[out] vec             Buffer to store bytes read from the file
*/
void read_from_image(const char *fpath, size_t bytes_expected, void *vec)
{
    se_assert(fpath);
    se_assert(vec);
    se_assert(bytes_expected);
#ifdef SE_ON_SPHERE_A7
    int imageFile = Storage_OpenFileInImagePackage(fpath);
#else
    int imageFile = open(fpath, 0);
#endif
    check_ret(imageFile, -1, fpath);

    ssize_t ret = 0;

    // -- Debugging
    // ZZ val;
    // ret = read(imageFile, &val, sizeof(ZZ));
    // check_ret(ret, sizeof(ZZ), fpath);
    // print_zz("val", val);

    // -- Debugging
    // char byte;
    // ret = read(imageFile, &byte, 1);
    // check_ret(ret, 1, fpath);
    // print_zz("byte", (ZZ)byte);

    ret = read(imageFile, vec, bytes_expected);
    // FILE *file = fdopen(imageFile, "r");
    // ret = fread(vec, 1, bytes_expected, file);
    // check_ret(ret, bytes_expected, fpath);

    ret = close(imageFile);
    // ret = fclose(file);
    check_ret(ret, -1, fpath);
}
#endif

void load_sk(const Parms *parms, ZZ *s)
{
    se_assert(parms && s);
    size_t n = parms->coeff_count;

    // -- Image will always be in small form (2 bits per coeff)
    size_t bytes_expected = n / 4;
#if defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT)
#ifndef SE_DEFINE_SK_DATA
    printf("Error! Sk data must be defined\n");
    while (1)
        ;
#elif defined(SE_DATA_FROM_CODE_COPY)
    // uint8_t *sk_bytes = (uint8_t*)s;
    // for(size_t i = 0; i < bytes_expected; i++) sk_bytes[i] = secret_key[i];
    memcpy(s, &(secret_key[0]), bytes_expected);
    return;
#else
    SE_UNUSED(parms);
    SE_UNUSED(n);
    SE_UNUSED(bytes_expected);
    s = (ZZ *)(&(secret_key[0]));
    return;
#endif
#else
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/sk_%zu.dat", SE_DATA_PATH, n);
    // printf("Retrieving secret key from file located at: %s\n", fpath);
    read_from_image(fpath, bytes_expected, s);
#endif
}

void load_pki(size_t i, const Parms *parms, ZZ *pki)
{
    se_assert(i == 0 || i == 1);
    se_assert(parms && pki);

    size_t n    = parms->coeff_count;
    size_t midx = parms->curr_modulus_idx;
#if defined(SE_DATA_FROM_CODE_COPY) || defined(SE_DATA_FROM_CODE_DIRECT)
#ifndef SE_DEFINE_PK_DATA
    SE_UNUSED(n);
    SE_UNUSED(midx);
    printf("Error! Pk data must be defined\n");
    while (1)
        ;
#elif defined(SE_DATA_FROM_CODE_COPY)
    // for(size_t k = 0; k < n; k++) pki[j] = pk_addr[k];
    memcpy(pki, pk_prime_addr[midx][i], n * sizeof(ZZ));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    pki = pk_prime_addr[midx][i];
#endif
#else
    SE_UNUSED(midx);
    char fpath[MAX_FPATH_SIZE];
    ZZ q = parms->curr_modulus->value;
#ifdef SE_NTT_NONE
    snprintf(fpath, MAX_FPATH_SIZE, "%s/pk%zu_%zu_%" PRIuZZ ".dat", SE_DATA_PATH, i, n, q);
#else
    snprintf(fpath, MAX_FPATH_SIZE, "%s/pk%zu_ntt_%zu_%" PRIuZZ ".dat", SE_DATA_PATH, i, n, q);
#endif

    read_from_image(fpath, n * sizeof(ZZ), pki);
#endif
}

#if defined(SE_INDEX_MAP_LOAD) || defined(SE_INDEX_MAP_LOAD_PERSIST) || \
    defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
void load_index_map(const Parms *parms, uint16_t *index_map)
{
    size_t n = parms->coeff_count;
#ifdef SE_DATA_FROM_CODE_COPY
    memcpy(index_map, &(index_map_store[0]), n * sizeof(uint16_t));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    SE_UNUSED(parms);
    SE_UNUSED(n);
    index_map = (uint16_t *)&(index_map_store[0]);
#else
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/index_map_%zu.dat", SE_DATA_PATH, n);
    read_from_image(fpath, n * sizeof(uint16_t), index_map);
#endif
}
#endif

#ifdef SE_IFFT_LOAD_FULL
void load_ifft_roots(size_t n, double complex *ifft_roots)
{
    se_assert(ifft_roots);
#ifdef SE_DATA_FROM_CODE_COPY
    /*
    double *ifft_roots_double = (double *)(&(ifft_roots[0]));
    for (size_t i = 0; i < 2 * n; i += 2)
    {
        // -- This could be written more simply, but want to make clear
        //    that we are storing only 1 double at a time
        // uint64_t t1 = ifft_roots_save[i];
        // uint64_t *t1_p = &t1;
        // double *t1_pd = (double*)(t1_p);
        // double t1_d = *t1_pd;
        // ifft_roots_double[i] = t1_d;

        // uint64_t t2 = ifft_roots_save[i+1];
        // uint64_t *t2_p = &t2;
        // double *t2_pd = (double*)(t2_p);
        // double t2_d = *t2_pd;
        // ifft_roots_double[i+1] = t2_d;
    }
    */
    memcpy(ifft_roots, ifft_roots_save, n * sizeof(double complex));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    SE_UNUSED(n);
    ifft_roots = (double complex *)&(ifft_roots_save[0]);
#else
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/ifft_roots_%zu.dat", SE_DATA_PATH, n);
    // printf("Retrieving ifft_roots from file located at: %s\n", fpath);
    read_from_image(fpath, n * sizeof(double complex), ifft_roots);
#endif
}
#endif

#ifdef SE_FFT_LOAD_FULL
void load_fft_roots(size_t n, double complex *fft_roots)
{
    se_assert(fft_roots);
#ifdef SE_DATA_FROM_CODE_COPY
    /*
    double *fft_roots_double = (double *)(fft_roots);
    // -- This could be written more simply, but want to make clear
    //    that we are storing only 1 double at a time
    for (size_t i = 0; i < 2 * n; i += 2)
    {
        fft_roots_double[i]     = (double)fft_roots_store[i];
        fft_roots_double[i + 1] = (double)fft_roots_store[i + 1];
    }
    */
    memcpy(fft_roots, fft_roots_save, n * sizeof(double complex));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    SE_UNUSED(n);
    fft_roots = (double complex *)&(fft_roots_store[0]);
#else
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/fft_roots_%zu.dat", SE_DATA_PATH, n);
    // printf("Retrieving fft_roots from file located at: %s\n", fpath);
    read_from_image(fpath, n * sizeof(double complex), fft_roots);
#endif
}
#endif

#ifdef SE_NTT_REG
void load_ntt_roots(const Parms *parms, ZZ *ntt_roots)
{
    se_assert(parms && ntt_roots);
    size_t n    = parms->coeff_count;
    size_t midx = parms->curr_modulus_idx;
#ifdef SE_DATA_FROM_CODE_COPY
    // for(size_t i = 0; i < n; i++)
    // { ntt_roots[i] = ntt_roots_addr[midx][i]; }
    memcpy(ntt_roots, ntt_roots_addr[midx], n * sizeof(ZZ));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    SE_UNUSED(n);
    ntt_roots = ntt_roots_addr[midx];
#else
    SE_UNUSED(midx);
    ZZ q = parms->curr_modulus->value;
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/ntt_roots_%zu_%" PRIuZZ ".dat", SE_DATA_PATH, n, q);
    // printf("Retrieving ntt roots from file located at: %s\n", fpath);
    read_from_image(fpath, n * sizeof(ZZ), ntt_roots);
#endif
}
#endif

#ifdef SE_INTT_REG
void load_intt_roots(const Parms *parms, ZZ *intt_roots)
{
    se_assert(parms && intt_roots);
    size_t n    = parms->coeff_count;
    size_t midx = parms->curr_modulus_idx;
#ifdef SE_DATA_FROM_CODE_COPY
    // for(size_t i = 0; i < n; i++)
    // { intt_roots[i] = intt_roots_addr[midx][i]; }
    memcpy(intt_roots, intt_roots_addr[midx], n * sizeof(ZZ));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    SE_UNUSED(n);
    intt_roots = intt_roots_addr[midx];
#else
    SE_UNUSED(midx);
    ZZ q = parms->curr_modulus->value;
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/intt_roots_%zu_%" PRIuZZ ".dat", SE_DATA_PATH, n, q);
    // printf("Retrieving inverse ntt roots from file located at: %s\n", fpath);
    read_from_image(fpath, n * sizeof(ZZ), intt_roots);
#endif
}
#endif

#ifdef SE_NTT_FAST
void load_ntt_fast_roots(const Parms *parms, MUMO *ntt_fast_roots)
{
    se_assert(parms && ntt_fast_roots);
    size_t n    = parms->coeff_count;
    size_t midx = parms->curr_modulus_idx;
#ifdef SE_DATA_FROM_CODE_COPY
    // for (size_t i = 0; i < n; i++)
    // {
    //     ntt_fast_roots[i].operand  = ntt_roots_addr[midx][2 * i];
    //     ntt_fast_roots[i].quotient = ntt_roots_addr[midx][2 * i + 1];
    // }
    memcpy(ntt_fast_roots, ntt_roots_addr[midx], n * sizeof(MUMO));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    SE_UNUSED(n);
    ntt_fast_roots = ntt_roots_addr[midx];
#else
    SE_UNUSED(midx);
    ZZ q = parms->curr_modulus->value;
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/ntt_fast_roots_%zu_%" PRIuZZ ".dat", SE_DATA_PATH, n, q);
    // printf("Retrieving fast roots from file located at: %s\n", fpath);
    read_from_image(fpath, n * sizeof(MUMO), ntt_fast_roots);
#endif
}
#endif

#ifdef SE_INTT_FAST
void load_intt_fast_roots(const Parms *parms, MUMO *intt_fast_roots)
{
    se_assert(parms && intt_fast_roots);
    size_t n    = parms->coeff_count;
    size_t midx = parms->curr_modulus_idx;
#ifdef SE_DATA_FROM_CODE_COPY
    /*
    for (size_t i = 0; i < n; i++)
    {
        intt_fast_roots[i].operand  = intt_roots_addr[midx][2 * i];
        intt_fast_roots[i].quotient = intt_roots_addr[midx][2 * i + 1];
    }
    */
    memcpy(intt_fast_roots, intt_roots_addr[midx], n * sizeof(MUMO));
#elif defined(SE_DATA_FROM_CODE_DIRECT)
    SE_UNUSED(n);
    intt_fast_roots = intt_roots_addr[midx];
#else
    SE_UNUSED(midx);
    ZZ q = parms->curr_modulus->value;
    char fpath[MAX_FPATH_SIZE];
    snprintf(fpath, MAX_FPATH_SIZE, "%s/intt_fast_roots_%zu_%" PRIuZZ ".dat", SE_DATA_PATH, n, q);
    // printf("Retrieving fast inverse ntt roots from file located at: %s\n", fpath);
    read_from_image(fpath, n * sizeof(MUMO), intt_fast_roots);
#endif
}
#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file main.c

Note: While the tests can run back-to-back, on device it is best to run only one test at a time
(i.e., uncomment the test you want to run and uncomment all others before compiling.)
Note: Not all the tests may run on the devices since tests may consume much more memory than regular
API usage.
*/

#include "defines.h"

#ifndef SE_DISABLE_TESTING_CAPABILITY
#include "util_print.h"

extern void test_add_uint(void);
extern void test_mult_uint(void);
extern void test_add_mod(void);
extern void test_neg_mod(void);
extern void test_mul_mod(void);
extern void test_sample_poly_uniform(size_t n);
extern void test_sample_poly_ternary(size_t n);
extern void test_sample_poly_ternary_small(size_t n);
extern void test_barrett_reduce(void);
extern void test_barrett_reduce_wide(void);
extern void test_poly_mult_ntt(size_t n, size_t nprimes);
extern void test_fft(size_t n);
extern void test_enc_zero_sym(size_t n, size_t nprimes);
extern void test_enc_zero_asym(size_t n, size_t nprimes);
extern void test_ckks_encode(size_t n);
extern void test_ckks_encode_encrypt_sym(size_t n, size_t nprimes);
extern void test_ckks_encode_encrypt_asym(size_t n, size_t nprimes);
extern void test_ckks_api_sym(void);
extern void test_ckks_api_asym(void);

#ifdef SE_ON_SPHERE_M4
#include "mt3620.h"
#include "os_hal_gpt.h"
#include "os_hal_uart.h"

static const uint8_t uart_port_num = OS_HAL_UART_PORT0;
// static const uint8_t uart_port_num = OS_HAL_UART_ISU0;

/**
Hook for "printf"
@param[in] character  Character to print
*/
// /*
void _putchar(char character)
{
    mtk_os_hal_uart_put_char(uart_port_num, character);
    if (character == '\n') mtk_os_hal_uart_put_char(uart_port_num, '\r');
}
// */

void RTCoreMain(void)
{
    // -- Init Vector Table --
    NVIC_SetupVectorTable();

    // -- Init UART --
    mtk_os_hal_uart_ctlr_init(uart_port_num);
    printf("\nUART Inited (port_num=%d)\n", uart_port_num);

    // -- Init GPT --
    // gpt0_int.gpt_cb_hdl = Gpt0Callback;
    // gpt0_int.gpt_cb_data = (void *)gpt_cb_data;
    mtk_os_hal_gpt_init();
#else

extern int test_network_basic(void);
extern void test_network(void);

int main(void)
{
#ifdef SE_NRF5_UART_PRINTF_ENABLED
    se_setup_uart();
#endif
#endif
    // while(1)
    // {
    //     printf("Beginning tests...\n");
    // }
    printf("Beginning tests...\n");
#ifdef SE_RAND_NRF5
    se_randomness_init();  // required for nrf
#endif

#ifdef SE_USE_MALLOC
    // const size_t n =  1024, nprimes = 1;
    // const size_t n =  2048, nprimes = 1;
    const size_t n = 4096, nprimes = 3;
    // const size_t n =  8192, nprimes = 6;
    // const size_t n = 16384, nprimes = 13;
#else
    const size_t n       = SE_DEGREE_N;
    const size_t nprimes = SE_NPRIMES;
#endif

    test_sample_poly_uniform(n);
    test_sample_poly_ternary(n);
    test_sample_poly_ternary_small(n);  // Only useful when SE_USE_MALLOC is defined

    test_add_uint();
    test_mult_uint();

    test_barrett_reduce();
    test_barrett_reduce_wide();

    test_add_mod();
    test_neg_mod();
    test_mul_mod();

    // -- Note: This test sometimes takes a while to run
    //    because it uses schoolbook multiplication
    // -- Comment it out unless you need to test it
    // test_poly_mult_ntt(n, nprimes);

    test_fft(n);

    test_enc_zero_sym(n, nprimes);
    test_enc_zero_asym(n, nprimes);

    test_ckks_encode(n);

    // -- Main tests
    test_ckks_encode_encrypt_sym(n, nprimes);
    test_ckks_encode_encrypt_asym(n, nprimes);

    // -- Run these tests to verify api
    // -- Check the result with the adapter by writing output to a text file
    //    and passing that to the adapter "verify ciphertexts" functionality
    // test_ckks_api_sym();
    // test_ckks_api_asym();

    // test_network_basic();
    // test_network();

    printf("...done with all tests. All tests passed.\n");
    exit(0);
}

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file main.c

Note: Currently, you can only run one test at a time. Uncomment the test you want to run (and
uncomment all others) before compiling. Note: Not all the tests will run on the devices since tests
may consume much more memory than regular API usage.
*/

#include "defines.h"
#include "util_print.h"

extern void test_add_uint(void);
extern void test_mult_uint(void);
extern void test_add_mod(void);
extern void test_neg_mod(void);
extern void test_mul_mod(void);
extern void test_sample_poly_uniform(void);
extern void test_sample_poly_ternary(void);
extern void test_sample_poly_ternary_small(void);
extern void test_barrett_reduce(void);
extern void test_barrett_reduce_wide(void);
extern void test_poly_mult_ntt(void);
extern void test_fft(void);
extern void test_enc_zero_sym(void);
extern void test_enc_zero_asym(void);
extern void test_ckks_encode(void);
extern void test_ckks_encode_encrypt_sym(void);
extern void test_ckks_encode_encrypt_asym(void);
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
    se_randomness_init();  // required for nrf. does nothing if not on nrf

    // test_sample_poly_uniform();
    // test_sample_poly_ternary();
    // test_sample_poly_ternary_small(); // Only useful when SE_USE_MALLOC is defined

    // test_add_uint();
    // test_mult_uint();

    // test_barrett_reduce();
    // test_barrett_reduce_wide();

    // test_add_mod();
    // test_neg_mod();
    // test_mul_mod();

    // -- Note: This test sometimes takes a while to run
    //    because it uses schoolbook multiplication
    // test_poly_mult_ntt();

    test_fft();

    // test_enc_zero_sym();
    // test_enc_zero_asym();

    // test_ckks_encode();

    // -- Main tests
    test_ckks_encode_encrypt_sym();
    // test_ckks_encode_encrypt_asym();

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

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file main.c

Note: While the benchmarks can run back-to-back, on device it is best to run only one benchmark at a
time (i.e., uncomment the benchmark you want to run and uncomment all others before compiling.)
*/

#include "defines.h"

#ifdef SE_ENABLE_TIMERS
#include "util_print.h"

// -- Benchmarks
extern void bench_index_map(void);
extern void bench_ifft(void);
extern void bench_ntt(void);
extern void bench_prng_randomize_seed(void);
extern void bench_prng_fill_buffer(void);
extern void bench_prng_randomize_seed_fill_buffer(void);
extern void bench_sample_uniform(void);
extern void bench_sample_ternary_small(void);
extern void bench_sample_poly_cbd(void);
extern void bench_sym(void);
extern void bench_asym(void);

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

int main(void)
{
#ifdef SE_NRF5_UART_PRINTF_ENABLED
    se_setup_uart();
#endif
#endif

    printf("Beginning bench...\n");
#ifdef SE_RAND_NRF5
    se_randomness_init();  // required for nrf
#endif

    // -- Uncomment benchmark to run
    // -- Note: All benchmarks use n = 4K, 3 primes if malloc is used,
    //    SE_DEGREE_N and SE_NPRIMES otherwise
    // TODO: make benchmarks easily configurable for other degrees

    bench_index_map();
    bench_ifft();
    bench_ntt();
    bench_prng_randomize_seed();
    bench_prng_fill_buffer();
    bench_prng_randomize_seed_fill_buffer();
    bench_sample_uniform();
    bench_sample_ternary_small();
    bench_sample_poly_cbd();
    bench_sym();
#if defined(SE_USE_MALLOC) || defined(SE_DEFINE_PK_DATA)
    bench_asym();
#endif
    printf("...done with all benchmarks!\n");
}
#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file main.c
*/

#include "defines.h"
#include "util_print.h"

#ifdef SE_ENABLE_TIMERS

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

int main(void)
{
    #ifdef SE_NRF5_UART_PRINTF_ENABLED
    se_setup_uart();
    #endif
    se_randomness_init();  // required for nrf. does nothing if not on nrf

    // -- Uncomment benchmark to run
    bench_index_map();
    // bench_ifft();
    // bench_ntt();
    // bench_prng_randomize_seed();
    // bench_prng_fill_buffer();
    // bench_prng_randomize_seed_fill_buffer();
    // bench_sample_uniform();
    // bench_sample_ternary_small();
    // bench_sample_poly_cbd();
    // bench_sym();
    #ifdef SE_DEFINE_PK_DATA
    bench_asym();
    #endif
    printf("...done with all benchmarks!\n");
}
#else
int main(void)
{
}
#endif

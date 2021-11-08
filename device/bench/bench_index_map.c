// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file bench_index_map.c
*/

#include "defines.h"

#if defined(SE_ENABLE_TIMERS)
#include <stdbool.h>
#include <stdio.h>

#include "bench_common.h"
#include "ckks_common.h"
#include "fileops.h"
#include "parameters.h"
#include "timer.h"
#include "util_print.h"

void bench_index_map(void)
{
#ifdef SE_USE_MALLOC
    const PolySizeType n = 4096;
    uint16_t *vec        = calloc(n, sizeof(uint16_t));
#else
    const PolySizeType n = SE_DEGREE_N;
    uint16_t vec[SE_DEGREE_N];
    memset(&vec, 0, SE_DEGREE_N * sizeof(uint16_t));
#endif
    uint16_t *index_map = &(vec[0]);

    Parms parms;
    ckks_setup(n, 1, NULL, &parms);

    const char *bench_name = "index map";
    print_bench_banner(bench_name, &parms);

    Timer timer;
    const size_t COUNT = 10;
    float t_total = 0, t_min = 0, t_max = 0, t_curr = 0;
    for (size_t b_itr = 0; b_itr < COUNT + 1; b_itr++)
    {
        se_secure_zero_memset(index_map, n * sizeof(uint16_t));
        reset_start_timer(&timer);

#if defined(SE_INDEX_MAP_PERSIST) || defined(SE_INDEX_MAP_OTF)
        ckks_calc_index_map(&parms, index_map);
#elif defined(SE_INDEX_MAP_LOAD) || defined(SE_INDEX_MAP_LOAD_PERSIST) || \
    defined(SE_INDEX_MAP_LOAD_PERSIST_SYM_LOAD_ASYM)
        load_index_map(&parms, index_map);
#endif

        stop_timer(&timer);
        t_curr = read_timer(timer, MICRO_SEC);
        fflush(stdout);

        if (b_itr) set_print_time_vals(bench_name, t_curr, b_itr, &t_total, &t_min, &t_max);

        fflush(stdout);
        print_poly_uint16_full("indices", index_map, n);
        fflush(stdout);
    }
    fflush(stdout);
    print_time_vals(bench_name, t_curr, COUNT, &t_total, &t_min, &t_max);

    fflush(stdout);
    print_bench_banner(bench_name, &parms);

#ifdef SE_USE_MALLOC
    if (vec)
    {
        free(vec);
        vec = 0;
    }
#endif

    delete_parameters(&parms);
}
#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file timer.h

Timer struct and timing functions. Mainly useful for testing. We leave it as part of the library in
case we want to time internal functions.
*/

#pragma once

#include "defines.h"

#ifdef SE_ENABLE_TIMERS
#include <stdint.h>

#ifdef SE_ON_SPHERE_M4
#include "os_hal_gpt.h"
// -- We set these to the clock we want to use
//    There are 5 GPT clocks: {0, 1, 3} --> interrupt based, {2, 4} --> free-run
//    GPT2 ~= 32KHz or 1Hz, GPT4 = ~= bus clock speed or (1/2)*bus clockspeed
static const uint8_t global_gpt_timer_id = OS_HAL_GPT4;
static const bool global_gpt_high_speed  = 1;
#elif defined(SE_ON_NRF5)
#include "nrf.h"
#include "nrf_drv_timer.h"  // TODO: Check if this is needed
#include "sdk_config.h"
#elif !defined(SE_ON_SPHERE_A7)
#include <time.h>
#endif

/**
Struct for storing time points.

@param start           Starting time
@param stop            End time
@param elapsed_time    Elapsed time (start-stop) in nanoseconds
@param timer_instance  Timer instance (Used only with SE_ON_NRF5). Equal to NRFX_TIMER_INSTANCE(0)
@param timer_config    Timer config (Used only with SE_ON_NRF5). Equal to NRFX_TIMER_DEFAULT_CONFIG
*/
typedef struct Timer
{
#if defined(SE_ON_SPHERE_M4) || defined(SE_ON_NRF5)
    uint32_t start;  // Starting time
    uint32_t stop;   // End time
#elif defined(SE_ON_SPHERE_A7)
    uint64_t start;  // Starting time
    uint64_t stop;   // End time
#else
    struct timespec start;  // Starting time
    struct timespec stop;   // End time
#endif
    float elapsed_time;  // Elapsed time (start-stop) in nanoseconds
#ifdef SE_ON_NRF5
    nrfx_timer_t *timer_instance;       // Timer instance (= NRFX_TIMER_INSTANCE(0))
    nrfx_timer_config_t *timer_config;  // Timer config (= NRFX_TIMER_DEFAULT_CONFIG)
#endif
} Timer;

typedef enum TimeUnit {
    SEC       = 1UL,
    MILLI_SEC = 1000UL,
    MICRO_SEC = 1000000UL,
    NANO_SEC  = 1000000000UL
} TimeUnit;

void start_timer(Timer *timer);

void stop_timer(Timer *timer);

void reset_timer(Timer *timer);

void reset_start_timer(Timer *timer);

float read_timer(Timer timer, TimeUnit unit);

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file timer.c
*/

#include "defines.h"

#ifdef SE_ENABLE_TIMERS
#include "timer.h"

// TODO: Account for counter overflow

#ifdef SE_ON_SPHERE_M4
// -- These values are from CodethinkLabs. We will use this even though
//    we are using MediaTek drivers, since MediaTek does not seem to have
//    something similar. See the following url:
//    https://github.com/CodethinkLabs/mt3620-m4-drivers/blob/bef9d7187a621a79f68683049dc42c5b413a13a4/mt3620/gpt.h
#ifndef ROUND_DIVIDE
#define ROUND_DIVIDE(x, y) ((x + (y / 2)) / y)
#endif
#ifndef MT3620_BUS_CLOCK
#define MT3620_BUS_CLOCK 160000000.0f  // Note: This is different from the gpt bus clock!
#endif
#ifndef MT3620_GPT_BUS_CLOCK
#define MT3620_GPT_BUS_CLOCK 26000000.0f
#endif
#ifndef MT3620_GPT_012_HIGH_SPEED
#define MT3620_GPT_012_HIGH_SPEED 32768.0f
#endif
#ifndef MT3620_GPT_012_LOW_SPEED
#define MT3620_GPT_012_LOW_SPEED \
    ROUND_DIVIDE(MT3620_GPT_012_HIGH_SPEED, 33)  // (should be 32/33 kHz - according to datasheet)
#endif
#ifndef MT3620_GPT_3_SRC_CLK_HZ
#define MT3620_GPT_3_SRC_CLK_HZ MT3620_GPT_BUS_CLOCK
#endif
#ifndef MT3620_GPT_3_LOW_SPEED
#define MT3620_GPT_3_LOW_SPEED ROUND_DIVIDE(MT3620_GPT_BUS_CLOCK, 26)
#endif
#ifndef MT3620_GPT_3_HIGH_SPEED
#define MT3620_GPT_3_HIGH_SPEED MT3620_GPT_BUS_CLOCK
#endif
#ifndef MT3620_GPT_3_SPEED_RANGE
#define MT3620_GPT_3_SPEED_RANGE (MT3620_GPT_3_HIGH_SPEED - MT3620_GPT_3_LOW_SPEED)
#endif
#ifndef MT3620_GPT_4_HIGH_SPEED
#define MT3620_GPT_4_HIGH_SPEED MT3620_BUS_CLOCK
#endif
#ifndef MT3620_GPT_4_LOW_SPEED
#define MT3620_GPT_4_LOW_SPEED MT3620_BUS_CLOCK / 2
#endif

/**
Helper function to get the current value of the counter at global_gpt_timer_id

@param[out] clc  Value of the counter
*/
static void get_counter(uint32_t *clc)
{
    *clc = mtk_os_hal_gpt_get_cur_count(global_gpt_timer_id);
}

/**
Helper function to get the frequency of the timer at global_gpt_timer_id

@param[out] freq  Frequency value
*/
static inline void get_freq(uint32_t *freq)
{
    size_t freq_temp;
    switch (global_gpt_timer_id)
    {
        case OS_HAL_GPT0:
        case OS_HAL_GPT1:
        case OS_HAL_GPT2:
            freq_temp =
                global_gpt_high_speed ? MT3620_GPT_012_HIGH_SPEED : MT3620_GPT_012_LOW_SPEED;
            break;
        case OS_HAL_GPT3:
            freq_temp = global_gpt_high_speed ? MT3620_GPT_3_HIGH_SPEED : MT3620_GPT_3_LOW_SPEED;
            break;
        case OS_HAL_GPT4:
            freq_temp = global_gpt_high_speed ? MT3620_GPT_4_HIGH_SPEED : MT3620_GPT_4_LOW_SPEED;
            break;
        default: return;
    }
    se_assert(freq_temp < UINT32_MAX);
    *freq = freq_temp;
}

/**
Start the timer at global_gpt_timer_id using global_gpt_high_speed.

@param[in,out] timer  Timer instance
*/
void start_timer(Timer *timer)
{
    static bool already_started = 0;
    if (!already_started)
    {
        int err = mtk_os_hal_gpt_config(global_gpt_timer_id, global_gpt_high_speed, NULL);
        // if (err) printf("Error (gpt config): %d\n", err);
        se_assert(!err);
        already_started = 1;
    }
    get_counter(&timer->start);
    int err = mtk_os_hal_gpt_start(global_gpt_timer_id);
    // if (err) printf("Error (gpt start): %d\n", err);
    se_assert_msg(!err, "gpt start");
}

/**
Helper function to calculated the elapsed time. Should only be called after timer has been stopped.

@param[in] start  Starting counter value
@param[in] stop   Ending counter value
@returns          Elapsed time (in nanoseconds)
*/
static float calc_elapsed_time(uint32_t start, uint32_t stop)
{
    uint32_t freq;
    get_freq(&freq);
    static bool here_before = 0;
    if (!here_before) printf("frequency (Hz): %u\n", freq);
    here_before = 1;
    return (float)((double)(stop - start) * (double)1e9 / (double)freq);
}

/**
Stop the timer at global_gpt_timer_id. Should only be called after timer has been started.

@param[in,out] timer  Timer instance
*/
void stop_timer(Timer *timer)
{
    get_counter(&(timer->stop));
    printf("called stop_timer. timer->start: %u, timer->stop: %u\n", timer->start, timer->stop);
    timer->elapsed_time += calc_elapsed_time(timer->start, timer->stop);

    // -- This will clear the counter too
    // int err =
    mtk_os_hal_gpt_stop(global_gpt_timer_id);
    // if (err) printf("Error (gpt stop): %d\n", err);
    // se_assert(!err);
}

#elif defined(SE_ON_NRF5)
#include "app_error.h"
#include "nrf.h"
#include "nrf_drv_timer.h"  // TODO: Check if this is needed

static nrfx_timer_t timer_instance_global      = (nrfx_timer_t)NRFX_TIMER_INSTANCE(0);
static nrfx_timer_config_t timer_config_global = (nrfx_timer_config_t)NRFX_TIMER_DEFAULT_CONFIG;

/**
Helper function to get the current value of the NRFX0 timer instance

@param[in]  timer  Timer instance
@param[out] clc    Value of the counter
*/
static void get_counter(Timer *timer, uint32_t *clc)
{
    // volatile uint32_t time_result =
    // nrfx_timer_capture(&(timer->timer_instance), NRF_TIMER_CC_CHANNEL0);
    uint32_t time_result = nrfx_timer_capture(timer->timer_instance, NRF_TIMER_CC_CHANNEL0);
    // printf("count: %u\n", time_result);
    *clc = time_result;
}

/**
NRF timer timeout function handler. Throws an error if called if debugging is enabled.

@param[in]  event_type
@param[in]  p_context
*/
void se_nrf5_timer_timeout_func(nrf_timer_event_t event_type, void *p_context)
{
    se_assert_msg(0, "Error: Timer expired when it probably should not have!\n");
}

/**
Starts the NRFX0 timer instance.

@param[in,out] timer  Timer instance
*/
void start_timer(Timer *timer)
{
    static bool already_started = 0;
    nrfx_err_t ret_val;
    if (!already_started)
    {
        // nrfx_timer_event_handler_t event_handler = &se_nrf5_timer_timeout_func;
        nrfx_timer_event_handler_t event_handler = se_nrf5_timer_timeout_func;
        // -- Note that a different frequency value would require a change to calc_elapsed_time
        timer->timer_instance = &timer_instance_global;  // (nrfx_timer_t)NRFX_TIMER_INSTANCE(0);
        timer->timer_config =
            &timer_config_global;  // (nrfx_timer_config_t)NRFX_TIMER_DEFAULT_CONFIG;
        se_assert(timer->timer_config->frequency ==
                  (nrf_timer_frequency_t)NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY);
        se_assert(timer->timer_config->frequency == (nrf_timer_frequency_t)NRF_TIMER_FREQ_16MHz);
        se_assert(timer->timer_config->mode == (nrf_timer_mode_t)NRFX_TIMER_DEFAULT_CONFIG_MODE);
        se_assert(timer->timer_config->mode == (nrf_timer_mode_t)NRF_TIMER_MODE_TIMER);
        se_assert(timer->timer_config->bit_width ==
                  (nrf_timer_bit_width_t)NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH);
        se_assert(timer->timer_config->bit_width == (nrf_timer_bit_width_t)NRF_TIMER_BIT_WIDTH_32);
        se_assert(timer->timer_config->p_context == NULL);
        // ret_val = nrfx_timer_init(&(timer->timer_instance),
        // &(timer->timer_config), event_handler);
        ret_val = nrfx_timer_init(timer->timer_instance, timer->timer_config, event_handler);
        se_assert(ret_val == NRF_SUCCESS);
        already_started = 1;
    }
    // nrfx_timer_clear(&(timer->timer_instance));
    nrfx_timer_clear(timer->timer_instance);
    get_counter(timer, &timer->start);
    se_assert(timer->start == 0);
    // nrfx_timer_enable(&(timer->timer_instance));
    // se_assert(nrfx_timer_is_enabled(&(timer->timer_instance)));
    nrfx_timer_enable(timer->timer_instance);
    se_assert(nrfx_timer_is_enabled(timer->timer_instance));
}

/**
Stops the NRFX0 timer instance. Should only be called after timer has been started.

@param[in,out] timer  Timer instance
*/
void stop_timer(Timer *timer)
{
    uint32_t time_result;
    get_counter(timer, &time_result);
    timer->stop = time_result;
    // nrfx_timer_disable(&(timer->timer_instance));
    nrfx_timer_disable(timer->timer_instance);
    printf("called stop_timer. timer->start: %u, timer->stop: %u\n", timer->start, timer->stop);
    se_assert(timer->start < timer->stop);

    // -- Time in ns = [# of ticks = (stop-start)] * [(1/freq) = (seconds/tick)] *
    // (10^9 ns/1sec)
    // -- We know frequency = 16 MHz = 16 * 10^6 from start_timer
    // -- So, return Time in ns = (stop-start) * (1/16) * 10^3 = (stop-start)*62.5
    // -- (Note that the resolution of this timer is only microseconds though)
    timer->elapsed_time += (float)((double)(timer->stop - timer->start) * 62.5);
}

#elif defined(SE_ON_SPHERE_A7)

/**
Helper function to get the current value of the counter at CNTVCT.

Note: Uses assembly.

@param[out] clc    Value of the counter
*/
static void get_counter(uint64_t *clc)
{
    uint32_t *clc32 = (uint32_t *)clc;
    __asm volatile(
        "ISB\n\t"  // memfence
        "DSB\n\t"
        "DMB\n\t"
        "MRRC p15, 1, %0, %1, c14\n\t"  // reads CNTVCT into R0 (low) and R1
                                        // (high)
        "ISB\n\t"                       // memfence
        "DSB\n\t"
        "DMB\n\t"
        : "=r"(clc32[0]), "=r"(clc32[1]));
}

/**
Helper function to get the frequency of the counter

@param[out] freq  Frequency value
*/
static void get_freq(uint32_t *freq)
{
    __asm volatile("MRC p15, 0, %0, c14, c0, 0\n\t" : "=r"(freq[0]));
}

/**
Starts the timer

@param[in,out] timer  Timer instance
*/
void start_timer(Timer *timer)
{
    get_counter(&(timer->start));
}

/**
Helper function to calculated the elapsed time. Should only be called after timer has been stopped.

@param[in] start  Starting counter value
@param[in] stop   Ending counter value
@returns          Elapsed time (in nanoseconds)
*/
static float calc_elapsed_time(uint64_t start, uint64_t stop)
{
    uint32_t freq;
    get_freq(&freq);
    return (float)((double)(stop - start) * (double)1e9 / (double)freq);
}

/**
Stops the timer. Should only be called after timer has been started.

@param[in,out] timer  Timer instance
*/
void stop_timer(Timer *timer)
{
    get_counter(&(timer->stop));
    timer->elapsed_time += calc_elapsed_time(timer->start, timer->stop);
}

#else

/**
Starts the timer (uses clock_gettime).

@param[in,out] timer  Timer instance
*/
void start_timer(Timer *timer)
{
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &timer->start);
}

/**
Helper function to calculated the elapsed time. Should only be called after timer has been stopped.

@param[in] start  Starting counter value
@param[in] stop   Ending counter value
@returns          Elapsed time (in nanoseconds)
*/
static float calc_elapsed_time(struct timespec start, struct timespec stop)
{
    return (float)(1e9 * (stop.tv_sec - start.tv_sec) + stop.tv_nsec - start.tv_nsec);
}

/**
Stops the timer (uses clock_gettime). Should only be called after timer has been started.

@param[in,out] timer  Timer instance
*/
void stop_timer(Timer *timer)
{
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &timer->stop);
    timer->elapsed_time += calc_elapsed_time(timer->start, timer->stop);
}
#endif

/**
Resets the timer instance.

@param[in,out] timer  Timer instance
*/
void reset_timer(Timer *timer)
{
#if defined(SE_ON_SPHERE_A7) || defined(SE_ON_SPHERE_M4) || defined(SE_ON_NRF5)
    timer->start = 0;
    timer->stop  = 0;
#else
    timer->start.tv_sec  = 0;
    timer->start.tv_nsec = 0;
    timer->stop.tv_sec   = 0;
    timer->stop.tv_nsec  = 0;
#endif
    timer->elapsed_time = 0;
}

/**
Resets the timer instance and starts the timer.

@param[in,out] timer  Timer instance
*/
void reset_start_timer(Timer *timer)
{
    reset_timer(timer);
    start_timer(timer);
}

/**
Returns the elapsed time of the timer in the requested unit of time.

@param[in] timer Timer instance
@param[in] unit  Time unit
@returns         The elapsed time of the timer in the requested unit
*/
volatile float read_timer(Timer timer, TimeUnit unit)
{
    return timer.elapsed_time / (float)(NANO_SEC / unit);
}
#endif

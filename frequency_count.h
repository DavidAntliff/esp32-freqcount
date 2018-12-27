/*
 * MIT License
 *
 * Copyright (c) 2018 Chris Morgan <chmorgan@gmail.com>
 * Copyright (c) 2018 David Antliff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file frequency_count.h
 * @brief Example application for the ESP32 Frequency Counter component.
 *
 * An accurate sampling window signal is created by the RMT component, during which time
 * a Pulse Counter counts both rising and falling edges of the signal of interest.
 *
 * To connect the RMT window output to the PCNT enable, a GPIO must be used. This is set by rmt_gpio.
 * It should not be used for any external purposes.
 *
 * Usage:
 * 
 * - Create an instance of frequency_count_configuration_t and set its values appropriately
 * - Create a task with frequency_count_task() as its function, passing a pointer
 *   to the frequency_count_configuration_t into the task creation function
 * 
 * TODO:
 * - Support sampling periods of milliseconds
 * 
 */

#pragma once
#ifndef FREQUENCY_COUNT_H
#define FREQUENCY_COUNT_H

#include <stdint.h>
#include "driver/pcnt.h"
#include "driver/rmt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    gpio_num_t pcnt_gpio;                   ///< count events on this GPIO
    pcnt_unit_t pcnt_unit;                  ///< PCNT unit to use for counting
    pcnt_channel_t pcnt_channel;            ///< PCNT channel to use for counting
    gpio_num_t rmt_gpio;                    ///< used by RMT to define a sampling window
    rmt_channel_t rmt_channel;              ///< The RMT channel to use
    uint8_t rmt_clk_div;                    ///< RMT pulse length, as a divider of the APB clock
    uint8_t rmt_max_blocks;                 ///< Maximum number of RMT RAM blocks that can be used by task. Each block provides 64 RMT items and each item can provide a window duration of up to 2 * 32767 = 65534 periods.
    float sampling_period_seconds;          ///< time (in seconds) between start of adjacent samples
    float sampling_window_seconds;          ///< sample window length (in seconds)
    uint16_t filter_length;                 ///< counter filter length in APB cycles
    void (*window_start_callback)(void);    ///< called just prior to starting a sampling window
    void (*frequency_update_callback)(double hz);  ///< called each time a frequency is determined
} frequency_count_configuration_t;

/**
 * @brief Task function - pass this to xTaskCreate with a pointer to an instance of frequency_count_configuration_t as the parameter.
 */
void frequency_count_task_function(void * pvParameter);

#ifdef __cplusplus
}
#endif

#endif // FREQUENCY_COUNT

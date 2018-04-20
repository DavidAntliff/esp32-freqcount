#pragma once

#include <stdint.h>
#include "driver/rmt.h"

/**
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
typedef struct
{
    uint8_t pcnt_gpio;                      // count events on this GPIO
    pcnt_unit_t pcnt_unit;                  // PCNT unit to use for counting
    pcnt_channel_t pcnt_channel;            // PCNT channel to use for counting
    uint8_t rmt_gpio;                       // used by RMT to define a sampling window
    rmt_channel_t rmt_channel;              // The RMT channel to use
    uint8_t rmt_clk_div;                    // RMT pulse length, as a divider of the APB clock
    float sampling_period_seconds;          // time (in seconds) between subsequent samples
    float sampling_window_seconds;          // sample window length (in seconds)
    uint16_t filter_length;                 // counter filter length in APB cycles
    void (*frequency_update_callback)(double hz);    // called each time a frequency is determined
} frequency_count_configuration_t;

void frequency_count_task_function(void * pvParameter);

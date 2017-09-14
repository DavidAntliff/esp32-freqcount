/*
 * MIT License
 *
 * Copyright (c) 2017 David Antliff
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "soc/rtc.h"
#include "driver/rmt.h"

#include "esp_log.h"
#include "sdkconfig.h"

#define TAG "freqcount"

#define GPIO_LED             (GPIO_NUM_2)
#define GPIO_FREQ_SIGNAL     (CONFIG_FREQ_SIGNAL_GPIO)
#define GPIO_RMT             (GPIO_NUM_5)

#define PCNT_UNIT 0
#define RMT_TX_CHANNEL RMT_CHANNEL_0
//#define RMT_CLK_DIV          20   // results in 0.25us steps (80MHz / 20 = 4 MHz
#define RMT_CLK_DIV            160   // results in 2us steps (80MHz / 160 = 0.5 MHz
//#define RMT_CLK_DIV          1  // results in 25ns steps (80MHz / 2 / 1 = 40 MHz)

//#define ESP_INTR_FLAG_DEFAULT 0
//
//SemaphoreHandle_t xSemaphore = NULL;
//
//// interrupt service routine
//void IRAM_ATTR isr_handler(void * arg)
//{
//    // notify
//    xSemaphoreGiveFromISR(xSemaphore, NULL);
//}

void led_init(void)
{
    gpio_pad_select_gpio(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);
}

void led_set(bool state)
{
    gpio_set_level(GPIO_LED, state ? 1 : 0);
}

void led_on(void)
{
    gpio_set_level(GPIO_LED, 1);
}

void led_off(void)
{
    gpio_set_level(GPIO_LED, 0);
}

//void task(void * arg)
//{
//    bool led_status = false;
//
//    for (;;) {
//        // wait for notification from ISR
//        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
//        {
//            ESP_LOGI(TAG, "ISR fired!");
//            led_status = !led_status;
//            led_set(led_status);
//        }
//    }
//}

void read_counter_task(void * arg)
{
    while (1)
    {
        // read counter
        int16_t count = 0;
        pcnt_get_counter_value(PCNT_UNIT, &count);
        pcnt_counter_clear(PCNT_UNIT);
        if (count > 0)
        {
            led_on();
        }
        else
        {
            led_off();
        }
        ESP_LOGI(TAG, "counter = %d", count);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}



typedef struct
{
    float period;
} rmt_tx_task_args_t;

void rmt_tx_task(void * arg)
{
    rmt_tx_task_args_t * rmt_tx_task_args = arg;

    bool led = false;
    while (1)
    {
        led_set(led);
        led = !led;

        rmt_item32_t items[100] = { 0 };
        int num_items = 0;

//        // toggle:
//        for (num_items = 0; num_items < 16; ++num_items)
//        {
//            items[num_items].level0 = 0x1 & num_items;
//            items[num_items].duration0 = 1;
//            items[num_items].level1 = ~items[num_items].level0;
//            items[num_items].duration1 = 1;
//        }

        // high for exactly x seconds:
        float x = 1.0;  // seconds
        int32_t total_duration = (uint32_t)(x / rmt_tx_task_args->period);
        ESP_LOGI(TAG, "total_duration %d periods", total_duration);

        // max duration per item is 2^15-1 = 32767
        while (total_duration > 0)
        {
            uint32_t duration = total_duration > 32767 ? 32767 : total_duration;
            items[num_items].level0 = 1;
            items[num_items].duration0 = duration;
            total_duration -= duration;
            ESP_LOGI(TAG, "duration %d", duration);

            if (total_duration > 0)
            {
                uint32_t duration = total_duration > 32767 ? 32767 : total_duration;
                items[num_items].level1 = 1;
                items[num_items].duration1 = duration;
                total_duration -= duration;
            }
            else
            {
                items[num_items].level1 = 0;
                items[num_items].duration1 = 0;
            }
            ESP_LOGI(TAG, "[%d].level0 %d", num_items, items[num_items].level0);
            ESP_LOGI(TAG, "[%d].duration0 %d", num_items, items[num_items].duration0);
            ESP_LOGI(TAG, "[%d].level1 %d", num_items, items[num_items].level1);
            ESP_LOGI(TAG, "[%d].duration1 %d", num_items, items[num_items].duration1);

            ++num_items;
        }
        ESP_LOGI(TAG, "num_items %d", num_items);

        rmt_write_items(RMT_TX_CHANNEL, items, num_items, false);
        rmt_wait_tx_done(RMT_TX_CHANNEL);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
//    xSemaphore = xSemaphoreCreateBinary();

    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "[APP] Startup..");
    led_init();
    led_off();

    // round to nearest MHz (stored value is only precise to MHz)
    uint32_t apb_freq = (rtc_clk_apb_freq_get() + 500000) / 1000000 * 1000000;
    ESP_LOGI(TAG, "APB CLK %u Hz", apb_freq);

    // set Freq signal GPIO to be input only
//    gpio_pad_select_gpio(GPIO_FREQ_SIGNAL);
//    gpio_set_direction(GPIO_FREQ_SIGNAL, GPIO_MODE_INPUT);

//    // Basic GPIO interrupt:
//    // enable interrupt on falling edge for pin
//    gpio_set_intr_type(GPIO_FREQ_SIGNAL, GPIO_INTR_NEGEDGE);
//
//    xTaskCreate(task, "task", 2048, NULL, 10, NULL);
//
//    // install ISR service with default configuration
//    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
//
//    // attach the interrupt service routine
//    gpio_isr_handler_add(GPIO_FREQ_SIGNAL, isr_handler, NULL);

    // set up counter
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = GPIO_FREQ_SIGNAL,
        .ctrl_gpio_num = -1,  // ignored
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_UNIT,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_INC,
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_KEEP,
        .counter_h_lim = 0,
        .counter_l_lim = 0,
    };

    /*Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    // set the GPIO back to hi-impedance, as pcnt_unit_config sets it as pull-up
    gpio_set_pull_mode(GPIO_FREQ_SIGNAL, GPIO_FLOATING);

    // enable counter filter - at 80MHz APB CLK, 1000 pulses is max 80,000 Hz, so ignore pulses less than 12.5 us.
    pcnt_set_filter_value(PCNT_UNIT, 1000);
    pcnt_filter_enable(PCNT_UNIT);

    pcnt_counter_pause(PCNT_UNIT);
    pcnt_counter_clear(PCNT_UNIT);

//    xTaskCreate(read_counter_task, "read_counter_task", 2048, NULL, 10, NULL);

    pcnt_counter_resume(PCNT_UNIT);


    // RMT

    rmt_config_t rmt_tx = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_TX_CHANNEL,
        .gpio_num = GPIO_RMT,
        .mem_block_num = 1,  // 0 is invalid
        .clk_div = RMT_CLK_DIV,
        .tx_config.loop_en = false,
        .tx_config.carrier_en = false,
        .tx_config.idle_level = RMT_IDLE_LEVEL_LOW,
        .tx_config.idle_output_en = true,
    };
    rmt_config(&rmt_tx);
    //rmt_set_source_clk(RMT_TX_CHANNEL, RMT_BASECLK_APB);
    rmt_driver_install(rmt_tx.channel, 0, 0);

    const float RMT_PERIOD = (float)(RMT_CLK_DIV) / 80000000.0;
    ESP_LOGI(TAG, "RMT_PERIOD %.10e", RMT_PERIOD);
    rmt_tx_task_args_t rmt_tx_task_args = {
        .period = RMT_PERIOD,
    };
    xTaskCreate(rmt_tx_task, "rmt_tx_task", 2048, &rmt_tx_task_args, 10, NULL);

    ESP_LOGI(TAG, "[APP] Idle.");
    while(1) ;
}

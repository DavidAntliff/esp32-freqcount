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

#include "esp_log.h"
#include "sdkconfig.h"

#define TAG "freqcount"

#define GPIO_LED             (GPIO_NUM_2)
#define GPIO_FREQ_SIGNAL     (CONFIG_FREQ_SIGNAL_GPIO)

#define PCNT_UNIT 0

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

    xTaskCreate(read_counter_task, "read_counter_task", 2048, NULL, 10, NULL);

    pcnt_counter_resume(PCNT_UNIT);

    ESP_LOGI(TAG, "[APP] Idle.");
}

#include "frequency_count.h"

#define TAG "app"

/* this has been tested with the "16 Mt Bytes (128 Mt bit) Pro ESP32 OLED V2.0 TTGO"
 * #define LCD 0  if you dont need/have the LCD, you will still get the frequency
 * on the serial output
 */
#define BT 0
#if BT
#include "BluetoothSerial.h"
#include <esp_bt.h>
BluetoothSerial serialBT;
#else
#define serialBT Serial
#endif

#define USE_SERIAL Serial

char message[200];

#define LCD 1
#if LCD
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
SSD1306Wire display(0x3c, 4, 15);
#endif

#define CONFIG_FREQ_SIGNAL_GPIO 13
#define CONFIG_SAMPLING_WINDOW_GPIO 2

#define GPIO_SIGNAL_INPUT ((gpio_num_t)CONFIG_FREQ_SIGNAL_GPIO)
#define GPIO_RMT_GATE     (CONFIG_SAMPLING_WINDOW_GPIO)
#define GPIO_LED          (25) //CONFIG_LED_GPIO)

// internal signals for GPIO constant levels
#define GPIO_CONSTANT_LOW   0x30
#define GPIO_CONSTANT_HIGH  0x38

#define PCNT_UNIT         (0)
#define PCNT_CHANNEL      (PCNT_CHANNEL_0)
#define RMT_CHANNEL       (RMT_CHANNEL_0)
#define RMT_MAX_BLOCKS    (2)   // allow up to 2 * 64 * (2 * 32767) RMT periods in window
#define RMT_CLK_DIV       160   // results in 2us steps (80MHz / 160 = 0.5 MHz
//#define RMT_CLK_DIV       20    // results in 0.25us steps (80MHz / 20 = 4 MHz
//#define RMT_CLK_DIV       1     // results in 25ns steps (80MHz / 2 / 1 = 40 MHz)

#define SAMPLE_PERIOD 1.2  // seconds

// The counter is signed 16-bit, so maximum positive value is 32767
// The filter is unsigned 10-bit, maximum value is 1023. Use full period of maximum frequency.
// For higher expected frequencies, the sample period and filter must be reduced.

// suitable up to 16,383.5 Hz
//#define WINDOW_DURATION 1.0  // seconds
//#define FILTER_LENGTH 1023  // APB @ 80MHz, limits to < 39,100 Hz

// suitable up to 163,835 Hz
//#define WINDOW_DURATION 0.1  // seconds
//#define FILTER_LENGTH 122  // APB @ 80MHz, limits to < 655,738 Hz

// suitable up to 1,638,350 Hz
//#define WINDOW_DURATION 0.01  // seconds
//#define FILTER_LENGTH 12  // APB @ 80MHz, limits to < 3,333,333 Hz

// suitable up to 16,383,500 Hz - no filter
//#define WINDOW_DURATION 0.001  // seconds
//#define FILTER_LENGTH 1  // APB @ 80MHz, limits to < 40 MHz

// suitable up to 163,835,000 Hz - no filter
#define WINDOW_DURATION 0.0001  // seconds
#define FILTER_LENGTH 0  // APB @ 80MHz, limits to < 40 MHz


static void window_start_callback(void)
{
    ESP_LOGI(TAG, "Begin sampling");
    gpio_matrix_in(GPIO_SIGNAL_INPUT, SIG_IN_FUNC228_IDX, false);
}

static volatile double frequency;
static void frequency_callback(double hz)
{
    gpio_matrix_in(GPIO_CONSTANT_LOW, SIG_IN_FUNC228_IDX, false);
    frequency=hz;
    ESP_LOGI(TAG, "Frequency %f Hz", hz);
}

static void config_led(void)
{
    gpio_pad_select_gpio(GPIO_LED);
    gpio_set_direction((gpio_num_t)GPIO_LED, GPIO_MODE_OUTPUT);

    // route incoming frequency signal to onboard LED when sampling
    gpio_matrix_out(GPIO_LED, SIG_IN_FUNC228_IDX, false, false);
}

void setup()
{
#ifdef USE_SERIAL
  USE_SERIAL.begin(115200);
  Serial.println("Frequency counter starting");
  delay(500);
#endif
    /*
  * LCD
  */
#if LCD
  pinMode(16,OUTPUT); 
  digitalWrite(16,LOW); 
  delay(50); 
  digitalWrite(16,HIGH); 
  // Initialising the UI will init the display too.
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0,50,"counter ready");
    display.display();
#endif
  
  config_led();

    frequency_count_configuration_t * config = (frequency_count_configuration_t*)malloc(sizeof(*config));
    config->pcnt_gpio = GPIO_SIGNAL_INPUT;
    config->pcnt_unit = (pcnt_unit_t)PCNT_UNIT;
    config->pcnt_channel = PCNT_CHANNEL;
    config->rmt_gpio = (gpio_num_t)GPIO_RMT_GATE;
    config->rmt_channel = RMT_CHANNEL;
    config->rmt_clk_div = RMT_CLK_DIV;
    config->rmt_max_blocks = 2;
    config->sampling_period_seconds = SAMPLE_PERIOD;
    config->sampling_window_seconds = WINDOW_DURATION;
    config->filter_length = FILTER_LENGTH;
    config->window_start_callback = &window_start_callback;
    config->frequency_update_callback = &frequency_callback;

    // task takes ownership of allocated memory
    xTaskCreate(&frequency_count_task_function, "frequency_count_task", 4096, config, 5, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
#if LCD
  display.clear();
  sprintf(message,"F=%9.0f Hz",frequency);
  display.drawString(0,10,message);
  display.display();
#endif
#ifdef USE_SERIAL
  Serial.println(message);
#endif

  delay(100);
}

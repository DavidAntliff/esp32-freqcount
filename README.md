# esp32-freqcount

ESP32 Application to accurately count frequency of pulses on a GPIO using Pulse Counter, RMT and Interrupt.

The RMT generates a pulse of precise length, which is mapped to a GPIO.

The Pulse Counter (PCNT) control input is wired to this same GPIO.

PCNT is configured to inhibit counting when the control input is low, and count when it is high.

The RMT TX Complete interrupt is used to fetch the counter value at the end of each control frame.

See also [ref_clock](https://github.com/espressif/esp-idf/blob/master/tools/unit-test-app/components/unity/ref_clock.c).




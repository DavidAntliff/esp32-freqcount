# esp32-freqcount

## Introduction

This ESP32 component facilitates accurately measuring the frequency of square pulses on a GPIO using Pulse Counter, RMT and Interrupt.

The RMT generates a pulse of precise length, which is mapped to a GPIO. This is the sampling window signal.

The Pulse Counter (PCNT) control input is wired to this same GPIO. PCNT is configured to inhibit counting when the control input is low, and count when it is high.

Both rising and falling edges of the incoming signal of interest are counted during the sampling window.

The RMT TX Complete interrupt is used to fetch the counter value at the end of each control frame.

The sampling window duration and counter value are used to calculate the average frequency of the signal in Hertz.

## Dependencies

It is written and tested for the [ESP-IDF](https://github.com/espressif/esp-idf) environment, using the xtensa-esp32-elf toolchain (gcc version 5.2.0).

## Example

An example application that uses this component is available: [esp32-freqcount-example](https://github.com/DavidAntliff/esp32-freqcount-example)

## Features

* Measurement of average frequency of input signal in Hertz.

## Documentation

Automatically generated API documentation is available [here](https://davidantliff.github.io/esp32-freqcount/index.html).

## Configuration

In order to accurately measure a frequency signal of interest, it is important to consider the expected frequency range of the signal.
The general rule is: the slower the expected frequency, the longer the sampling window needs to be to get an accurate measurement.

For example, a ~10 Hz input signal measured over 10 seconds will only be accurate to +/- 0.05 Hz. However a ~10,000 Hz signal measured over a 0.1 second window will be accurate to +/- 5 Hz.

In addition, the onboard peripheral that generates the sampling window (RMT) is constrained by the system clock and a programmable divider.
The duration of the window is specified by an array of "items" that are processed in order. Each item describes the number of RMT periods (after the divider) to hold the window signal high or low. Describing RMT operation is outside of the scope of this document, however it is sufficient to know that:

 * RMT programming consists of a block of instructions, called "items",
 * Each block contains 64 items,
 * There are up to 8 blocks of items,
 * Each item can encode up to 2 * 32767 = 65534 periods of the window

Therefore, for a APB clock of 80 MHz and an RMT divider of 160, the RMT period will be 1 / (80 MHz / 160) = 1 / 500,000 seconds.
A fully-employed item will therefore represent 65534 / 500,000 = 0.1311 seconds of window. So it follows that a 10 second sampling window will need 10.0 / 0.1311 = ~76 items. Since each block contains 64 items, this will require two blocks.

The user specifies the maximum number of blocks that this component is allowed to use. If the sampling window requirements exceed this maximum, an assertion will fire.

In addition to the sampling window, there is also a filter that can be configured to reduce glitches on the counter input. The length of this filter must be carefully specified otherwise it can mask genuine edges. The filter length is specified as the number of periods at APB clock rate (80 MHz) that changes in the PCNT input signal will be ignored. For example, a value of 1023 will limit square-wave inputs to a maxmum frequency of 80 MHz / 1023 / 2 (because there are two edges per cycle!) = 39,100 Hz.

## Source Code

The source is available from [GitHub](https://www.github.com/DavidAntliff/esp32-freqcount).

## License

The code in this project is licensed under the MIT license - see LICENSE for details.

## Links

 * [esp32-freqcount-example](https://github.com/DavidAntliff/esp32-freqcount-example)
 * [Espressif IoT Development Framework for ESP32](https://github.com/espressif/esp-idf)

## Acknowledgements

Thank you to [Chris Morgan](https://github.com/chmorgan) for converting the original demo application into this reusable IDF component.




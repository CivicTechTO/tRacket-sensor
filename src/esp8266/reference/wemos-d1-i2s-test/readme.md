# Wemos D1 Mini - I2s Microphone Tests

Original repo: https://github.com/joextodd/listener

Microphone: SPH0645

## Connections:

| Mic  | Board |
| ---- | ----- |
| GND  | GND   |
| BCLK | D7    |
| DOUT | D6    |
| LRCL | D5    |
| SEL  | N/C   |
| 3V   | 3V    |

## CGPT Answer:

The discrepancy in readings between the ESP8266 and ESP32 code could be attributed to the difference in the way the two platforms handle I2S data, specifically in terms of signedness and bit alignment.

In the ESP8266 code, the convert function is used to process I2S data:

```cpp
#define convert(sample) (((int32_t)(sample) >> 13) - 240200)
```

This function right-shifts the 32-bit signed sample by 13 bits and then subtracts a calibration value. The right-shift operation effectively discards the 13 least significant bits. The ESP8266 uses 15 bits for data, so after the shift, only the upper 18 bits of the 32-bit sample are retained.

On the other hand, the ESP32 code reads I2S data directly into a float array:

```cpp
i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(SAMPLE_T), &bytes_read, portMAX_DELAY);
```

The ESP32 code reads the full 32-bit signed data without any bit manipulation or calibration.

To bring the ESP8266 readings in line with the ESP32 code, you can modify the convert function or the way you handle the I2S data in the ESP8266 code. Here's a modified version of the convert function:

```cpp
#define convert(sample) ((float)(sample) / (1 << 24))  // Assuming 24-bit data
```

This modification converts the 32-bit signed sample to a float by dividing it by 2^24. This assumes that the I2S data is 24 bits, as indicated by the I2S_24BIT constant in the ESP8266 code. The resulting float value should be more comparable to the values obtained in the ESP32 code.

Please note that the actual bit depth of the I2S data may vary based on your hardware configuration. Make sure to adjust the divisor accordingly if your I2S data is different from 24 bits.

## Possible reference matrial:

https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-reference/peripherals/i2s.html

# ESP32 Breadboard prototype

![ESP32-WROOM Pinout](media\ESP-WROOM-32-Pinout.jpg)

Board used (ESP32-WROOM): [https://a.co/d/6XwJp64](https://a.co/d/6XwJp64)

Mic module used (SPH0645LM4H): [https://www.adafruit.com/product/3421](https://www.adafruit.com/product/3421)

## Pin Connections:

| Mic  | Board |
| ---- | ----- |
| GND  | GND   |
| BCLK | P23   |
| DOUT | P19   |
| LRCL | P18   |
| SEL  | GND   |
| 3V   | 3V    |

| Misc  | Board |
| ----- | ----- |
| Reset | P5    |

> IMPORTANT - The I2S protocol is a 2-channel data stream. The SEL pin on the mic must be pulled LOW (to GND) in order to work with the example code, which reads data from the RIGHT CHANNEL only. Alternatively, the I2S driver can be set to read from left channel, although it remains unclear whether this would involve tying the SEL pin to 3.3V. If you get no appreciable readings (or all readings are the same as the MIC_NOISE_DB variable) then you may be reading the wrong channel, or you may need to check the SEL pin!


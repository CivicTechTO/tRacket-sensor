# ESP32 V1 Code

![ESP32-WROOM Pinout](media\ESP-WROOM-32-Pinout.jpg)

Board used (ESP32-WROOM): [https://a.co/d/6XwJp64](https://a.co/d/6XwJp64)

Mic module used (SPH0645LM4H): [https://www.adafruit.com/product/3421](https://a.co/d/6XwJp64)

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

## Before running:

- Update SSL certificate using the following command:
```bash
python cert.py -s noisemeter.webcomand.com -n webcomand
```
Then copy the console output to certs.h.

- Create secret.h file with thre following contents:
```c
const char* API_TOKEN = "<Your API token here>";
```
(Replace '<Your API token here>' with your API token)

## Operating Instructions:

- Power on the device by connecting it to power. The device should start in Hotspot mode.
- Connect to device's wifi hotspot, which will be called "Noise Meter". Password is "noisemeter".
- Open a web browser and navigate to IP address 4.3.2.1 and fill out your Wifi network SSID and password. This will then bbe saved to the onboard flash, and should persist between restarts.
- The device should reset and begin attempting to connect to wifi. Use Serial monitor to confirm this.
- Once connected, the device will sync itself with a NTP time server and then begin taking readings.
- The device fills a buffer with integer readings from the microphone until it has enough to 
- Periodically (at a time interval determined by UPLOAD_INTERVAL_MS) the device will collate its data and upload it to the server, whereupon it will clear its cached data and begin taking readings again.
- To clear your saved information, hold the Reset button (Pin 5 in this example) while you power on the device, or hold the Reset button and reset the device using the built-in reset button.

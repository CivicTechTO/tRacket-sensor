# ESP32C3 Code

This is the firmware for the official Noisemeter device:

![Noisemeter device](https://raw.githubusercontent.com/CivicTechTO/proj-noisemeter-device/main/src/esp32c3/media/noisemeter.jpg)

## Before running:

1. Update SSL certificate using the following command:
```bash
python cert.py -s noisemeter.webcomand.com -n webcomand
```
Then copy the console output to certs.h.

2. Create secret.h file with the following contents:
```c
const char* API_TOKEN = "<Your API token here>";
```
(Replace '<Your API token here>' with your API token)

3. Edit the `.ino` file and set your own `DEVICE_ID`.

4. Follow the [wiki page](https://github.com/CivicTechTO/proj-noisemeter-device/wiki/Device-Preparation) to program your device.

## Operating Instructions:

1. Power on the device through the USB port. If the device has not been set up before or has gone through a factory reset, a WiFi hotspot named "Noise meter" will be created. Password is "noisemeter".
2. Open a web browser and navigate to `http://4.3.2.1` and fill out your WiFi network SSID and password. This will be saved to the onboard flash and the device will reboot.
3. Once the device is connected to WiFi, it will sync its time with an NTP server and begin to take readings. You can use a serial monitor to confirm this.
4. Once the device has enough noise samples or once enough time has passed, the device will upload its measurement data to the server whereupon it will clear its cached data and begin taking readings again.

To clear your saved information (WiFi credentials), hold the Reset button down while you power on the device.

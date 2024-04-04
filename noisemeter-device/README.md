# Build Instructions

## Source code preparation

1. Update SSL certificate using the following command:
```bash
python cert.py -s noisemeter.webcomand.com -n webcomand
```
Then copy the console output to certs.h.

2. Create secret.h file with thre following contents:
```c
const char* API_TOKEN = "Your API token here";
```
(Replace "Your API token here" with your API token)

3. Copy `config.h.example` to `config.h` and edit the file to select your board type and set your device ID.

## Code compiling and upload

### PlatformIO

1. [Install PlatformIO](https://platformio.org/install).

2. Run `pio run` to compile for the PCB. A breadboard target is available too: `pio run -e esp32-breadboard`.

3. Run `pio run -t upload` to upload to the device (this also compiles the code if there have been any changes).

### Arduino

1. Install the Arduino IDE and [follow these instructions](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) to add support for ESP32 microcontrollers.

2. Under "Tools" > "Board: " > "ESP32 Arduino", select either "ESP32C3 Dev Module" for the PCB boards or "ESP32-WROOM-DA Module" for the ESP32 breadboard prototype.

3. Compile the sketch and upload it to the device.

## HMAC encryption key

Data stored on the device (e.g. WiFi credentials) are encrypted with an "eFuse" key. This key can only be written once, and is not be read or written after that. 

Using PlatformIO:

```bash
dd if=/dev/urandom of=hmac_key bs=1 count=32
pio pkg exec -- espefuse.py --port /dev/ttyACM0 burn_key BLOCK4 hmac_key HMAC_UP
```

# Operating Instructions:

- Power on the device by connecting it to power. The device should start in Hotspot mode.
- Connect to device's wifi hotspot, which will be called "Noise Meter". Password is "noisemeter".
- Open a web browser and navigate to IP address 4.3.2.1 and fill out your Wifi network SSID and password. This will then bbe saved to the onboard flash, and should persist between restarts.
- The device should reset and begin attempting to connect to wifi. Use Serial monitor to confirm this.
- Once connected, the device will sync itself with a NTP time server and then begin taking readings.
- The device fills a buffer with integer readings from the microphone until it has enough to 
- Periodically (at a time interval determined by UPLOAD_INTERVAL_MS) the device will collate its data and upload it to the server, whereupon it will clear its cached data and begin taking readings again.
- To clear your saved information, hold the Reset button (Pin 5 in this example) while you power on the device, or hold the Reset button and reset the device using the built-in reset button.


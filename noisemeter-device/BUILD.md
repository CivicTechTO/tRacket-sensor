# Build Instructions

## Source code preparation

1. Save the SSL root certificate to `certs.h` using the following command:
```bash
python certs.py -s api.tracket.info > certs.h
```

2. Copy `config.h.example` to `config.h`; if compiling with the Arduino IDE, edit the file to select your board type.

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

## Operation Overview:

* After initial programming or a factory reset, the device will enter Hotspot mode once it is powered on. This is indicated by a blinking LED.
* In Hotspot mode, you can connect to the device's "Noise meter" WiFi network to set it up. Password is "noisemeter".
* Once connected, a captive portal will bring you to a form to enter your WiFi credentials and email for registration. Your credentials will be encrypted and stored to the on-board flash memory.
* After completing the form, the device will reboot and attempt to connect to your WiFi.
* Once connected, the device will sync itself with a NTP time server and then begin taking readings.
* The device takes 100-millisecond audio recordings and instantly converts them to decibel "loudness" values. Each recording is discarded as soon as it is processed.
* Over time, the decibel values are aggregated into minimum, average, and maximum values. Periodically, these values will be uploaded to the server at which point the device will clear its cached data and begin taking a new set of readings.
* To factory reset the device and clear your saved credentials, hold the Reset button while you power on the device until the LED begins blinking.


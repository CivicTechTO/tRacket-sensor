# Build Instructions

## Source code preparation

1. Save the SSL root certificate to `certs.h` using the following command:
```bash
python certs.py -s api.tracket.info > certs.h
```

## Code compilation and upload

1. [Install PlatformIO](https://platformio.org/install).

2. Run `pio run` to compile for the PCB. A breadboard target is available too: `pio run -e esp32-breadboard`.

3. Run `pio run -t upload` to upload to the device (this also compiles the code if there have been any changes).

## HMAC encryption key

Data stored on the device (e.g. WiFi credentials) are encrypted with an "eFuse" key. This key can only be configured once, and cannot be read or written after that. 

```bash
dd if=/dev/urandom of=hmac_key bs=1 count=32
pio pkg exec -- espefuse.py --port /dev/ttyACM0 burn_key BLOCK4 hmac_key HMAC_UP
rm hmac_key
```

This is done in the `bringup.sh` script that is used to program new sensors.

**Please generate a unique hmac_key for each device.**

## Enable secure download mode

Enabling secure download mode prevents users from using USB/serial download mode to dump memory contents (WiFi credentials and API token):

```bash
pio pkg exec -- espefuse.py --port /dev/ttyACM0 burn_efuse ENABLE_SECURE_DOWNLOAD
```

Then, if new firmware is to be uploaded over USB in the future, upload using this command:

```bash
pio pkg exec -- esptool.py write_flash 0x10000 .pio/build/esp32-pcb/firmware.bin
```

## Signing OTA updates

OTA updates must be signed for deployed tRacket sensors to accept them. The
GitHub repo is configured to automatically sign firmware updates when releases
are published.

Signing requires a 4096-bit RSA key. To sign an update (assuming you have the
private key), run `pio run -t ota`.

The public key is to be stored in `noisemeter_device/ota_update.cpp`. To obtain
the public key (assuming you have the private key), run:

```bash
openssl rsa -in priv_key.pem -pubout > rsa_key.pub
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


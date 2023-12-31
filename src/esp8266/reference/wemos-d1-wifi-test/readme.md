# Wemos D1 Wifi & Audio Tests

## Note:

You will need to add an extra file called `secret.h`, which contains the following variables:

```c
const char* ssid = "<YOUR WIFI NETWORK SSID>";
const char* password = "<YOUR WIFI PASSWORD>";
const char* token = "Token <YOUR API TOKEN>";
const char fingerprint[] PROGMEM = "<SHA1 FINGERPRINT>";
```

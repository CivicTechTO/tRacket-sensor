# Wemos D1 SSL Tests

## Update certs using:

```bash
python cert.py -s noisemeter.webcomand.com -n webcomand
```

Then copy terminal output to certs.h

## secret.h:

```c
const char* NETWORK_SSID = "<NETWORK_NAME>";
const char* NETWORK_PASSWORD = "<NETWORK_PASSWORD>";
const char* API_TOKEN = "<API_TOKEN>";
```

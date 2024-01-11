/**
 * Civic Tech TO - Noisemeter Device (ESP32 Version) v1
 * Written by Clyne Sullivan and Nick Barnard.
 * Open source dB meter code taken from Ivan Kostoski (https://github.com/ikostoski/esp32-i2s-slm)
 * 
 * TODO:
 *  - Use DNS to make a "captive portal" that brings users directly to the credentials form.
 *  - Encrypt the stored credentials (simple XOR with a long key?).
 *  - Add second step to Access Point flow - to gather users email, generate a UUID and upload them to the cloud. UUID to be saved in EEPROM
 *  - Add functionality to reset the device periodically (eg every 24 hours)
 *  - 
 */
#include <ArduinoJson.h>  // https://arduinojson.org/
#include <ArduinoJson.hpp>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <CRC32.h>  // https://github.com/bakercp/CRC32
#include <EEPROM.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <dummy.h>       // ESP32 core
#include <driver/i2s.h>  // ESP32 core

#include "sos-iir-filter.h"
#include "certs.h"
#include "secret.h"

#include <cstdint>

WiFiMulti WiFiMulti;

constexpr auto AccessPointSSID = "Noise meter";
constexpr auto AccessPointPsk = "noisemeter";
const bool UPLOAD_DISABLED = false;
const unsigned long UPLOAD_INTERVAL_MS = 60000 * 5;  // Upload every 5 mins
// const unsigned long UPLOAD_INTERVAL_MS = 30000;  // Upload every 30 secs
const String DEVICE_ID = "nickjan2024int";  // TODO EPROM
const unsigned long MIN_READINGS_BEFORE_UPLOAD = 20;

// The ESP32's IP address within its access point will be "4.3.2.1".
// Once connected to the access point, open up 4.3.2.1 in a browser to get
// to the credentials form.
static const IPAddress AccessPointIP(4, 3, 2, 1);

//
// Constants & Config
//
#define LEQ_PERIOD 1         // second(s)
#define WEIGHTING A_weighting  // Also avaliable: 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS "LAeq"       // customize based on above weighting used
#define DB_UNITS "dBA"         // customize based on above weighting used
#define USE_DISPLAY 0

// NOTE: Some microphones require at least DC-Blocker filter
#define MIC_EQUALIZER SPH0645LM4H_B_RB  // See below for defined IIR filters or set to 'None' to disable
#define MIC_OFFSET_DB 3.0103            // Default offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration

// Customize these values from microphone datasheet
#define MIC_SENSITIVITY -26    // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
#define MIC_REF_DB 94.0        // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_OVERLOAD_DB 120.0  // dB - Acoustic overload point
#define MIC_NOISE_DB 29        // dB - Noise floor
#define MIC_BITS 24            // valid number of bits in I2S data
#define MIC_CONVERT(s) (s >> (SAMPLE_BITS - MIC_BITS))
#define MIC_TIMING_SHIFT 0  // Set to one to fix MSB timing for some microphones, i.e. SPH0645LM4H-x

// Calculate reference amplitude value at compile time
constexpr double MIC_REF_AMPL = pow(10, double(MIC_SENSITIVITY) / 20) * ((1 << (MIC_BITS - 1)) - 1);

//
// Sampling
//
#define SAMPLE_RATE 48000  // Hz, fixed to design of IIR filters
#define SAMPLE_BITS 32     // bits
#define SAMPLE_T int32_t
#define SAMPLES_SHORT (SAMPLE_RATE / 8)  // ~125ms
#define SAMPLES_LEQ (SAMPLE_RATE * LEQ_PERIOD)
#define DMA_BANK_SIZE (SAMPLES_SHORT / 16)
#define DMA_BANKS 32

// Data we push to 'samples_queue'
struct sum_queue_t {
  // Sum of squares of mic samples, after Equalizer filter
  float sum_sqr_SPL;
  // Sum of squares of weighted mic samples
  float sum_sqr_weighted;
};

// Static buffer for block of samples
float samples[SAMPLES_SHORT] __attribute__((aligned(4)));

// Grounding this pin (e.g. with a button) will force access open to start.
// Useful as a "reset" button to overwrite currently saved credentials.
#define CredsResetPin 5
//
// I2S pins - Can be routed to almost any (unused) ESP32 pin.
//            SD can be any pin, inlcuding input only pins (36-39).
//            SCK (i.e. BCLK) and WS (i.e. L/R CLK) must be output capable pins
//
// Below ones are just example for my board layout, put here the pins you will use
//
#define I2S_WS 18
#define I2S_SCK 23
#define I2S_SD 19
// I2S peripheral to use (0 or 1)
#define I2S_PORT I2S_NUM_0

// EEPROM addresses for credential data.
constexpr unsigned int EEPROMMaxStringSize = 64;
constexpr int EEPROMEntryCSum = 0;  // CRC32 checksum of SSID and passkey
constexpr int EEPROMEntrySSID = EEPROMEntryCSum + sizeof(uint32_t);
constexpr int EEPROMEntryPsk = EEPROMEntrySSID + EEPROMMaxStringSize;
constexpr int EEPROMTotalSize = EEPROMEntryPsk + EEPROMMaxStringSize;

// Not sure if WiFiClientSecure checks the validity date of the certificate.
// Setting clock just to be sure...
void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

// Main webpage HTML with form to collect WiFi credentials.
static const char SetupPageHTML[] PROGMEM = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
  <meta charset='utf-8'>
  <meta name='viewport' content='width=device-width,initial-scale=1'/>
</head>
<body>
<h1>Noise Meter Setup</h1>
<form method='POST' action='' enctype='multipart/form-data'>
<p>SSID:</p>
<input type='text' name='ssid'>
<p>Password:</p>
<input type='password' name='psk'>
<input type='submit' value='Connect'>
</form>
</body>
</html>
)";

// HTML to show after credentials are submitted.
static const char SubmitPageHTML[] PROGMEM = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
  <meta charset='utf-8'>
  <meta name='viewport' content='width=device-width,initial-scale=1'/>
</head>
<body>
<h1>Noise Meter Setup</h1>
<p>Connecting...</p>
</body>
</html>
)";

/**
 * Starts the WiFi access point and web server and enters an infinite loop to process connections.
 */
[[noreturn]] static void runAccessPoint();

/**
 * Calculates a CRC32 checksum based on the given SSID and passkey.
 * Checksums are used to determine if the credentials in EEPROM are valid.
 */
static uint32_t calculateCredsChecksum(const String& ssid, const String& psk);

/**
 * Prints the SSID and passkey to Serial.
 */
static void printCredentials(const String& ssid, const String& psk);

/**
 * Saves credentials to EEPROM is form was properly submitted.
 * If successful, reboots the microcontroller.
 */
static void saveNetworkCreds(WebServer& httpServer);

/**
 * Returns true if the credentials stored in EEPROM are valid.
 */
static bool isEEPROMCredsValid();

/**
 * Returns true if the "reset" button is pressed, meaning the user wants to input new credentials.
 */
static bool isCredsResetPressed();

// Sampling Buffers & accumulators
sum_queue_t q;
uint32_t Leq_samples = 0;
double Leq_sum_sqr = 0;
double Leq_dB = 0;
size_t bytes_read = 0;

// Noise Level Readings
int numberOfReadings = 0;
float minReading = MIC_OVERLOAD_DB;
float maxReading = MIC_NOISE_DB;
float sumReadings = 0;
unsigned long lastUploadMillis = 0;

/**
 * Initialization routine.
 */
void setup() {
  // If needed, now you can actually lower the CPU frquency,
  // i.e. if you want to (slightly) reduce ESP32 power consumption
  // setCpuFrequencyMhz(80);  // It should run as low as 80MHz

  Serial.begin(115200);
  Serial.println();
  Serial.println("Initializing...");

  EEPROM.begin(EEPROMTotalSize);
  delay(2000);  // Ensure the EEPROM peripheral has enough time to initialize.

  initMicrophone();

  if (!UPLOAD_DISABLED) {
    // Run the access point if it is requested or if there are no valid credentials.
    if (isCredsResetPressed() || !isEEPROMCredsValid()) {
      eraseNetworkCreds();
      runAccessPoint();
    }

    // Valid credentials: Next step would be to connect to the network.
    const auto ssid = EEPROM.readString(EEPROMEntrySSID);
    const auto psk = EEPROM.readString(EEPROMEntryPsk);

    Serial.print("Ready to connect to ");
    printCredentials(ssid, psk);

    int ssid_len = ssid.length() + 1;
    int psk_len = psk.length() + 1;

    char intSSID[ssid_len];
    char intPSK[psk_len];
    ssid.toCharArray(intSSID, ssid_len);
    psk.toCharArray(intPSK, psk_len);

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(intSSID, intPSK);


    // wait for WiFi connection
    Serial.print("Waiting for WiFi to connect...");
    while ((WiFiMulti.run() != WL_CONNECTED)) {
      Serial.print(".");
      delay(500);
    }
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());

    setClock();
  }
}

void loop() {


  readMicrophoneData();

  if (canUploadData()) {
    WiFiClientSecure* client = new WiFiClientSecure;
    float average = sumReadings / numberOfReadings;
    String payload = createJSONPayload(DEVICE_ID, minReading, maxReading, average);
    uploadData(client, payload);
    delete client;
  };
}

void runAccessPoint() {
  static WebServer httpServer(80);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AccessPointIP, AccessPointIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AccessPointSSID, AccessPointPsk);

  // GET request means user wants to see the form.
  // POST request means user has submitted data through the form.
  httpServer.on("/", HTTP_GET,
                [] {
                  httpServer.send_P(200, PSTR("text/html"), SetupPageHTML);
                });
  httpServer.on("/", HTTP_POST,
                [] {
                  // Show "submitted" page immediately before we begin saving the credentials.
                  httpServer.client().setNoDelay(true);
                  httpServer.send_P(200, PSTR("text/html"), SubmitPageHTML);
                  saveNetworkCreds(httpServer);
                });
  httpServer.begin();

  Serial.println("Running setup access point.");

  while (1)
    httpServer.handleClient();
}

void printCredentials(const String& ssid, const String& psk) {
  Serial.print("SSID \"");
  Serial.print(ssid);
  Serial.print("\" passkey \"");
  Serial.print(psk);
  Serial.println("\"");
}

void printArray(float arr[], unsigned long length) {
  Serial.print(length);
  Serial.print(" {");
  for (int i = 0; i < length; i++) {
    Serial.print(arr[i]);
    if (i < length - 1) Serial.print(", ");
  }
  Serial.println("}");
}

void printReadingToConsole(double reading) {
  String output = "";
  output += reading;
  output += "dB";
  if (numberOfReadings > 1) {
    output += " [+" + String(numberOfReadings - 1) + " more]";
  }
  Serial.println(output);
}

uint32_t calculateCredsChecksum(const String& ssid, const String& psk) {
  CRC32 crc;
  crc.update(ssid.c_str(), ssid.length());
  crc.update(psk.c_str(), psk.length());
  return crc.finalize();
}

bool isEEPROMCredsValid() {
  const auto csum = EEPROM.readUInt(EEPROMEntryCSum);
  const auto ssid = EEPROM.readString(EEPROMEntrySSID);
  const auto psk = EEPROM.readString(EEPROMEntryPsk);

  Serial.print("EEPROM stored credentials: ");
  printCredentials(ssid, psk);

  return !ssid.isEmpty() && !psk.isEmpty() && csum == calculateCredsChecksum(ssid, psk);
}

bool isCredsResetPressed() {
  pinMode(CredsResetPin, INPUT_PULLUP);
  delay(100);  // Let the IO circuit settle.
  Serial.println();
  Serial.print("Is reset detected: ");
  Serial.println(!digitalRead(CredsResetPin));
  return !digitalRead(CredsResetPin);
}

void saveNetworkCreds(WebServer& httpServer) {
  // Confirm that the form was actually submitted.
  if (httpServer.hasArg("ssid") && httpServer.hasArg("psk")) {
    const auto ssid = httpServer.arg("ssid");
    const auto psk = httpServer.arg("psk");

    // Confirm that the given credentials will fit in the allocated EEPROM space.
    if (ssid.length() < EEPROMMaxStringSize && psk.length() < EEPROMMaxStringSize) {
      EEPROM.writeUInt(EEPROMEntryCSum, calculateCredsChecksum(ssid, psk));
      EEPROM.writeString(EEPROMEntrySSID, ssid);
      EEPROM.writeString(EEPROMEntryPsk, psk);
      EEPROM.commit();

      Serial.print("Saving ");
      printCredentials(ssid, psk);

      Serial.println("Saved network credentials. Restarting...");
      delay(2000);
      ESP.restart();  // Software reset.
    }
  }

  // TODO inform user that something went wrong...
  Serial.println("Error: Invalid network credentials!");
}

void eraseNetworkCreds() {
  Serial.println("Erasing stored credentials...");
  EEPROM.writeUInt(EEPROMEntryCSum, calculateCredsChecksum("", ""));
  EEPROM.writeString(EEPROMEntrySSID, "");
  EEPROM.writeString(EEPROMEntryPsk, "");
  EEPROM.commit();
  Serial.println("Erase complete");

  const auto ssid = EEPROM.readString(EEPROMEntrySSID);
  const auto psk = EEPROM.readString(EEPROMEntryPsk);
  Serial.print("Stored Credentials after erasing: ");
  printCredentials(ssid, psk);
  delay(2000);
}

String createJSONPayload(String deviceId, float min, float max, float average) {
  DynamicJsonDocument doc(2048);
  doc["parent"] = "/Bases/nm1";
  doc["data"]["type"] = "comand";
  doc["data"]["version"] = "1.0";
  doc["data"]["contents"][0]["Type"] = "Noise";
  doc["data"]["contents"][0]["Min"] = min;
  doc["data"]["contents"][0]["Max"] = max;
  doc["data"]["contents"][0]["Mean"] = average;
  doc["data"]["contents"][0]["DeviceID"] = deviceId;  // TODO

  // Serialize JSON document
  String json;
  serializeJson(doc, json);
  return json;
}

bool canUploadData() {
  // DEBUG SWITCH - If true, will skip upload and continue accumulating data
  if (UPLOAD_DISABLED) return false;

  // Has it been at least the upload interval since we uploaded data?
  long now = millis();
  long msSinceLastUpload = now - lastUploadMillis;
  if (msSinceLastUpload < UPLOAD_INTERVAL_MS) return false;

  // Do we have the minimum number of readings stored to form a resonable average?
  if (numberOfReadings < MIN_READINGS_BEFORE_UPLOAD) return false;
  return true;
}

void uploadData(WiFiClientSecure* client, String json) {
  if (client) {
    client->setCACert(cert_ISRG_Root_X1);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;
      // void addHeader(const String& name, const String& value, bool first = false, bool replace = true);

      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, "https://noisemeter.webcomand.com/ws/put")) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header


        // void addHeader(const String& name, const String& value, bool first = false, bool replace = true);
        https.addHeader("Authorization", String("Token ") + API_TOKEN);
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Content-Length", String(json.length()));
        https.addHeader("User-Agent", "ESP32");

        int httpCode = https.POST(json);
        // int POST(uint8_t * payload, size_t size);
        // int POST(String payload);

        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }

    // delete client;
  } else {
    Serial.println("Unable to create client");
  }

  long now = millis();
  lastUploadMillis = now;
  resetReading();
};  // Given a serialized JSON payload, upload the data to webcomand

void resetReading() {
  Serial.println("Resetting readings cache...");
  numberOfReadings = 0;
  minReading = MIC_OVERLOAD_DB;
  maxReading = MIC_NOISE_DB;
  sumReadings = 0;
  Serial.println("Reset complete");
};

//
// I2S Microphone sampling setup
//
void mic_i2s_init() {
  // Setup I2S to sample mono channel for SAMPLE_RATE * SAMPLE_BITS
  // NOTE: Recent update to Arduino_esp32 (1.0.2 -> 1.0.3)
  //       seems to have swapped ONLY_LEFT and ONLY_RIGHT channels
  const i2s_config_t i2s_config = {
    mode: i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate: SAMPLE_RATE,
    bits_per_sample: i2s_bits_per_sample_t(SAMPLE_BITS),
    channel_format: I2S_CHANNEL_FMT_ONLY_RIGHT,
    communication_format: i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
    dma_buf_count: DMA_BANKS,
    dma_buf_len: DMA_BANK_SIZE,
    use_apll: true,
    tx_desc_auto_clear: false,
    fixed_mclk: 0
  };
  // I2S pin mapping
  const i2s_pin_config_t pin_config = {
    bck_io_num: I2S_SCK,
    ws_io_num: I2S_WS,
    data_out_num: -1,  // not used
    data_in_num: I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

#if (MIC_TIMING_SHIFT > 0)
  // Undocumented (?!) manipulation of I2S peripheral registers
  // to fix MSB timing issues with some I2S microphones
  REG_SET_BIT(I2S_TIMING_REG(I2S_PORT), BIT(9));
  REG_SET_BIT(I2S_CONF_REG(I2S_PORT), I2S_RX_MSB_SHIFT);
#endif

  i2s_set_pin(I2S_PORT, &pin_config);

  //FIXME: There is a known issue with esp-idf and sampling rates, see:
  //       https://github.com/espressif/esp-idf/issues/2634
  //       In the meantime, the below line seems to set sampling rate at ~47999.992Hz
  //       fifs_req=24576000, sdm0=149, sdm1=212, sdm2=5, odir=2 -> fifs_reached=24575996
  //NOTE:  This seems to be fixed in ESP32 Arduino 1.0.4, esp-idf 3.2
  //       Should be safe to remove...
  //#include <soc/rtc.h>
  //rtc_clk_apll_enable(1, 149, 212, 5, 2);
}

void initMicrophone() {
  mic_i2s_init();
  // Discard first block, microphone may have startup time (i.e. INMP441 up to 83ms)
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);
  // Equivalent to: i2s_read(0, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);
}

void readMicrophoneData() {
  // Block and wait for microphone values from I2S
  //
  // Data is moved from DMA buffers to our 'samples' buffer by the driver ISR
  // and when there is requested ammount of data, task is unblocked
  //
  // Note: i2s_read does not care it is writing in float[] buffer, it will write
  //       integer values to the given address, as received from the hardware peripheral.
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(SAMPLE_T), &bytes_read, portMAX_DELAY);

  // Convert (including shifting) integer microphone values to floats,
  // using the same buffer (assumed sample size is same as size of float),
  // to save a bit of memory
  SAMPLE_T* int_samples = (SAMPLE_T*)&samples;

  for (int i = 0; i < SAMPLES_SHORT; i++) samples[i] = MIC_CONVERT(int_samples[i]);

  // Apply equalization and calculate Z-weighted sum of squares,
  // writes filtered samples back to the same buffer.
  q.sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

  // Apply weighting and calucate weigthed sum of squares
  q.sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);

  // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
  double short_RMS = sqrt(double(q.sum_sqr_SPL) / SAMPLES_SHORT);
  double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);

  // In case of acoustic overload or below noise floor measurement, report infinty Leq value
  if (short_SPL_dB > MIC_OVERLOAD_DB) {
    Leq_sum_sqr = MIC_OVERLOAD_DB;
  } else if (isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB)) {
    Leq_sum_sqr = MIC_NOISE_DB;
  }

  // Accumulate Leq sum
  Leq_sum_sqr += q.sum_sqr_weighted;
  Leq_samples += SAMPLES_SHORT;

  // When we gather enough samples, calculate new Leq value
  if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD) {
    double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
    Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);
    Leq_sum_sqr = 0;
    Leq_samples = 0;

    // Serial.printf("Calculated dB: %.1fdB\n", Leq_dB);
    printReadingToConsole(Leq_dB);

    if (Leq_dB < minReading) minReading = Leq_dB;
    if (Leq_dB > maxReading) maxReading = Leq_dB;
    sumReadings += Leq_dB;
    numberOfReadings++;
  }
}
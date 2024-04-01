/**
 * Civic Tech TO - Noisemeter Device (ESP32 Version) v1
 * Written by Clyne Sullivan and Nick Barnard.
 * Open source dB meter code taken from Ivan Kostoski (https://github.com/ikostoski/esp32-i2s-slm)
 * 
 * TODO:
 *  - Encrypt the stored credentials (simple XOR with a long key?).
 *  - Add second step to Access Point flow - to gather users email, generate a UUID and upload them to the cloud. UUID to be saved in EEPROM
 *  - Add functionality to reset the device periodically (eg every 24 hours)?
 */
#include <ArduinoJson.h> // https://arduinojson.org/
#include <ArduinoJson.hpp>
#include <HTTPClient.h>
#include <Ticker.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <dummy.h> // ESP32 core
#include <driver/i2s.h> // ESP32 core
#include <esp_efuse.h>
#include <esp_efuse_table.h>

#include "access-point.h"
#include "board.h"
#include "data-packet.h"
#include "sos-iir-filter.h"
#include "certs.h"
#include "secret.h"
#include "storage.h"
#include "ota-update.h"
#include "UUID/UUID.h"

#include <cmath>
#include <cstdint>
#include <list>
#include <iterator>

#if defined(BUILD_PLATFORMIO) && defined(BOARD_ESP32_PCB)
HWCDC USBSerial;
#endif

static Storage Creds;

// Uncomment these to disable WiFi and/or data upload
//#define UPLOAD_DISABLED

constexpr auto WIFI_CONNECT_TIMEOUT_MS = 20 * 1000;

const unsigned long UPLOAD_INTERVAL_SEC = 60 * 5;  // Upload every 5 mins
// const unsigned long UPLOAD_INTERVAL_SEC = 30;  // Upload every 30 secs
const unsigned long OTA_INTERVAL_SEC = 60 * 60 * 24; // Check for updates daily

// Remember up to two weeks of measurement data.
constexpr unsigned MAX_SAVED_PACKETS = 14 * 24 * 60 * 60 / UPLOAD_INTERVAL_SEC;

//
// Constants & Config
//
#define LEQ_PERIOD 1           // second(s)
#define WEIGHTING A_weighting  // Also avaliable: 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS "LAeq"       // customize based on above weighting used
#define DB_UNITS "dBA"         // customize based on above weighting used
#define USE_DISPLAY 0

// NOTE: Some microphones require at least DC-Blocker filter
#define MIC_EQUALIZER SPH0645LM4H_B_RB  // See below for defined IIR filters or set to 'None' to disable
#define MIC_OFFSET_DB 0                 // Default offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration

// Customize these values from microphone datasheet
#define MIC_SENSITIVITY -26    // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
#define MIC_REF_DB 94.0        // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_OVERLOAD_DB 120.0  // dB - Acoustic overload point
#define MIC_NOISE_DB 29        // dB - Noise floor
#define MIC_BITS 24            // valid number of bits in I2S data
#define MIC_CONVERT(s) (s >> (SAMPLE_BITS - MIC_BITS))

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
static_assert(sizeof(float) == sizeof(int32_t));
using SampleBuffer alignas(4) = float[SAMPLES_SHORT];
static SampleBuffer samples;

// Sampling Buffers & accumulators
sum_queue_t q;
uint32_t Leq_samples = 0;
double Leq_sum_sqr = 0;
double Leq_dB = 0;

// Noise Level Readings
static std::list<DataPacket> packets;
static Timestamp lastUpload = Timestamp::invalidTimestamp();
static Timestamp lastOTACheck = Timestamp::invalidTimestamp();

static UUID buildDeviceId();
static int tryWifiConnection();

/**
 * Initialization routine.
 */
void setup() {
  pinMode(PIN_LED1, OUTPUT);
  digitalWrite(PIN_LED1, LOW);

  // Grounding this pin (e.g. with a button) will force access open to start.
  // Useful as a "reset" button to overwrite currently saved credentials.
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // If needed, now you can actually lower the CPU frquency,
  // i.e. if you want to (slightly) reduce ESP32 power consumption
  // setCpuFrequencyMhz(80);  // It should run as low as 80MHz

  SERIAL.begin(115200);
  delay(2000);
  SERIAL.println();
  SERIAL.print("Noisemeter ");
  SERIAL.print(NOISEMETER_VERSION);
  SERIAL.print(' ');
  SERIAL.println(buildDeviceId());
  SERIAL.println("Initializing...");

  Creds.begin();

  initMicrophone();
  packets.emplace_front();

#ifndef UPLOAD_DISABLED
  bool isAPNeeded = false;

  if (!digitalRead(PIN_BUTTON) || !Creds.valid()) {
    SERIAL.print("Erasing stored credentials...");
    Creds.clear();
    SERIAL.println(" done.");

    isAPNeeded = true;
  } else if (tryWifiConnection() < 0 || Timestamp::synchronize() < 0) {
    isAPNeeded = true;
  }

  // Run the access point if it is requested or if there are no valid credentials.
  if (isAPNeeded) {
    AccessPoint ap (saveNetworkCreds);
    Ticker blink;

    blink.attach_ms(500, [] {
      static bool state = HIGH;
      digitalWrite(PIN_LED1, state ^= HIGH);
    });
    ap.run(); // does not return
  }

  Timestamp now;
  lastUpload = now;
  lastOTACheck = now;

  SERIAL.println("Connected to the WiFi network.");
  SERIAL.print("Local ESP32 IP: ");
  SERIAL.println(WiFi.localIP());
  SERIAL.print("Current time: ");
  SERIAL.println(now);
#endif // !UPLOAD_DISABLED

  digitalWrite(PIN_LED1, HIGH);
}

void loop() {
  readMicrophoneData();

#ifndef UPLOAD_DISABLED
  const auto now = Timestamp();

  if (lastUpload.secondsBetween(now) >= UPLOAD_INTERVAL_SEC) {
    packets.front().timestamp = now;

    if (WiFi.status() != WL_CONNECTED) {
      SERIAL.println("Attempting WiFi reconnect...");
      WiFi.reconnect();
      delay(5000);
    }

    if (WiFi.status() == WL_CONNECTED) {
      packets.remove_if([](const auto& pkt) {
        if (pkt.count > 0) {
          const auto payload = createJSONPayload(pkt);
          WiFiClientSecure client;
          return uploadData(&client, payload) == 0;
        } else {
          return true; // Discard empty packets
        }
      });

#if defined(BOARD_ESP32_PCB)
      // We have WiFi: also check for software updates
      if (lastOTACheck.secondsBetween(now) >= OTA_INTERVAL_SEC) {
        lastOTACheck = now;
        SERIAL.println("Checking for updates...");

        OTAUpdate ota (cert_ISRG_Root_X1);
        if (ota.available()) {
          SERIAL.print(ota.version);
          SERIAL.println(" available!");
          digitalWrite(PIN_LED1, LOW);

          if (ota.download()) {
            SERIAL.println("Download success! Restarting...");
            digitalWrite(PIN_LED1, HIGH);
            delay(1000);
            ESP.restart();
          } else {
            SERIAL.println("Update download failed.");
            digitalWrite(PIN_LED1, HIGH);
          }
        } else {
          SERIAL.println("No update available.");
        }
      }
#endif // BOARD_ESP32_PCB
    }

    if (!packets.empty()) {
      const auto count = std::distance(packets.cbegin(), packets.cend());
      SERIAL.print(count);
      SERIAL.println(" packets still need to be sent!");

      if (count >= MAX_SAVED_PACKETS) {
        SERIAL.println("Discarded a packet!");
        packets.pop_back();
      }
    }

    // Create new packet for next measurements
    packets.emplace_front();
    lastUpload = now;
  }
#endif // !UPLOAD_DISABLED
}

void printArray(float arr[], unsigned long length) {
  SERIAL.print(length);
  SERIAL.print(" {");
  for (int i = 0; i < length; i++) {
    SERIAL.print(arr[i]);
    if (i < length - 1) SERIAL.print(", ");
  }
  SERIAL.println("}");
}

void printReadingToConsole(double reading) {
  String output = "";
  output += std::lround(reading);
  output += "dB";

  const auto currentCount = packets.front().count;
  if (currentCount > 1) {
    output += " [+" + String(currentCount - 1) + " more]";
  }
  SERIAL.println(output);
}

void saveNetworkCreds(WebServer& httpServer) {
  // Confirm that the form was actually submitted.
  if (httpServer.hasArg("ssid") && httpServer.hasArg("psk")) {
    const auto ssid = httpServer.arg("ssid");
    const auto psk = httpServer.arg("psk");

    // Confirm that the given credentials will fit in the allocated EEPROM space.
    if (!ssid.isEmpty() && Creds.canStore(ssid) && Creds.canStore(psk)) {
      Creds.set(Storage::Entry::SSID, ssid);
      Creds.set(Storage::Entry::Passkey, psk);
      Creds.commit();

      SERIAL.print("Saving ");
      SERIAL.println(Creds);

      SERIAL.println("Saved network credentials. Restarting...");
      delay(2000);
      ESP.restart();  // Software reset.
    }
  }

  // TODO inform user that something went wrong...
  SERIAL.println("Error: Invalid network credentials!");
}

UUID buildDeviceId()
{
  std::array<uint8_t, 6> mac;
  esp_efuse_read_field_blob(ESP_EFUSE_MAC_FACTORY, mac.data(), /* bits */ mac.size() * 8);
  return UUID(mac[0] | (mac[1] << 8) | (mac[2] << 16), mac[3] | (mac[4] << 8) | (mac[5] << 16));
}

int tryWifiConnection()
{
  const auto ssid = Creds.get(Storage::Entry::SSID);

  if (ssid.isEmpty())
    return -1;

  SERIAL.print("Ready to connect to ");
  SERIAL.println(ssid);

  WiFi.mode(WIFI_STA);
  if (WiFi.begin(ssid.c_str(), Creds.get(Storage::Entry::Passkey).c_str()) == WL_CONNECT_FAILED)
    return -1;

  // wait for WiFi connection
  SERIAL.print("Waiting for WiFi to connect...");
  const auto start = millis();
  bool connected;

  do {
    connected = WiFi.status() != WL_CONNECTED;
    SERIAL.print(".");
    delay(500);
  } while (!connected && millis() - start < WIFI_CONNECT_TIMEOUT_MS);

  return connected ? 0 : -1;
}

String createJSONPayload(const DataPacket& dp)
{
#ifdef BUILD_PLATFORMIO
  JsonDocument doc;
#else
  DynamicJsonDocument doc (2048);
#endif

  doc["parent"] = "/Bases/nm1";
  doc["data"]["type"] = "comand";
  doc["data"]["version"] = "1.0";
  doc["data"]["contents"][0]["Type"] = "Noise";
  doc["data"]["contents"][0]["Min"] = std::lround(dp.minimum);
  doc["data"]["contents"][0]["Max"] = std::lround(dp.maximum);
  doc["data"]["contents"][0]["Mean"] = std::lround(dp.average);
  doc["data"]["contents"][0]["DeviceID"] = String(buildDeviceId());
  doc["data"]["contents"][0]["Timestamp"] = String(dp.timestamp);

  // Serialize JSON document
  String json;
  serializeJson(doc, json);
  return json;
}

// Given a serialized JSON payload, upload the data to webcomand
int uploadData(WiFiClientSecure* client, String json)
{
  if (client) {
    client->setCACert(cert_ISRG_Root_X1);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;
      // void addHeader(const String& name, const String& value, bool first = false, bool replace = true);

      SERIAL.print("[HTTPS] begin...\n");
      if (https.begin(*client, "https://noisemeter.webcomand.com/ws/put")) {  // HTTPS
        SERIAL.print("[HTTPS] POST...\n");
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
          SERIAL.printf("[HTTPS] POST... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            SERIAL.println(payload);
          } else {
            SERIAL.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
            return -1;
          }
        } else {
          SERIAL.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
          return -1;
        }

        https.end();
      } else {
        SERIAL.printf("[HTTPS] Unable to connect\n");
        return -1;
      }

      // End extra scoping block
    }

    // delete client;
  } else {
    SERIAL.println("Unable to create client");
    return -1;
  }

  return 0;
}

//
// I2S Microphone sampling setup
//
void initMicrophone() {
  // Setup I2S to sample mono channel for SAMPLE_RATE * SAMPLE_BITS
  // NOTE: Recent update to Arduino_esp32 (1.0.2 -> 1.0.3)
  //       seems to have swapped ONLY_LEFT and ONLY_RIGHT channels
  const i2s_config_t i2s_config = {
    mode: i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate: SAMPLE_RATE,
    bits_per_sample: i2s_bits_per_sample_t(SAMPLE_BITS),
    channel_format: I2S_FORMAT,
    communication_format: i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
    dma_buf_count: DMA_BANKS,
    dma_buf_len: DMA_BANK_SIZE,
    use_apll: true,
    tx_desc_auto_clear: false,
    fixed_mclk: 0,
    mclk_multiple: I2S_MCLK_MULTIPLE_DEFAULT,
    bits_per_chan: I2S_BITS_PER_CHAN_DEFAULT,
  };

  // I2S pin mapping
  const i2s_pin_config_t pin_config = {
    mck_io_num: -1, // not used
    bck_io_num: I2S_SCK,
    ws_io_num: I2S_WS,
    data_out_num: -1,  // not used
    data_in_num: I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);

  // Discard first block, microphone may need time to startup and settle.
  size_t bytes_read;
  i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
}

void readMicrophoneData() {
  // Block and wait for microphone values from I2S
  //
  // Data is moved from DMA buffers to our 'samples' buffer by the driver ISR
  // and when there is requested ammount of data, task is unblocked
  //
  // Note: i2s_read does not care it is writing in float[] buffer, it will write
  //       integer values to the given address, as received from the hardware peripheral.
  size_t bytes_read;
  i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

  // Convert (including shifting) integer microphone values to floats,
  // using the same buffer (assumed sample size is same as size of float),
  // to save a bit of memory
  auto int_samples = reinterpret_cast<SAMPLE_T*>(samples);

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

    printReadingToConsole(Leq_dB);
    packets.front().add(Leq_dB);
  }
}

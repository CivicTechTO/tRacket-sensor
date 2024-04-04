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
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_efuse.h>
#include <esp_efuse_table.h>

#include "access-point.h"
#include "blinker.h"
#include "board.h"
#include "data-packet.h"
#include "certs.h"
#include "secret.h"
#include "spl-meter.h"
#include "storage.h"
#include "ota-update.h"
#include "UUID/UUID.h"

#include <cmath>
#include <cstdint>
#include <list>
#include <iterator>
#include <optional>

#if defined(BUILD_PLATFORMIO) && defined(BOARD_ESP32_PCB)
HWCDC USBSerial;
#endif

static SPLMeter SPL;
static Storage Creds;

// Uncomment these to disable WiFi and/or data upload
//#define UPLOAD_DISABLED

constexpr auto WIFI_CONNECT_TIMEOUT_MS = 20 * 1000;

const unsigned long UPLOAD_INTERVAL_SEC = 60 * 5;  // Upload every 5 mins
// const unsigned long UPLOAD_INTERVAL_SEC = 30;  // Upload every 30 secs
const unsigned long OTA_INTERVAL_SEC = 60 * 60 * 24; // Check for updates daily

// Remember up to two weeks of measurement data.
constexpr unsigned MAX_SAVED_PACKETS = 14 * 24 * 60 * 60 / UPLOAD_INTERVAL_SEC;

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

  SPL.initMicrophone();
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
    Blinker bl (500);

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
  if (auto db = SPL.readMicrophoneData(); db) {
    packets.front().add(*db);
    printReadingToConsole(*db);
  }

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
      {
        std::optional<Blinker> bl;

        // Only blink if there's multiple packets to send
        if (++packets.cbegin() != packets.cend())
          bl.emplace(300);

        packets.remove_if([](const auto& pkt) {
          if (pkt.count > 0) {
            const auto payload = createJSONPayload(pkt);
            WiFiClientSecure client;
            return uploadData(&client, payload) == 0;
          } else {
            return true; // Discard empty packets
          }
        });
      }

#if defined(BOARD_ESP32_PCB)
      // We have WiFi: also check for software updates
      if (lastOTACheck.secondsBetween(now) >= OTA_INTERVAL_SEC) {
        lastOTACheck = now;
        SERIAL.println("Checking for updates...");

        OTAUpdate ota (cert_ISRG_Root_X1);
        if (ota.available()) {
          SERIAL.print(ota.version);
          SERIAL.println(" available!");

          if (ota.download()) {
            SERIAL.println("Download success! Restarting...");
            delay(1000);
            ESP.restart();
          } else {
            SERIAL.println("Update download failed.");
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
    connected = WiFi.status() == WL_CONNECTED;
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


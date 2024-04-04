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
 */
#include <ArduinoJson.h> // https://arduinojson.org/
#include <ArduinoJson.hpp>
#include <HTTPClient.h>
#include <UUID.h> // https://github.com/RobTillaart/UUID
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>

#include "access-point.h"
#include "board.h"
#include "certs.h"
#include "secret.h"
#include "spl-meter.h"
#include "storage.h"

#include <cstdint>
#include <ctime>

#if defined(BUILD_PLATFORMIO) && defined(BOARD_ESP32_PCB)
HWCDC USBSerial;
#endif

static SPLMeter SPL;
static Storage Creds;
static WiFiMulti WiFiMulti;

// Uncomment these to disable WiFi and/or data upload
//#define UPLOAD_DISABLED

const unsigned long UPLOAD_INTERVAL_MS = 60000 * 5;  // Upload every 5 mins
// const unsigned long UPLOAD_INTERVAL_MS = 30000;  // Upload every 30 secs
const unsigned long MIN_READINGS_BEFORE_UPLOAD = 20;

// Not sure if WiFiClientSecure checks the validity date of the certificate.
// Setting clock just to be sure...
void setClock() {
  configTime(0, 0, "pool.ntp.org");

  SERIAL.print("Waiting for NTP time sync: ");
  std::time_t nowSecs = std::time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    SERIAL.print(".");
    yield();
    nowSecs = time(nullptr);
  }
  SERIAL.println();

  char timebuf[32];

  const auto timeinfo = std::gmtime(&nowSecs);
  SERIAL.print("Current time: ");
  if (std::strftime(timebuf, sizeof(timebuf), "%c", timeinfo) > 0)
    SERIAL.println(timebuf);
  else
    SERIAL.println("(error)");
}

unsigned long lastUploadMillis = 0;

/**
 * Initialization routine.
 */
void setup() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);

  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, HIGH);

  // Grounding this pin (e.g. with a button) will force access open to start.
  // Useful as a "reset" button to overwrite currently saved credentials.
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // If needed, now you can actually lower the CPU frquency,
  // i.e. if you want to (slightly) reduce ESP32 power consumption
  // setCpuFrequencyMhz(80);  // It should run as low as 80MHz

  SERIAL.begin(115200);
  SERIAL.println();
  SERIAL.println("Initializing...");

  Creds.begin();
  SERIAL.print("Stored credentials: ");
  SERIAL.println(Creds);

  SPL.initMicrophone();

#ifndef UPLOAD_DISABLED
  // Run the access point if it is requested or if there are no valid credentials.
  bool resetPressed = !digitalRead(PIN_BUTTON);
  if (resetPressed || !Creds.valid()) {
    AccessPoint ap;

    SERIAL.println("Erasing stored credentials...");
    Creds.clear();
    SERIAL.print("Stored Credentials after erasing: ");
    SERIAL.println(Creds);

    ap.onCredentialsReceived(saveNetworkCreds);
    ap.begin();
    ap.run(); // does not return
  }

  // Valid credentials: Next step would be to connect to the network.
  const auto ssid = Creds.get(Storage::Entry::SSID);

  SERIAL.print("Ready to connect to ");
  SERIAL.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid.c_str(), Creds.get(Storage::Entry::Passkey).c_str());

  // wait for WiFi connection
  SERIAL.print("Waiting for WiFi to connect...");
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    SERIAL.print(".");
    delay(500);
  }
  SERIAL.println("\nConnected to the WiFi network");
  SERIAL.print("Local ESP32 IP: ");
  SERIAL.println(WiFi.localIP());

  setClock();
#endif // !UPLOAD_DISABLED

  digitalWrite(PIN_LED1, HIGH);
}

void loop() {
  SPL.readMicrophoneData();

#ifndef UPLOAD_DISABLED
  if (canUploadData()) {
    WiFiClientSecure* client = new WiFiClientSecure;
    float average = SPL.sumReadings / SPL.numberOfReadings;
    String payload = createJSONPayload("", SPL.minReading, SPL.maxReading, average);
    uploadData(client, payload);
    delete client;
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
  output += reading;
  output += "dB";
  if (SPL.numberOfReadings > 1) {
    output += " [+" + String(SPL.numberOfReadings - 1) + " more]";
  }
  SERIAL.println(output);
}

void saveNetworkCreds(WebServer& httpServer) {
  // Confirm that the form was actually submitted.
  if (httpServer.hasArg("ssid") && httpServer.hasArg("psk")) {
    const auto ssid = httpServer.arg("ssid");
    const auto psk = httpServer.arg("psk");
    UUID uuid; // generates random UUID

    // Confirm that the given credentials will fit in the allocated EEPROM space.
    if (Creds.canStore(ssid) && Creds.canStore(psk)) {
      Creds.set(Storage::Entry::SSID, ssid);
      Creds.set(Storage::Entry::Passkey, psk);
      Creds.set(Storage::Entry::UUID, uuid.toCharArray());
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

String createJSONPayload(String deviceId, float min, float max, float average) {
#ifdef BUILD_PLATFORMIO
  JsonDocument doc;
#else
  DynamicJsonDocument doc (2048);
#endif
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
  // Has it been at least the upload interval since we uploaded data?
  long now = millis();
  long msSinceLastUpload = now - lastUploadMillis;
  if (msSinceLastUpload < UPLOAD_INTERVAL_MS) return false;

  // Do we have the minimum number of readings stored to form a resonable average?
  if (SPL.numberOfReadings < MIN_READINGS_BEFORE_UPLOAD) return false;
  return true;
}

void uploadData(WiFiClientSecure* client, String json) {
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
          }
        } else {
          SERIAL.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        SERIAL.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }

    // delete client;
  } else {
    SERIAL.println("Unable to create client");
  }

  long now = millis();
  lastUploadMillis = now;
  resetReading();
};  // Given a serialized JSON payload, upload the data to webcomand

void resetReading() {
  SERIAL.println("Resetting readings cache...");
  SPL.numberOfReadings = 0;
  SPL.minReading = MIC_OVERLOAD_DB;
  SPL.maxReading = MIC_NOISE_DB;
  SPL.sumReadings = 0;
  SERIAL.println("Reset complete");
};


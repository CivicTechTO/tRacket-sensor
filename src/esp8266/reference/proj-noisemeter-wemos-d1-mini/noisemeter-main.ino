#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include "math.h"
#include "certs.h"
#include "secret.h"

/*
  ==============================================================================
  CONSTANTS & VARIABLES
  ==============================================================================
*/
X509List cert(cert_ISRG_Root_X1);
// Use WiFiClientSecure class to create TLS connection
WiFiClientSecure client;

const bool IS_CALIBRATION_MODE = true;

const String REQUEST_PATH = "/ws/put";
const int MAX_LEVEL_FLOAT = 1024;
const float REFERENCE_LEVEL_PA = 0.00002; // lowest detectable level in Pascal
const float MAX_LEVEL_PASCAL = 0.6325; // Max detectable level in pascal
const int CALIBRATION_DB_SPL = 0; // -3
const String DEVICE_ID = "nick4";
const unsigned long uploadIntervalMS = 60000 * 5; // Upload every 5 mins

const int SAMPLE_CACHE_LENGTH = 100;
int numberOfSamples = 0;
int sampleCache[SAMPLE_CACHE_LENGTH]; // Store raw input data for smoothing

int numberOfReadings = 0;
float minReading = 0;
float maxReading = 0;
unsigned long lastUploadMillis = 0;


/*
  ==============================================================================
  SETUP
  ==============================================================================
*/
void setup() {
  pinMode(A0, INPUT); // Set analog pin to input

  for (int i = 0; i < SAMPLE_CACHE_LENGTH; i++) {
    sampleCache[i] = 0;
  }; // Set all sample points to 0

  connectToWifi();
  configTime();

  Serial.print("Connecting to ");
  Serial.println(REQUEST_HOSTNAME);

  Serial.printf("Using certificate: %s\n", cert_ISRG_Root_X1);
  client.setTrustAnchors(&cert);
}

/*
  ==============================================================================
  LOOP
  ==============================================================================
*/
void loop() {
  int micReading = analogRead(A0);
//  if (IS_CALIBRATION_MODE) {
//    Serial.println(micReading);
//  }

  if (numberOfSamples == 0) {
    // If no samples have been taken, init sample cache
    initSampleCache(micReading);
  } else {
    updateSamples(micReading);
  } // Always update samples

  if (numberOfSamples >= SAMPLE_CACHE_LENGTH) {
    // When the sample cache is full, take a reading and store it
    takeReading();

    if (!IS_CALIBRATION_MODE) {
      // If enough time has elapsed since last upload, attempt upload
      long now = millis();
      long msSinceLastUpload = now - lastUploadMillis;
      if (msSinceLastUpload >= uploadIntervalMS) {
        if (!client.connect(REQUEST_HOSTNAME, REQUEST_PORT)) {
          Serial.println("Wifi Client Connection failed");
          return;
        }
        String payload = createJSONPayload();
        uploadData(client, payload);
      };
    };
  };

//  delay(100);
};

/*
  ==============================================================================
  FUNCTIONS
  ==============================================================================
*/
void connectToWifi() {
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(NETWORK_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}; // Connect to WiFi network using NETWORK_SSID and NETWORK_PASSWORD

void configTime() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}; // Set time via NTP, as required for x.509 validation

String createJSONPayload() {
  // Prepare JSON document
  DynamicJsonDocument doc(2048);
  doc["parent"] = "/Bases/nm1";
  doc["data"]["type"] = "comand";
  doc["data"]["version"] = "1.0";
  doc["data"]["contents"][0]["Type"] = "Noise";
  doc["data"]["contents"][0]["Min"] = minReading;
  doc["data"]["contents"][0]["Max"] = maxReading;
  doc["data"]["contents"][0]["DeviceID"] = "nick2";

  // Serialize JSON document
  String json;
  serializeJson(doc, json);
  return json;
}; // Assemble JSON payload from global variables

void uploadData(WiFiClientSecure client, String json) {

  Serial.print("Requesting URL: ");
  Serial.println(REQUEST_PATH);

  String request = String("POST ") + REQUEST_PATH + " HTTP/1.1\r\n" +
                   "Host: " + REQUEST_HOSTNAME + "\r\n" +
                   "User-Agent: ESP8266\r\n" +
                   "Connection: close\r\n" +
                   "Authorization: Token " + API_TOKEN + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + json.length() + "\r\n\r\n" +
                   json + "\r\n";

  Serial.println(request);
  client.print(request);

  Serial.println("Request sent");
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers received");
      break;
    }
  }
  String line = client.readString();
  if (line.startsWith("HTTP/1.1 200")) {
    Serial.println("esp8266/Arduino CI successful!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.println("Reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("Closing connection");

  long now = millis();
  lastUploadMillis = now;
  resetReading();
}; // Given a serialized JSON payload, upload the data to webcomand

void displayReading(float micReading, float db) {
  int noOfDots = floor(micReading / 10);
  String vuMeter;
  vuMeter = vuMeter + micReading + " ";
  vuMeter = vuMeter + floor(db);
  vuMeter = vuMeter + "dB: ";
  for (int i = 0; i < noOfDots; i++) {
    vuMeter = vuMeter + "#";
  };
  Serial.println(vuMeter);
};

void takeReading() {
  int micReading = getAverageReading();
  float db = getDbSplFromAudioMeasurement(micReading);

  if (numberOfReadings == 0) {
    minReading = db;
    maxReading = db;
  };
  if (db > maxReading) {
    maxReading = db;
  }
  if (db < minReading) {
    minReading = db;
  }
  numberOfReadings ++;
    if (IS_CALIBRATION_MODE) {
     displayReading(micReading, db);
    };
};

float getDbSplFromAudioMeasurement(int measurement) {
  if (measurement <= 0) {
    // No valid level detected
    return 0;
  }
  float denominator = MAX_LEVEL_FLOAT / MAX_LEVEL_PASCAL;
  float pressure = measurement / denominator;
  float dbSpl = 20 * log10 (pressure / REFERENCE_LEVEL_PA);
  return dbSpl + CALIBRATION_DB_SPL;
};

void resetReading() {
  Serial.println("Resetting...");
  Serial.println("Min: " + String(minReading));
  Serial.println("Max: " + String(maxReading));
  Serial.println("numberOfReadings: " + String(numberOfReadings));
  numberOfReadings = 0;
  minReading = 0;
  maxReading = 0;
  Serial.println("Reset complete");
  Serial.println("Min: " + String(minReading));
  Serial.println("Max: " + String(maxReading));
  Serial.println("numberOfReadings: " + String(numberOfReadings));
};

// Initialize readings cache - set all values to current reading
void initSampleCache (int reading) {
  numberOfSamples++;
  for (int i = 0; i < SAMPLE_CACHE_LENGTH; i++) {
    sampleCache[i] = reading;
  }
}

// Add latest reading to cache, move others along
void updateSamples (int reading) {
  if (numberOfSamples < SAMPLE_CACHE_LENGTH) {
    numberOfSamples++;
  }
  int newArray[SAMPLE_CACHE_LENGTH];
  newArray[0] = reading;
  for (int i = 1; i < SAMPLE_CACHE_LENGTH; i++) {
    newArray[i] = sampleCache[i - 1];
  }
  for (int i = 0; i < SAMPLE_CACHE_LENGTH; i++) {
    sampleCache[i] = newArray[i];
  }
  //   Serial.print(sampleCache[0]);
  //  Serial.print(", ");
  //  Serial.print(sampleCache[1]);
  //  Serial.print(", ");
  //  Serial.print(sampleCache[2]);
  //  Serial.print(", ");
  //  Serial.print(sampleCache[3]);
  //  Serial.print(", ");
  //  Serial.println(sampleCache[4]);
}

int getAverageReading() {
  int sum = 0;
  for (int i = 0; i < SAMPLE_CACHE_LENGTH; i++) {
    sum = sum + sampleCache[i];
  };
  int averageReading = sum / SAMPLE_CACHE_LENGTH;
  return averageReading;
}

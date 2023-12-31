#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <ESP8266WiFi.h>
#include "secret.h"
#include "math.h"

WiFiClientSecure wifiClient;
////    { "parent": "/Bases/nm1",
////                   "data": {
////         "type": "comand",
////        "version": "1.0",
////        "contents":
////         [
////            {
////         "Type": "Noise",
////         "Min": int.from_bytes(dbmin, "big"),
////        "Max": int.from_bytes(dbmax, "big"),
////        "DeviceID": "gabe1"
////
////            }
////        ]
////    }}

struct MinMax {
  int Min;
  int Max;
};

struct NoiseData {
  String Type;
  int Min;
  int Max;
  String DeviceID;
};

struct UploadData {
  String type;
  String version;
  NoiseData contents[];
};

struct UploadPacket {
  String parent;
  UploadData data;
};

unsigned short wifiAttempts = 0;

const int MAX_LEVEL_FLOAT = 1024;
const float REFERENCE_LEVEL_PA = 0.00002; // lowest detectable level in Pascal
const float MAX_LEVEL_PASCAL = 0.6325; // Max detectable level in pascal
const int CALIBRATION_DB_SPL = -3;

//const String UPLOAD_URI = "http://192.168.2.15:3000/upload?count=" + count;
const char* HOST = "https://noisemeter.webcomand.com";
const String PATH = "/ws/put";
const uint16_t PORT = 443;

void setup() {

  Serial.begin(9600);
  delay(10);

  pinMode(A0, INPUT);

  connectToWifi();
}

void loop() {
  //  takeReading();

  uploadData();

  delay(5000);  // Send a request every 5 seconds
}

void connectToWifi() {
  // Connect WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("Name");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;

    if (wifiAttempts >= 30)
      wifiRestart();
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
}

void wifiRestart() {
  Serial.println("Turning WiFi off...");
  WiFi.mode(WIFI_OFF);
  Serial.println("Sleeping for 10 seconds...");
  delay(10000);
  Serial.println("Trying to connect to WiFi...");
  WiFi.mode(WIFI_STA);
}

//void uploadData2() {
//
//};

void uploadData() {
  if (WiFi.status() == WL_CONNECTED) {  //Check WiFi connection status

    //    Serial.println("Connecting to ");
    //    Serial.println(UPLOAD_URI);

    //
    //    if (wifiClient.verify(fingerprint, UPLOAD_URI)) {
    //      Serial.println("certificate matches");
    //    } else {
    //      Serial.println("certificate doesn't match");
    //    }
    //// TODO - 'class BearSSL::WiFiClientSecure' has no member named 'verify'

    //    HTTPClient http;  //Declare object of class HTTPClient
    //
    //    http.begin(wifiClient, UPLOAD_URI);  //Specify request destination
    //    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    //    http.addHeader("Authorization", token); // Add auth token

    // Prepare JSON document
    DynamicJsonDocument doc(2048);
    doc["parent"] = "/Bases/nm1";
    doc["data"]["type"] = "comand";
    doc["data"]["version"] = "1.0";
    doc["data"]["contents"][0]["Type"] = "Noise";
    doc["data"]["contents"][0]["Min"] = 0;
    doc["data"]["contents"][0]["Max"] = 70;
    doc["data"]["contents"][0]["DeviceID"] = "nick2";

    // Serialize JSON document
    String json;
    serializeJson(doc, json);

    Serial.print("connecting to : '");
    Serial.print(HOST);
    Serial.println("'");
    Serial.printf("Using fingerprint '%s'\n", fingerprint);

    wifiClient.setFingerprint(fingerprint);

    //// TODO - connection always fails

    Serial.print("requesting URL: '");
    Serial.print(PATH);
    Serial.println("'");

    //    if (wifiClient.verify(fingerprint, HOST)) {
    //      Serial.println("Request verified");
    //    } else {
    //      Serial.println("Request not verified");
    //      };

    wifiClient.print(String("POST ") + PATH + " HTTP/1.1\r\n" +
                     "Host: " + HOST + "\r\n" +
                     "Connection: close\r\n" +
                     "Content-Type: application/json" +
                     "Authorization: " + token + "\r\n" +
                     "Content-Length: " + json.length() + "\r\n" +
                     "\r\n" +
                     json + "\r\n");

    Serial.println("request sent");

    while (wifiClient.connected()) {
      String line = wifiClient.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    String line = wifiClient.readStringUntil('\n');
    if (line.startsWith("{\"state\":\"success\"")) {
      Serial.println("esp8266/Arduino CI successfull!");
    } else {
      Serial.println("esp8266/Arduino CI has failed");
    }
    Serial.println("reply was:");
    Serial.println("==========");
    Serial.println(line);
    Serial.println("==========");
    Serial.println("closing connection");
//    int code = wifiClient.lastError();
//    Serial.println("Error: ");
//    Serial.println(code);


    //    unsigned long timeout = millis();
    //    while (wifiClient.available() == 0) {
    //      if (millis() - timeout > 30000) {
    //        Serial.println(">>> Client Timeout !");
    //        wifiClient.stop();
    //        return;
    //      }
    //      yield();
    //    }

    //    int httpCode = http.POST(json);
    //    String payload = http.getString();

    // DynamicJsonDocument response(2048);
    // deserializeJson(response, http.getString());  //Get the response payload

    //    if (httpCode > 0) {
    //      // HTTP header has been send and Server response header has been handled
    //      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
    //      const String& payload = http.getString();
    //      Serial.println("received payload:\n<<");
    //      Serial.println(payload);
    //      Serial.println(">>");
    //
    //      // file found at server
    //      if (httpCode == HTTP_CODE_OK) {
    //        const String& payload = http.getString();
    //        Serial.println("received payload:\n<<");
    //        Serial.println(payload);
    //        Serial.println(">>");
    //      }
    //    } else {
    //      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    //    }
    //
    //    http.end();  //Close connection
    //
  } else {

    Serial.println("Error in WiFi connection");
  }
};

void takeReading() {
  int micReading = analogRead(A0);
  float db = getDbSplFromAudioMeasurement(micReading);
  int noOfDots = floor(micReading / 10);
  String vuMeter;
  vuMeter = vuMeter + floor(db);
  vuMeter = vuMeter + "dB: ";
  for (int i = 0; i < noOfDots; i++) {
    vuMeter = vuMeter + "#";
  };
  Serial.println(vuMeter);
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
}

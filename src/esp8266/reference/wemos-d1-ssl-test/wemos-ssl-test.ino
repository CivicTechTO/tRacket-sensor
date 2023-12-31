/*
    HTTP over TLS (HTTPS) example sketch

    This example demonstrates how to use
    WiFiClientSecure class to access HTTPS API.
    We fetch and display the status of
    esp8266/Arduino project continuous integration
    build.

    Created by Ivan Grokhotkov, 2015.
    This example is in public domain.
*/

/*
  python cert.py -s noisemeter.webcomand.com -n webcomand
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include "math.h"
#include "certs.h"
#include "secret.h"

X509List cert(cert_ISRG_Root_X1);

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set time via NTP, as required for x.509 validation
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

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("Connecting to ");
  Serial.println(webcomand_host);

  Serial.printf("Using certificate: %s\n", cert_ISRG_Root_X1);
  client.setTrustAnchors(&cert);

  if (!client.connect(webcomand_host, webcomand_port)) {
    Serial.println("Connection failed");
    return;
  }

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

  String url = "/ws/put";
//  String url = "/io_comand_webservice/get?query=SELECT%20Timestamp,%20DeviceID,%20Min,%20Max%20FROM%20Noise%20%20WHERE%20DeviceID%3D'nick2'%20ORDER%20BY%20Timestamp%20DESC&token=UPooYuLm4Zwu859Ehidgl5Ta19iuKTUG";
  Serial.print("Requesting URL: ");
  Serial.println(url);

//  String request = String("POST ") + url + " HTTP/1.1\r\n" + "Host: " + webcomand_host + "\r\n" + "User-Agent: ESP8266\r\n" + "Connection: close\r\n" + "Authorization: " + token + "\r\n" + "Content-Type: application/json;boundary=\"boundary\"\r\n" + "Content-Length: " + json.length() + "\r\n\r\n" + "--boundary\r\n" + "Content-Disposition: form-data; name=\"data\"\r\n\r\n" + "Content-Type: application/json\r\n\r\n" + json + "\r\n" + "--boundary--\r\n";
  String request = String("POST ") + url + " HTTP/1.1\r\n" +
  "Host: " + webcomand_host + "\r\n" +
  "User-Agent: ESP8266\r\n" +
  "Connection: close\r\n" +
  "Authorization: " + token + "\r\n" +
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
}

void loop() {}

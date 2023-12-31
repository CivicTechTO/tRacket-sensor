/**
 * WiFi Credential Manager example
 * Written by Clyne Sullivan.
 * 
 * This Arduino sketch stores WiFi SSID and passkey information in non-volatile
 * EEPROM in order to connect to a given network. If these credentials are not
 * saved, an access point will be started with a web server allowing the user
 * to input new credentials. The access point can also be started by grounding
 * the "CredsResetPin" pin (default pin 0).
 * 
 * This example is intended for ESP32 microcontrollers.
 * 
 * TODO:
 *  - Use DNS to make a "captive portal" that brings users directly to the credentials form.
 *  - Encrypt the stored credentials (simple XOR with a long key?).
 */

#include <CRC32.h> // https://github.com/bakercp/CRC32
#include <EEPROM.h>
#include <WebServer.h>
#include <WiFiAP.h>

#include <cstdint>

constexpr auto AccessPointSSID = "Noise meter";
constexpr auto AccessPointPsk  = "noisemeter";

// The ESP32's IP address within its access point will be "4.3.2.1".
// Once connected to the access point, open up 4.3.2.1 in a browser to get
// to the credentials form.
static const IPAddress AccessPointIP (4, 3, 2, 1);

// Grounding this pin (e.g. with a button) will force access open to start.
// Useful as a "reset" button to overwrite currently saved credentials.
constexpr int CredsResetPin = 0;

// EEPROM addresses for credential data.
constexpr unsigned int EEPROMMaxStringSize = 64;
constexpr int EEPROMEntryCSum = 0; // CRC32 checksum of SSID and passkey
constexpr int EEPROMEntrySSID = EEPROMEntryCSum + sizeof(uint32_t);
constexpr int EEPROMEntryPsk  = EEPROMEntrySSID + EEPROMMaxStringSize;
constexpr int EEPROMTotalSize = EEPROMEntryPsk + EEPROMMaxStringSize;

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
[[noreturn]]
static void runAccessPoint();

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

/**
 * Initialization routine.
 */
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Initializing...");

  EEPROM.begin(EEPROMTotalSize);
  delay(2000); // Ensure the EEPROM peripheral has enough time to initialize.

  // Run the access point if it is requested or if there are no valid credentials.
  if (isCredsResetPressed() || !isEEPROMCredsValid()) {
    runAccessPoint();
  }

  // Valid credentials: Next step would be to connect to the network.
  const auto ssid = EEPROM.readString(EEPROMEntrySSID);
  const auto psk = EEPROM.readString(EEPROMEntryPsk);
  Serial.print("Ready to connect to ");
  printCredentials(ssid, psk);
}

void loop() {
  delay(1000); // Nothing to do yet...
}

void runAccessPoint()
{
  static WebServer httpServer (80);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AccessPointIP, AccessPointIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AccessPointSSID, AccessPointPsk);

  // GET request means user wants to see the form.
  // POST request means user has submitted data through the form.
  httpServer.on("/", HTTP_GET,
    [] { httpServer.send_P(200, PSTR("text/html"), SetupPageHTML); });
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

void printCredentials(const String& ssid, const String& psk)
{
  Serial.print("SSID \"");
  Serial.print(ssid);
  Serial.print("\" passkey \"");
  Serial.print(psk);
  Serial.println("\"");
}

uint32_t calculateCredsChecksum(const String& ssid, const String& psk)
{
  CRC32 crc;
  crc.update(ssid.c_str(), ssid.length());
  crc.update(psk.c_str(), psk.length());
  return crc.finalize();
}

bool isEEPROMCredsValid()
{
  const auto csum = EEPROM.readUInt(EEPROMEntryCSum);
  const auto ssid = EEPROM.readString(EEPROMEntrySSID);
  const auto psk = EEPROM.readString(EEPROMEntryPsk);

  Serial.print("EEPROM stored credentials: ");
  printCredentials(ssid, psk);

  return !ssid.isEmpty() && !psk.isEmpty() && csum == calculateCredsChecksum(ssid, psk);
}

bool isCredsResetPressed()
{
  pinMode(CredsResetPin, INPUT_PULLUP);
  delay(100); // Let the IO circuit settle.
  return !digitalRead(CredsResetPin);
}

void saveNetworkCreds(WebServer& httpServer)
{
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
      ESP.restart(); // Software reset.
    }
  }

  // TODO inform user that something went wrong...
  Serial.println("Error: Invalid network credentials!");
}
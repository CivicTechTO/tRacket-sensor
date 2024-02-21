#include "access-point.h"
#include "board.h"

#include <WiFiAP.h>

#define HTML_HEADER \
    "<!DOCTYPE html>" \
    "<html lang='en'>" \
    "<head>" \
    "<meta charset='utf-8'>" \
    "<meta name='viewport' content='width=device-width,initial-scale=1'/>" \
    "</head>" \
    "<body>"

#define HTML_FOOTER \
    "</body>" \
    "</html>"

// The ESP32's IP address within its access point will be "4.3.2.1".
// Once connected to the access point, open up 4.3.2.1 in a browser to get
// to the credentials form.
const IPAddress AccessPoint::IP (4, 3, 2, 1);
const IPAddress AccessPoint::Netmask (255, 255, 255, 0);

// Main webpage HTML with form to collect WiFi credentials.
const char *AccessPoint::htmlSetup =
    HTML_HEADER
    "<h1>Noise Meter Setup</h1>"
    "<form method='POST' action='' enctype='multipart/form-data'>"
    "<p>SSID:</p>"
    "<input type='text' name='ssid'>"
    "<p>Password:</p>"
    "<input type='password' name='psk'>"
    "<input type='submit' value='Connect'>"
    "</form>"
    HTML_FOOTER;

// HTML to show after credentials are submitted.
const char *AccessPoint::htmlSubmit =
    HTML_HEADER
    "<h1>Noise Meter Setup</h1>"
    "<p>Connecting...</p>"
    HTML_FOOTER;

void AccessPoint::begin()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IP, IP, Netmask);
    WiFi.softAP(SSID, Passkey);

    // GET request means user wants to see the form.
    // POST request means user has submitted data through the form.
    server.on("/", HTTP_GET,
        [this] { server.send_P(200, PSTR("text/html"), htmlSetup); });
    server.on("/", HTTP_POST,
        [this] {
            server.client().setNoDelay(true);
            server.send_P(200, PSTR("text/html"), htmlSubmit);
            if (funcOnCredentialsReceived)
                funcOnCredentialsReceived(server);
        });
    server.begin();

    SERIAL.println("Running setup access point.");
}

[[noreturn]]
void AccessPoint::run()
{
    while (1)
        server.handleClient();
}

void AccessPoint::onCredentialsReceived(void (*func)(WebServer&))
{
    funcOnCredentialsReceived = func;
}


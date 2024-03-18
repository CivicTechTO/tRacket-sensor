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

constexpr int DNSPort = 53;

const IPAddress AccessPoint::IP (8, 8, 4, 4);
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

[[noreturn]]
void AccessPoint::run()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IP, IP, Netmask);
    WiFi.softAP(SSID, Passkey);

    dns.start(DNSPort, "*", IP);

    server.addHandler(this);
    server.begin();

    SERIAL.println("Running setup access point.");

    while (1) {
        dns.processNextRequest();
        server.handleClient();
        delay(10);
    }
}

bool AccessPoint::canHandle(HTTPMethod, String)
{
    return true;
}

bool AccessPoint::handle(WebServer& server, HTTPMethod method, String uri)
{
    if (method == HTTP_POST) {
        if (uri == "/") {
            server.client().setNoDelay(true);
            server.send_P(200, PSTR("text/html"), htmlSubmit);
            if (onCredentialsReceived)
                onCredentialsReceived(server);
        } else {
            server.sendHeader("Location", "http://8.8.4.4/");
            server.send(301);
        }
    } else if (method == HTTP_GET) {
        // Redirects taken from https://github.com/CDFER/Captive-Portal-ESP32

        if (uri == "/") {
            server.send_P(200, PSTR("text/html"), htmlSetup);
        } else if (uri == "/connecttest.txt") {
            // windows 11 captive portal workaround
            server.sendHeader("Location", "http://logout.net");
            server.send(301);
        } else if (uri == "/wpad.dat") {
            // Honestly don't understand what this is but a 404 stops win 10
            // keep calling this repeatedly and panicking the esp32 :)
            server.send(404);
        } else if (uri == "/success.txt") {
            // firefox captive portal call home
            server.send(200);
        } else if (uri == "/favicon.ico") {
            // return 404 to webpage icon
            server.send(404);
        } else {
            server.sendHeader("Location", "http://8.8.4.4/");
            server.send(301);
        }
    } else {
        server.sendHeader("Location", "http://8.8.4.4/");
        server.send(301);
    }

    return true;
}


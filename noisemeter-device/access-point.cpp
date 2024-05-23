/* noisemeter-device - Firmware for CivicTechTO's Noisemeter Device
 * Copyright (C) 2024  Clyne Sullivan, Nick Barnard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "access-point.h"
#include "board.h"
#include "timestamp.h"

#include <WiFiAP.h>

#define HTML_HEADER \
    "<!DOCTYPE html>" \
    "<html lang='en'>" \
    "<head>" \
    "<meta charset='utf-8'>" \
    "<meta name='viewport' content='width=device-width,initial-scale=1'/>" \
    "</head>" \
    "<body>" \
    "<h1>Noise Meter Setup</h1>"

#define HTML_FOOTER \
    "</body>" \
    "</html>"

constexpr int DNSPort = 53;
constexpr auto ACCESS_POINT_TIMEOUT_SEC = MIN_TO_SEC(30);

const IPAddress AccessPoint::IP (8, 8, 4, 4);
const IPAddress AccessPoint::Netmask (255, 255, 255, 0);

// Main webpage HTML with form to collect WiFi credentials.
const char *AccessPoint::htmlSetup =
    HTML_HEADER
    "<form method='POST' action='' enctype='multipart/form-data'>"
    "<p>SSID:</p>"
    "<input type='text' name='ssid' required>"
    "<p>Password:</p>"
    "<input type='password' name='psk' required>"
    "<p>Email (if registering new device):</p>"
    "<input type='email' name='email'>"
    "<input type='submit' value='Connect'>"
    "</form>"
    HTML_FOOTER;

// HTML to show after credentials are submitted.
const char *AccessPoint::htmlSubmit =
    HTML_HEADER
    "<p>Connected and registered! Restarting...</p>"
    HTML_FOOTER;

AccessPoint::AccessPoint(SubmissionHandler func):
    timeout(ACCESS_POINT_TIMEOUT_SEC),
    server(80),
    onCredentialsReceived(func) {}

// HTML to show when a submission is rejected.
String AccessPoint::htmlFromMsg(const char *msg)
{
    String html (HTML_HEADER);
    html += "<p>";
    html += msg;
    html += "</p>";
    html += "<p>Please <a href='/'>go back and try again</a>.</p>";
    html += HTML_FOOTER;
    return html;
}

void AccessPoint::run()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IP, IP, Netmask);
    WiFi.softAP(SSID, Passkey);

    dns.start(DNSPort, "*", IP);

    server.addHandler(this);
    server.begin();

    SERIAL.println("Running setup access point.");

    for (auto start = 0UL; start / 100 < timeout; ++start) {
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

            if (onCredentialsReceived) {
                auto msg = onCredentialsReceived(server);
                if (!msg) {
                    server.send_P(200, PSTR("text/html"), htmlSubmit);
                    delay(3000);
                    ESP.restart(); // Software reset.
                } else {
                    auto msgStr = htmlFromMsg(*msg);
                    server.send_P(200, PSTR("text/html"), msgStr.c_str());
                    WiFi.mode(WIFI_AP);
                }
            }
        } else {
            server.sendHeader("Location", "http://8.8.4.4/");
            server.send(301);
        }
    } else if (method == HTTP_GET) {
        // Redirects taken from https://github.com/CDFER/Captive-Portal-ESP32

        if (uri == "/") {
            timeout = DAY_TO_SEC(30);
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


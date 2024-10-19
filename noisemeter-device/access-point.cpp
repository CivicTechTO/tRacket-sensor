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
#include "access-point-html.hpp"
#include "board.h"
#include "timestamp.h"

#include <WiFiAP.h>

constexpr int DNSPort = 53;
constexpr auto ACCESS_POINT_TIMEOUT_SEC = MIN_TO_SEC(30);

const IPAddress AccessPoint::IP (8, 8, 4, 4);
const IPAddress AccessPoint::Netmask (255, 255, 255, 0);

static int networkScanCount = 0;

AccessPoint::AccessPoint(SubmissionHandler func):
    timeout(ACCESS_POINT_TIMEOUT_SEC),
    server(80),
    onCredentialsReceived(func) {}

// HTML to show when a submission is rejected.
String AccessPoint::htmlFromMsg(const char *msg, const char *extra)
{
    String html;
    html.reserve(2048);
    html += HTML_HEADER;
    html += HTML_CONTAINER;
    html += "<p>";
    html += msg;
    html += "</p>";
    if (extra)
        html += extra;
    html += HTML_FOOTER;
    return html;
}

void AccessPoint::run()
{
    networkScanCount = WiFi.scanNetworks();

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IP, IP, Netmask);
    WiFi.softAP(SSID, Passkey);

    dns.start(DNSPort, "*", IP);

    server.addHandler(this);
    server.begin();

    SERIAL.println("Running setup access point.");

    restarting = false;
    for (auto start = 0UL; start / (1000 / 10) < timeout; ++start) {
        dns.processNextRequest();
        server.handleClient();
        delay(10);

        if (restarting) {
            delay(5000);
            ESP.restart(); // Software reset.
        }
    }
}

bool AccessPoint::canHandle(HTTPMethod, String)
{
    return true;
}

void AccessPoint::taskOnCredentialsReceived(void *param)
{
    auto ap = reinterpret_cast<AccessPoint *>(param);

    if (ap->onCredentialsReceived) {
        const auto msg = ap->onCredentialsReceived(ap->ssid, ap->psk, ap->email);

        if (msg) {
            ap->finishHtml = htmlFromMsg(
                *msg,
                "<p>Please <a href=\"http://8.8.4.4/\">go back and try again</a>.</p>");
        } else {
            ap->finishHtml = htmlFromMsg(
                "The sensor has connected; the sensor will now restart and begin monitoring noise levels. "
                "You will receive an email with a link to your dashboard page.");
        }

        ap->finishGood = !msg;
        ap->complete = true;
    }

    vTaskDelete(xTaskGetHandle("credrecv"));
    while (1)
        delay(1000);
}

static String waitingHtml()
{
    String html;
    html.reserve(2048);
    html += HTML_HEADER;
    html += "<meta http-equiv=\"refresh\" content=\"2;url=http://8.8.4.4/submit\"/>";
    html += HTML_CONTAINER;
    html += "<p>The sensor is trying to connect to wifi...</p>"
            "<div class='meter'><span style='width:100%;'><span class='progress'></span></span></div>";
    html += HTML_FOOTER;
    return html;
}

bool AccessPoint::handle(WebServer& server, HTTPMethod method, String uri)
{
    if (method == HTTP_POST) {
        if (uri == "/submit") {
            const auto html = waitingHtml();
            server.client().setNoDelay(true);
            server.send_P(200, PSTR("text/html"), html.c_str());

            ssid = server.arg("ssid");
            psk = server.arg("psk");
            email = server.arg("email");
            complete = false;
            xTaskCreate(taskOnCredentialsReceived, "credrecv", 10000, this, 1, nullptr);
        } else {
            server.sendHeader("Location", "http://8.8.4.4/");
            server.send(301);
        }
    } else if (method == HTTP_GET) {
        // Redirects taken from https://github.com/CDFER/Captive-Portal-ESP32

        if (uri == "/submit") {
            server.client().setNoDelay(true);

            if (!complete) {
                const auto html = waitingHtml();
                server.send_P(200, PSTR("text/html"), html.c_str());
            } else {
                server.send_P(200, PSTR("text/html"), finishHtml.c_str());

                if (finishGood)
                    restarting = true;
                else
                    WiFi.mode(WIFI_AP); // Restart access point
            }
        } else if (uri == "/") {
            String response;
            response.reserve(2048);
            response += HTML_HEADER;
            response += HTML_CONTAINER;
            response += HTML_BODY_FORM_HEADER;

            for (int i = 0; i < networkScanCount; ++i) {
                const auto ssid = WiFi.SSID(i);

                response += "<option value=\"";
                response += ssid;
                response += "\">";
                response += ssid;
                if (auto ty = WiFi.encryptionType(i); ty != WIFI_AUTH_OPEN)
                    response += " *";
                response += "</option>";
            }

            response += HTML_BODY_FORM_FOOTER;
            response += HTML_FOOTER;

            timeout = DAY_TO_SEC(30);
            server.send_P(200, PSTR("text/html"), response.c_str());
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


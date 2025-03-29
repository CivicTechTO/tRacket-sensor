/* noisemeter-device - Firmware for CivicTechTO's Noisemeter Device
 * Copyright (C) 2024  Clyne Sullivan
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
#include "api.h"
#include "board.h"
#include "certs.h"
#include "url-encode.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <cmath>

API::Request::Request(const char endpoint[])
{
    url.reserve(128);
    url.concat(Base);
    url.concat(endpoint);
    params.reserve(128);
}

API::Request& API::Request::addParam(const char param[], String value)
{
    params.concat('&');
    params.concat(param);
    params.concat('=');
    params.concat(urlEncode(value));
    return *this;
}

std::optional<JsonDocument> API::sendAuthorizedRequest(const API::Request& req)
{
    WiFiClientSecure client;
    client.setCACert(rootCertificate());

#ifdef API_VERBOSE
    SERIAL.print("[api] Authorized request: ");
    SERIAL.println(req.url);
#endif

    HTTPClient https;
    if (https.begin(client, req.url)) {
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");
        https.addHeader("Authorization", String("Token ") + token);
        https.addHeader("X-Tracket-Device", id);
        return sendHttpPOST(https, req.params.substring(1));
#ifdef API_VERBOSE
    } else {
        SERIAL.println("[api] Failed to https.begin()");
#endif
    }

    return {};
}

std::optional<JsonDocument> API::sendNonauthorizedRequest(const API::Request& req)
{
    WiFiClientSecure client;
    client.setCACert(rootCertificate());

#ifdef API_VERBOSE
    SERIAL.print("[api] Non-authorized request: ");
    SERIAL.println(req.url);
#endif

    HTTPClient https;
    if (https.begin(client, req.url)) {
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");
        https.addHeader("X-Tracket-Device", id);
        return sendHttpPOST(https, req.params.substring(1));
#ifdef API_VERBOSE
    } else {
        SERIAL.println("[api] Failed to https.begin()");
#endif
    }

    return {};
}

std::optional<JsonDocument> API::sendHttpGET(HTTPClient& https)
{
    const auto code = https.GET();
    if (code == HTTP_CODE_OK || code == HTTP_CODE_MOVED_PERMANENTLY) {
        return handleHttpResponse(https);
#ifdef API_VERBOSE
    } else {
        SERIAL.print("[api] HTTP error: ");
        SERIAL.println(code);
#endif
    }

    return {};
}

std::optional<JsonDocument> API::sendHttpPOST(HTTPClient& https, const String& payload)
{
#ifdef API_VERBOSE
    SERIAL.print("[api] payload: ");
    SERIAL.println(payload);
#endif

    const auto code = https.POST(payload);
    if (code == HTTP_CODE_OK || code == HTTP_CODE_MOVED_PERMANENTLY) {
        return handleHttpResponse(https);
#ifdef API_VERBOSE
    } else {
        SERIAL.print("[api] HTTP error: ");
        SERIAL.println(code);
#endif
    }

    return {};
}

std::optional<JsonDocument> API::handleHttpResponse(HTTPClient& https)
{
    const auto response = https.getString();
    const auto json = responseToJson(response);
    https.end();

    if (json) {
#ifdef API_VERBOSE
        SERIAL.print("[api] ");
        SERIAL.print((String)(*json)["result"]);
        SERIAL.print(": ");
        SERIAL.println((String)(*json)["message"]);
#endif

        return *json;
#ifdef API_VERBOSE
    } else {
        SERIAL.print("[api] Invalid JSON!");
#endif
    }

    return {};
}

std::optional<JsonDocument> API::responseToJson(const String& response)
{
    JsonDocument doc;
    const auto error = deserializeJson(doc, response);

    if (error) {
        SERIAL.println(error.f_str());
        return {};
    } else {
        return doc;
    }
}

API::API(UUID id_, String token_):
    id(id_), token(token_) {}

bool API::sendMeasurement(const DataPacket& packet)
{
    const auto request = Request("measurement")
        .addParam("timestamp", packet.timestamp)
        .addParam("min",       String(std::lround(packet.minimum)))
        .addParam("max",       String(std::lround(packet.maximum)))
        .addParam("mean",      String(std::lround(packet.average)));

    const auto resp = sendAuthorizedRequest(request);
    return resp && (*resp)["result"] == "ok";
}

bool API::sendMeasurementWithDiagnostics(const DataPacket& packet, String version, String boottime)
{
    const auto request = Request("measurement")
        .addParam("timestamp", packet.timestamp)
        .addParam("min",       String(std::lround(packet.minimum)))
        .addParam("max",       String(std::lround(packet.maximum)))
        .addParam("mean",      String(std::lround(packet.average)))
        .addParam("version",   version)
        .addParam("boottime",  boottime);

    const auto resp = sendAuthorizedRequest(request);
    return resp && (*resp)["result"] == "ok";
}

std::optional<String> API::sendRegister(String email)
{
    const auto request = Request("device/register")
        .addParam("email", email);

    const auto resp = sendNonauthorizedRequest(request);
    if (resp && (*resp)["result"] == "ok")
        return (*resp)["token"];
    else
        return {};
}

std::optional<API::LatestSoftware> API::getLatestSoftware()
{
    const auto request = Request("software/latest");

    WiFiClientSecure client;
    client.setCACert(rootCertificate());

    String endpoint = request.url + '?' + request.params.substring(1);

#ifdef API_VERBOSE
    SERIAL.print("[api] Non-authorized request: ");
    SERIAL.println(endpoint);
#endif

    HTTPClient https;
    if (https.begin(client, endpoint)) {
        const auto resp = sendHttpGET(https);

        if (resp && (*resp)["result"] == "ok") {
            LatestSoftware ls = {
                (*resp)["version"],
                (*resp)["url"]
            };

            return ls;
        }
#ifdef API_VERBOSE
    } else {
        SERIAL.println("[api] Failed to https.begin()");
#endif
    }

    return {};
}

const char *API::rootCertificate()
{
    return cert_ISRG_Root_X1;
}


/// @file
/// @brief API implementation for communication with server
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
#ifndef API_H
#define API_H

#include "data-packet.h"
#include "UUID/UUID.h"

#include <ArduinoJson.h>
#include <WString.h>

#include <optional>

class API
{
    constexpr static const char Base[] = "https://noisemeter.webcomand.com/api/v1/";

    struct Request {
        Request(const char endpoint[]);

        Request& addParam(const char param[], String value);

        String url;
    };

public:
    struct LatestSoftware {
        String version;
        String url;
    };

    API(UUID id_, String token_ = {});

    bool sendMeasurement(const DataPacket& packet);
    bool sendDiagnostics(String version, String boottime);
    std::optional<String> sendRegister(String email);
    std::optional<LatestSoftware> getLatestSoftware();

    static const char *rootCertificate();

private:
    UUID id;
    String token;

    std::optional<JsonDocument> responseToJson(const String& response);
    std::optional<JsonDocument> sendAuthorizedRequest(const Request& req);
    std::optional<JsonDocument> sendUnauthorizedRequest(const Request& req);
};

#endif // API_H


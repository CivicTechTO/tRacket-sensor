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

class HTTPClient;

/**
 * @brief Provides interface for API calls.
 */
class API
{
    /** API host / base URL. */
    constexpr static const char Base[] = "https://api.tracket.info/v1/";

    /** String builder to create URLs for API endpoints and requests. */
    struct Request {
        /**
         * Initializes a new request with zero parameters.
         * @param endpoint Endpoint for the API request.
         */
        Request(const char endpoint[]);

        /**
         * Adds a parameter to the given request.
         * Resulting URL e.g. "https://host/endpoint?parm1=123&parm2=456&"
         * @param param Name of parameter to add.
         * @param value Value of the added parameter.
         * @return *this
         */
        Request& addParam(const char param[], String value);

        /** Stores the resulting request URL. */
        String url;
        String params;
    };

public:
    /** Results of the software update endpoint. */
    struct LatestSoftware {
        /** Version number of latest available software. */
        String version;
        /** Download URL for latest software. */
        String url;
    };

    /**
     * Creates a new API interface for the given device (ID).
     * @param id_ Device UUID
     * @param token_ API token (required for authorized API calls)
     */
    API(UUID id_, String token_ = {});

    /**
     * Sends a DataPacket (dB measurement) to the server.
     * This request requires authentication.
     * @param packet Packet to be sent.
     * @return True on success
     */
    bool sendMeasurement(const DataPacket& packet);

    /**
     * Sends diagnostic/analytic data to the server along with a DataPacket.
     * This request requires authentication.
     * @param packet Packet to be sent.
     * @param version Device's software version number.
     * @param boottime Timestamp of last connection to the internet.
     * @return True on success
     */
    bool sendMeasurementWithDiagnostics(const DataPacket& packet, String version, String boottime);

    /**
     * Attempts to register the device with the given email.
     * @param email Email to register with.
     * @return Device's new API token if successful.
     */
    std::optional<String> sendRegister(String email);

    /**
     * Attempts to get the latest available software info (for OTA).
     * @return LatestSoftware if successful.
     */
    std::optional<LatestSoftware> getLatestSoftware();

    /**
     * Provides the server's root certificate for non-API HTTPS requests.
     */
    static const char *rootCertificate();

private:
    /** Device's UUID. */
    UUID id;
    /** Device's API token for authorized requests. */
    String token;

    /** Converts response string into JSON. */
    std::optional<JsonDocument> responseToJson(const String& response);
    /** Attempts the given request and returns the JSON response on success. */
    std::optional<JsonDocument> sendAuthorizedRequest(const Request& req);
    /** Attempts the given request and returns the JSON response on success. */
    std::optional<JsonDocument> sendNonauthorizedRequest(const Request& req);

    std::optional<JsonDocument> sendHttpRequest(HTTPClient& https, const String& payload);
};

#endif // API_H


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
#include "ota-update.h"

#include <array>
#include <cstdint>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#include "board.h"

static bool applyUpdate(WiFiClientSecure& client, int totalSize);

bool downloadOTAUpdate(String url, String rootCA)
{
    if (url.isEmpty())
        return false;

    WiFiClientSecure client;
    client.setCACert(rootCA.c_str());

    HTTPClient https;
    if (https.begin(client, url)) {
        const auto code = https.GET();

        if (code == HTTP_CODE_OK || code == HTTP_CODE_MOVED_PERMANENTLY) {
            return applyUpdate(client, https.getSize());
        } else {
            SERIAL.print("Bad HTTP response: ");
            SERIAL.println(code);
        }
    } else {
        SERIAL.println("Unable to connect.");
    }

    return false;
}

bool applyUpdate(WiFiClientSecure& client, int totalSize)
{
    std::array<uint8_t, 128> buffer;

    if (totalSize <= 0) {
        //SERIAL.println("Warning: Unable to determine update size.");
        //totalSize = UPDATE_SIZE_UNKNOWN;
        SERIAL.println("Unknown update size, stop.");
        return false;
    }

    if (!Update.begin(totalSize)) {
        SERIAL.println("Failed to begin Update.");
        return false;
    }

    while (client.connected() && (totalSize > 0 || totalSize == UPDATE_SIZE_UNKNOWN)) {
        const auto size = client.available();

        if (size > 0) {
            const auto bytesToRead = std::min(static_cast<int>(buffer.size()), size);
            const auto bytesRead = client.read(buffer.data(), bytesToRead);

            if (!Update.write(buffer.data(), bytesRead)) {
                SERIAL.println("Failed to write Update.");
                return false;
            }

            if (totalSize > 0)
                totalSize -= bytesRead;
        } else {
            delay(1);
        }
    }

    if (totalSize == 0)
        Update.end(true);

    return totalSize == 0;
}


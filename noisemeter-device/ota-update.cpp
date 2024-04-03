#include "ota-update.h"

#include <array>
#include <cstdint>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>

#include "board.h"

constexpr auto OTA_JSON_URL = "https://live.noisemeter.webcomand.com/updates/latest.json";

bool OTAUpdate::available()
{
    WiFiClientSecure client;
    client.setCACert(rootCA);

    HTTPClient https;
    if (https.begin(client, OTA_JSON_URL)) {
        const auto code = https.GET();

        if (code == HTTP_CODE_OK || code == HTTP_CODE_MOVED_PERMANENTLY) {
            const auto response = client.readString();

            JsonDocument doc;
            const auto error = deserializeJson(doc, response);
            if (!error) {
                version = doc["Version"].as<String>();
                if (version.compareTo(NOISEMETER_VERSION) > 0) {
                    url = doc["URL"].as<String>();
                    return !url.isEmpty();
                } else {
                    SERIAL.print("Server version: ");
                    SERIAL.println(version);
                }
            } else {
                SERIAL.print("json failed: ");
                SERIAL.println(error.f_str());
            }
        } else {
            SERIAL.print("Bad HTTP response: ");
            SERIAL.println(code);
        }
    } else {
        SERIAL.println("Unable to connect.");
    }

    return false;
}

bool OTAUpdate::download()
{
    if (url.isEmpty())
        return false;

    WiFiClientSecure client;
    client.setCACert(rootCA);

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

bool OTAUpdate::applyUpdate(WiFiClientSecure& client, int totalSize)
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


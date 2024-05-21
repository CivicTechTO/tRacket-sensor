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
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <mbedtls/md_internal.h>

#include "board.h"

// Public key for OTA signature verification
const unsigned char cert_ota [] PROGMEM = R"CERT(
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAhe0evwkVr5J0ZcnPSiAY
75S1PP61ln95amfwQ4K9xCbaqHzizGVsfzKIbDHDmFMVQkNdFQRAewGit01WPZNJ
uj08vbWKcj00W/dNiMyL82+3/VSKgsWTRTrjq2PoqWXrAcYyXOPc+Nq+b+Qzhdfn
kZferH1Mkyt4mhEUoKeYRlU+lwPBzMQgeBMKYp2E8R6mtM3vscK+zpL7M65DKRay
R2ckugQeTQOOLga3lvjxgnq6yKNhRttNSLYQifAbGEftze58DwTbEaexZlTowidK
ysqIsN8SYmUFKg5CDLf39LEPFfEakLxKkoOm/2fhyzhdcTqJyVEWxKVrsplfWKLH
Bb+AKvQ0v7xbM8d3yok/wd1EJ2xEoyv7x5RJccyE7/WhoiEN5gZfwnlZnN+H10Qx
DH3BYzId+PWw9CIMkuqNME5G/h22eEzR8Z7gzQBdn5GKYHMKVLx3w9X7DsJ3RpRb
b388AnSeku0Wsorx2RhEdnjKe3FiNsbM7GXs94sIMj8X6fr+tY+K5acN0L3sAcaK
azGkUkeBYClDYT/cVWedSscUw02u0sONh4j8ayG3aDivA4BZndxQb/NJzxRD9C7e
b3NXshQuTUV+KePuT18B6J6mslQHJIO+jmCEnLh9eFKwjxHfO4mYAFL5RC/RprDz
STejf1pUQ7jnm6WdvwSBupkCAwEAAQ==
-----END PUBLIC KEY-----
)CERT";

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
    static std::array<uint8_t, 512> buffer;
    static std::array<uint8_t, 512> signature;
    mbedtls_pk_context pk;
    mbedtls_md_context_t rsa;
    mbedtls_pk_init( &pk );

    if (mbedtls_pk_parse_public_key(&pk, cert_ota, sizeof(cert_ota)) != 0) {
        SERIAL.println("Parsing public key failed!");
        return false;
    }

    if (!mbedtls_pk_can_do(&pk, MBEDTLS_PK_RSA)) {
        SERIAL.println("Public key is not an rsa key!");
        return false;
    }

    auto mdinfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&rsa);
    mbedtls_md_setup(&rsa, mdinfo, 0);
    mbedtls_md_starts(&rsa);


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

    bool first = true;
    while (client.connected() && (totalSize > 0 || totalSize == UPDATE_SIZE_UNKNOWN)) {
        const auto size = client.available();

        if (size > 0) {
            const auto bytesToRead = std::min(static_cast<int>(buffer.size()), size);
            const auto bytesRead = client.read(buffer.data(), bytesToRead);

            if (first) {
                if (bytesRead != signature.size()) {
                    SERIAL.println("Failed to read signature!");
                    return false;
                }
                std::copy(buffer.cbegin(), buffer.cend(), signature.begin());
                first = false;
            } else {
                if (!Update.write(buffer.data(), bytesRead)) {
                    SERIAL.println("Failed to write Update.");
                    return false;
                }
            }

            mbedtls_md_update(&rsa, buffer.data(), bytesRead);
            if (totalSize > 0)
                totalSize -= bytesRead;
        } else {
            delay(1);
        }
    }

    if (totalSize == 0) {
        unsigned char hash[mdinfo->size];
        mbedtls_md_finish(&rsa, hash);

        auto ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, mdinfo->size,
            signature.data(), signature.size());
        mbedtls_md_free(&rsa);
        mbedtls_pk_free(&pk);

        if (ret == 0) {
            Update.end(true);
            return true;
        } else {
            // validation failed, overwrite the first few bytes so this partition won't boot!
            SERIAL.println("Signature validation failed!");
            //ESP.partitionEraseRange( partition, 0, ENCRYPTED_BLOCK_SIZE);
            Update.abort();
        }
    }

    return false;
}


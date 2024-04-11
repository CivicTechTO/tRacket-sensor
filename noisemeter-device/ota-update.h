/// @file
/// @brief Facilities for installing over-the-air (OTA) software updates
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
#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <WString.h>
#include <WiFiClientSecure.h>

/**
 * Manages fetching and installing of OTA software updates.
 */
struct OTAUpdate
{
    /** Stores the latest version string from the server (for logging). */
    String version;

    /**
     * Creates an OTAUpdate object.
     * @param rootCA_ Root certificate to use for HTTPS requests.
     */
    OTAUpdate(const char *rootCA_):
        rootCA(rootCA_) {}

    /**
     * Checks if a new OTA update is available.
     * @return True if available.
     */
    bool available();

    /**
     * Downloads and applies the latest OTA update.
     * @return True if update is successfully downloaded and installed.
     */
    bool download();

private:
    /** Stores fetched URL for the latest update. */
    String url;
    /** Stores the given root certificate for HTTPS. */
    const char *rootCA;

    /**
     * Writes the received OTA update to flash memory.
     * @param client An active client for the update download.
     * @param totalSize The total size of the update if known.
     * @return True if update is successfully installed.
     */
    bool applyUpdate(WiFiClientSecure& client, int totalSize);
};

#endif // OTA_UPDATE_H


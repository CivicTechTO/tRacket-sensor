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

struct OTAUpdate
{
    String version;

    // Root CA needs to be passed to the object due to linking issues.
    // TODO this is not a good solution, fix it.
    OTAUpdate(const char *rootCA_):
        rootCA(rootCA_) {}

    // Checks if a new OTA update is available, returns true if so.
    bool available();

    // Downloads and applies the latest OTA update, returns true if successful.
    bool download();

private:
    String url;
    const char *rootCA;

    // Writes the received OTA update to flash memory, returns true on success.
    bool applyUpdate(WiFiClientSecure& client, int totalSize);
};

#endif // OTA_UPDATE_H


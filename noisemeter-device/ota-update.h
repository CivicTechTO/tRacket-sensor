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

/**
 * Downloads and applies the latest OTA update.
 * @return True if update is successfully downloaded and installed.
 */
bool downloadOTAUpdate(String url, String rootCA);

#endif // OTA_UPDATE_H


/// @file
/// @brief Management for storage of collected data points
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
#ifndef DATAPACKET_H
#define DATAPACKET_H

#include "timestamp.h"

#include <algorithm>

/**
 * Stores data points included in an uploaded "measurement".
 */
struct DataPacket
{
    /**
     * Creates a new DataPacket with sane defaults and an invalid timestamp.
     */
    constexpr DataPacket() = default;

    /**
     * Factors a sample into the packet's data points.
     * @param sample The dB sample value to add.
     */
    void add(float sample) noexcept {
        count++;
        minimum = std::min(minimum, sample);
        maximum = std::max(maximum, sample);
        average += (sample - average) / count;
    }

    /** Number of data points added to this DataPacket. */
    int count = 0;

    /** Minimum decibel value within the aggregated points. */
    float minimum = 999.f;

    /** Maximum decibel value within the aggregated points. */
    float maximum = 0.f;

    /** Average decibel value of the aggregated points. */
    float average = 0.f;

    /**
     * Timestamp to indicate the ending time point of this DataPacket's
     * aggregation. This should be manually set before the DataPacket is
     * uploaded to the server.
     */
    Timestamp timestamp = Timestamp::invalidTimestamp();
};

#endif // DATAPACKET_H


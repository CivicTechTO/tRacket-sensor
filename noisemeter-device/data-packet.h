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

struct DataPacket
{
    constexpr DataPacket() = default;

    void add(float sample) noexcept {
        count++;
        minimum = std::min(minimum, sample);
        maximum = std::max(maximum, sample);
        average += (sample - average) / count;
    }

    int count = 0;
    float minimum = 999.f;
    float maximum = 0.f;
    float average = 0.f;
    Timestamp timestamp = Timestamp::invalidTimestamp();
};

#endif // DATAPACKET_H


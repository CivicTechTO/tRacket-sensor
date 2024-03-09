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


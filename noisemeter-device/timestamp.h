/// @file
/// @brief Management of timestamping and NTP synchronization
/* noisemeter-device - Firmware for CivicTechTO's Noisemeter Device
 * Copyright (C) 2024  Clyne Sullivan, Nick Barnard
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
#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <Arduino.h>
#include <ctime>

#define SEC_TO_MS(s)  (s * 1000)
#define MIN_TO_SEC(m) (m * 60)
#define HR_TO_SEC(h)  MIN_TO_SEC(60)
#define DAY_TO_SEC(h) HR_TO_SEC(24)

/** Maximum number of milliseconds to wait for NTP sync to succeed. */
constexpr auto NTP_CONNECT_TIMEOUT_MS = SEC_TO_MS(20);

/**
 * Timestamping facility that uses NTP to provide accurate date and time.
 */
class Timestamp
{
public:
    /**
     * Creates a new timestamp.
     * @param tm_ time_t value for timestamp, default to now.
     */
    Timestamp(std::time_t tm_ = std::time(nullptr)):
        tm(tm_) {}

    /**
     * Determines if the timestamp is valid.
     * @return True if the timestamp is valid.
     */
    bool valid() const noexcept {
        return tm >= HR_TO_SEC(16);
    }

    /**
     * Converts the timestamp to a human-readable string.
     * Useful for serialization and timestamping of data packets.
     */
    operator String() const noexcept {
        char tsbuf[32];
        const auto timeinfo = std::gmtime(&tm);
        const auto success = std::strftime(tsbuf, sizeof(tsbuf), "%c", timeinfo) > 0;

        return success ? tsbuf : "(error)";
    }

    /**
     * Determines the number of seconds between this and the given timestamp.
     * @param ts Timestamp to compare against
     * @return Time difference in seconds as a floating point value
     */
    auto secondsBetween(Timestamp ts) const noexcept {
        return std::difftime(ts.tm, tm);
    }

    /**
     * Synchronizes system time with an NTP time server.
     * Requires the device to be connected to the general internet.
     * Will eventually timeout if synchronization does not succeed.
     * @see NTP_CONNECT_TIMEOUT_MS
     * @return Zero on success or a negative number on failure
     */
    static int synchronize() {
        configTime(0, 0, "pool.ntp.org");

        const auto start = millis();
        bool connected;

        do {
            delay(1000);
            connected = Timestamp().valid();
        } while (!connected && millis() - start < NTP_CONNECT_TIMEOUT_MS);

        return connected ? 0 : -1;
    }

    /**
     * Provides a timestamp that is guaranteed to be invalid.
     * @return The invalid timestamp
     */
    static Timestamp invalidTimestamp() {
        return Timestamp(0);
    }

private:
    /** Stores system time */
    std::time_t tm;
};

#endif // TIMESTAMP_H


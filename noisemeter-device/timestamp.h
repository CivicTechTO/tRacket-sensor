#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <Arduino.h>
#include <ctime>

constexpr auto NTP_CONNECT_TIMEOUT_MS = 20 * 1000;

class Timestamp
{
public:
    Timestamp(std::time_t tm_ = std::time(nullptr)):
        tm(tm_) {}

    bool valid() const noexcept {
        return tm >= 8 * 3600 * 2;
    }

    operator String() const noexcept {
        char tsbuf[32];
        const auto timeinfo = std::gmtime(&tm);
        const auto success = std::strftime(tsbuf, sizeof(tsbuf), "%c", timeinfo) > 0;

        return success ? tsbuf : "(error)";
    }

    auto secondsBetween(Timestamp ts) const noexcept {
        return std::difftime(ts.tm, tm);
    }

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

    static Timestamp invalidTimestamp() {
        return Timestamp(0);
    }

private:
    std::time_t tm;
};

#endif // TIMESTAMP_H


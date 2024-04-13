#ifndef SPL_METER_H
#define SPL_METER_H

#include <array>
#include <cstdint>
#include <driver/i2s.h>
#include <optional>

class SPLMeter
{
public:
    static constexpr auto SAMPLE_RATE = 48000u;

    // Prepares I2S Driver and mic config.
    void initMicrophone() noexcept;

    // Reads data to Microphones buffer
    std::optional<float> readMicrophoneData() noexcept;

private:
    union sample_t {
        std::int32_t i;
        float f;
    };
    static_assert(sizeof(float) == sizeof(std::int32_t));

    static constexpr auto SAMPLE_BITS = sizeof(sample_t) * 8u;
    static constexpr auto SAMPLES_SHORT = SAMPLE_RATE / 8u;
    static const i2s_config_t i2s_config;
    static const i2s_pin_config_t pin_config;

    alignas(4)
    std::array<sample_t, SAMPLES_SHORT> samples;

    // Sampling accumulators
    unsigned Leq_samples = 0;
    float Leq_sum_sqr = 0;

    void i2sRead() noexcept;

    static constexpr std::int32_t micConvert(std::int32_t s);
};

#endif // SPL_METER_H


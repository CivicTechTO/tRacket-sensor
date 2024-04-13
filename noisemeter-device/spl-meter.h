#ifndef SPL_METER_H
#define SPL_METER_H

#include <array>
#include <cstdint>
#include <optional>

#define SAMPLE_RATE 48000  // Hz, fixed to design of IIR filters
#define SAMPLE_BITS 32     // bits
#define SAMPLES_SHORT (SAMPLE_RATE / 8)  // ~125ms

#define MIC_OVERLOAD_DB 120.f // dB - Acoustic overload point
#define MIC_NOISE_DB     29.f // dB - Noise floor

class SPLMeter
{
    static_assert(sizeof(float) == sizeof(std::int32_t));
    union sample_t {
        std::int32_t i;
        float f;
    };

public:
    // Prepares I2S Driver and mic config.
    void initMicrophone() noexcept;

    // Reads data to Microphones buffer
    std::optional<float> readMicrophoneData() noexcept;

private:
    // Buffer for block of samples
    alignas(4)
    std::array<sample_t, SAMPLES_SHORT> samples;

    // Sampling accumulators
    unsigned Leq_samples = 0;
    float Leq_sum_sqr = 0;

    void i2sRead();
};

#endif // SPL_METER_H


#ifndef SPL_METER_H
#define SPL_METER_H

#include <cstdint>
#include <optional>

#define SAMPLE_RATE 48000  // Hz, fixed to design of IIR filters
#define SAMPLE_BITS 32     // bits
#define SAMPLE_T int32_t
#define SAMPLES_SHORT (SAMPLE_RATE / 8)  // ~125ms

#define MIC_OVERLOAD_DB 120.0  // dB - Acoustic overload point
#define MIC_NOISE_DB 29        // dB - Noise floor

static_assert(sizeof(float) == sizeof(SAMPLE_T));
using SampleBuffer alignas(4) = float[SAMPLES_SHORT];

class SPLMeter
{
public:
    // Prepares I2S Driver and mic config.
    void initMicrophone() noexcept;

    // Reads data to Microphones buffer
    std::optional<float> readMicrophoneData() noexcept;

private:
    // Buffer for block of samples
    SampleBuffer samples;

    // Sampling accumulators
    unsigned Leq_samples = 0;
    double Leq_sum_sqr = 0;

    void i2sRead();
};

#endif // SPL_METER_H


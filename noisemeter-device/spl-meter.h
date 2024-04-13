/// @file
/// @brief Microphone sampling and filtering for decibel metering
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
#ifndef SPL_METER_H
#define SPL_METER_H

#include <array>
#include <cstdint>
#include <driver/i2s.h>
#include <optional>

/**
 * Provides a minimal interface for reading decibel levels from the microphone.
 */
class SPLMeter
{
public:
    /** Sampling rate to run the microphone at, in Hertz. */
    static constexpr auto SAMPLE_RATE = 48000u;

    /** Prepares I2S Driver and microphone hardware. */
    void initMicrophone() noexcept;

    /**
     * Samples data from the microphone, potentially returning a new dB reading.
     * @return Latest calculated decibel reading, if ready
     */
    std::optional<float> readMicrophoneData() noexcept;

private:
    /**
     * During processing, microphone samples can either be ints or floats.
     * Since sample data takes up a lot of memory, a union is used to reuse
     * the buffer during processing.
     */
    union sample_t {
        /** Raw integer reading from the microphone. */
        std::int32_t i;
        /** Floating-point microphone value created during processing. */
        float f;
    };
    // Both types must be the same size to allow for buffer reuse.
    static_assert(sizeof(float) == sizeof(std::int32_t));

    /** The number of bits in a single microphone sample. */
    static constexpr auto SAMPLE_BITS = sizeof(sample_t) * 8u;
    /** The number of samples to keep in the sample buffer. */
    static constexpr auto SAMPLES_SHORT = SAMPLE_RATE / 8u;
    /** I2S peripheral config. */
    static const i2s_config_t i2s_config;
    /** I2S peripheral pin config. */
    static const i2s_pin_config_t pin_config;

    /** Buffer to store microphone samples in for reading and processing. */
    alignas(4)
    std::array<sample_t, SAMPLES_SHORT> samples;

    /** Number of samples included in Leq_sum_sqr accumulation. */
    unsigned Leq_samples = 0;
    /** Accumulation of sums of squares for decibel calculation. */
    float Leq_sum_sqr = 0;

    /** Reads enough samples from the microphone to fill the samples buffer. */
    void i2sRead() noexcept;

    /**
     * Converts a raw microphone sample into a usable number.
     * This is primarily a bit shift to discard unused bits in the left-aligned
     * 32-bit raw sample.
     * @param s Sample value to convert
     * @return Converted sample value ready for processing
     */
    static constexpr std::int32_t micConvert(std::int32_t s);
};

#endif // SPL_METER_H


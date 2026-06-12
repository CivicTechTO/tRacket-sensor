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

#if defined(USE_MIC_IM64D130A)
#include <driver/i2s_pdm.h>
#elif defined(USE_MIC_SPH0645)
#include <driver/i2s_std.h>
#endif

#include <array>
#include <cstdint>
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
    /** The number of samples to keep in the sample buffer. */
    static constexpr auto SAMPLES_SHORT = SAMPLE_RATE / 16u;

    /** Buffer to store microphone samples in for reading and processing. */
    alignas(4)
    std::array<float, SAMPLES_SHORT> samples;

    /** Number of samples included in Leq_sum_sqr accumulation. */
    unsigned Leq_samples = 0;
    /** Accumulation of sums of squares for decibel calculation. */
    float Leq_sum_sqr = 0;

    i2s_chan_handle_t i2s_handle;

    /** Reads enough samples from the microphone to fill the samples buffer. */
    size_t i2sRead() noexcept;
};

#endif // SPL_METER_H


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
#include "board.h"
#include "sos-iir-filter.h"
#include "spl-meter.h"

#include <cmath>

/** Sample size time duration to use for Leq calculation (seconds). */
static constexpr auto LEQ_PERIOD = 1.f;
/** Number of samples to use for a Leq decibel calculation. */
static constexpr auto SAMPLES_LEQ = SPLMeter::SAMPLE_RATE * LEQ_PERIOD;
/** Specifies the type of weighting to use for decibel calculation: dBA, dBC, or None/Z. */
static constexpr auto& WEIGHTING = A_weighting;
/** Specifies the microphone's equalization filter. See pre-defined filters or set to 'None'. */
static constexpr auto& MIC_EQUALIZER = SPH0645LM4H_B_RB;

/** Valid number of bits in a received I2S data sample. */
static constexpr auto MIC_BITS = 24u;
/** dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet). */
static constexpr auto MIC_SENSITIVITY = -26.f;
/** Value at which point sensitivity is specified in datasheet (dB). */
static constexpr auto MIC_REF_DB = 94.f;
/** Acoustic overload point (dB). */
static constexpr auto MIC_OVERLOAD_DB = 120.f;
/** Noise floor (dB). */
static constexpr auto MIC_NOISE_DB = 29.f;
/** Linear calibration offset to apply to calculated decibel values. */
static constexpr auto MIC_OFFSET_DB = 0.f;

/** Reference amplitude level for the microphone. */
static constexpr auto MIC_REF_AMPL = std::pow(10.f, MIC_SENSITIVITY / 20.f) * ((1 << (MIC_BITS - 1)) - 1);

const i2s_config_t SPLMeter::i2s_config = {
    mode: i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate: SAMPLE_RATE,
    bits_per_sample: i2s_bits_per_sample_t(SPLMeter::SAMPLE_BITS),
    channel_format: I2S_FORMAT,
    communication_format: i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
    dma_buf_count: 32,
    dma_buf_len: SPLMeter::SAMPLES_SHORT / 16u,
    use_apll: true,
    tx_desc_auto_clear: false,
    fixed_mclk: 0,
    mclk_multiple: I2S_MCLK_MULTIPLE_DEFAULT,
    bits_per_chan: I2S_BITS_PER_CHAN_DEFAULT,
};

const i2s_pin_config_t SPLMeter::pin_config = {
    mck_io_num: -1, // not used
    bck_io_num: I2S_SCK,
    ws_io_num: I2S_WS,
    data_out_num: -1,  // not used
    data_in_num: I2S_SD
};

constexpr std::int32_t SPLMeter::micConvert(std::int32_t s)
{
    return s >> (SAMPLE_BITS - MIC_BITS);
}

void SPLMeter::initMicrophone() noexcept
{
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);

  // Discard first block, microphone may need time to startup and settle.
  i2sRead();
}

std::optional<float> SPLMeter::readMicrophoneData() noexcept
{
  i2sRead();

  // Convert (including shifting) integer microphone values to floats,
  // using the same buffer (assumed sample size is same as size of float),
  // to save a bit of memory
  for (auto& s : samples)
      s.f = micConvert(s.i);

  // Apply equalization and calculate Z-weighted sum of squares,
  // writes filtered samples back to the same buffer.
  auto fptr = &samples[0].f;
  const auto sum_sqr_SPL = MIC_EQUALIZER.filter(fptr, fptr, samples.size());

  // Apply weighting and calucate weigthed sum of squares
  const auto sum_sqr_weighted = WEIGHTING.filter(fptr, fptr, samples.size());

  // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
  const auto short_RMS = std::sqrt(sum_sqr_SPL / samples.size());
  const auto short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * std::log10(short_RMS / MIC_REF_AMPL);

  // In case of acoustic overload or below noise floor measurement, report infinty Leq value
  if (short_SPL_dB > MIC_OVERLOAD_DB) {
    Leq_sum_sqr = MIC_OVERLOAD_DB;
  } else if (std::isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB)) {
    Leq_sum_sqr = MIC_NOISE_DB;
  }

  // Accumulate Leq sum
  Leq_sum_sqr += sum_sqr_weighted;
  Leq_samples += samples.size();

  // When we gather enough samples, calculate new Leq value
  if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD) {
    const auto Leq_RMS = std::sqrt(Leq_sum_sqr / Leq_samples);
    Leq_sum_sqr = 0;
    Leq_samples = 0;

    return MIC_OFFSET_DB + MIC_REF_DB + 20 * std::log10(Leq_RMS / MIC_REF_AMPL); // Leq dB
  } else {
    return {};
  }
}

void SPLMeter::i2sRead() noexcept
{
  // Block and wait for microphone values from I2S
  //
  // Data is moved from DMA buffers to our 'samples' buffer by the driver ISR
  // and when there is requested ammount of data, task is unblocked
  size_t bytes_read;
  i2s_read(I2S_PORT, samples.data(), samples.size() * sizeof(samples[0]), &bytes_read, portMAX_DELAY);
}


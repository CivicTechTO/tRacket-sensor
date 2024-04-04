#include "spl-meter.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <driver/i2s.h> // ESP32 core

#include "board.h"
#include "sos-iir-filter.h"

//
// Constants & Config
//
#define LEQ_PERIOD 1           // second(s)
#define WEIGHTING A_weighting  // Also avaliable: 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS "LAeq"       // customize based on above weighting used
#define DB_UNITS "dBA"         // customize based on above weighting used

// NOTE: Some microphones require at least DC-Blocker filter
#define MIC_EQUALIZER SPH0645LM4H_B_RB  // See below for defined IIR filters or set to 'None' to disable
#define MIC_OFFSET_DB 0                 // Default offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration

// Customize these values from microphone datasheet
#define MIC_SENSITIVITY -26    // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
#define MIC_REF_DB 94.0        // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_BITS 24            // valid number of bits in I2S data
#define MIC_CONVERT(s) (s >> (SAMPLE_BITS - MIC_BITS))

//
// Sampling
//
#define SAMPLES_LEQ (SAMPLE_RATE * LEQ_PERIOD)
#define DMA_BANK_SIZE (SAMPLES_SHORT / 16)
#define DMA_BANKS 32

// Calculate reference amplitude value at compile time
constexpr double MIC_REF_AMPL = pow(10, double(MIC_SENSITIVITY) / 20) * ((1 << (MIC_BITS - 1)) - 1);

void SPLMeter::initMicrophone() noexcept
{
  // Setup I2S to sample mono channel for SAMPLE_RATE * SAMPLE_BITS
  // NOTE: Recent update to Arduino_esp32 (1.0.2 -> 1.0.3)
  //       seems to have swapped ONLY_LEFT and ONLY_RIGHT channels
  const i2s_config_t i2s_config = {
    mode: i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate: SAMPLE_RATE,
    bits_per_sample: i2s_bits_per_sample_t(SAMPLE_BITS),
    channel_format: I2S_FORMAT,
    communication_format: i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
    dma_buf_count: DMA_BANKS,
    dma_buf_len: DMA_BANK_SIZE,
    use_apll: true,
    tx_desc_auto_clear: false,
    fixed_mclk: 0,
    mclk_multiple: I2S_MCLK_MULTIPLE_DEFAULT,
    bits_per_chan: I2S_BITS_PER_CHAN_DEFAULT,
  };

  // I2S pin mapping
  const i2s_pin_config_t pin_config = {
    mck_io_num: -1, // not used
    bck_io_num: I2S_SCK,
    ws_io_num: I2S_WS,
    data_out_num: -1,  // not used
    data_in_num: I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);

  // Discard first block, microphone may need time to startup and settle.
  size_t bytes_read;
  i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
}

std::optional<float> SPLMeter::readMicrophoneData() noexcept
{
  // Block and wait for microphone values from I2S
  //
  // Data is moved from DMA buffers to our 'samples' buffer by the driver ISR
  // and when there is requested ammount of data, task is unblocked
  //
  // Note: i2s_read does not care it is writing in float[] buffer, it will write
  //       integer values to the given address, as received from the hardware peripheral.
  size_t bytes_read;
  i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

  // Convert (including shifting) integer microphone values to floats,
  // using the same buffer (assumed sample size is same as size of float),
  // to save a bit of memory
  auto int_samples = reinterpret_cast<SAMPLE_T*>(samples);

  for (int i = 0; i < SAMPLES_SHORT; i++) samples[i] = MIC_CONVERT(int_samples[i]);

  // Apply equalization and calculate Z-weighted sum of squares,
  // writes filtered samples back to the same buffer.
  sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

  // Apply weighting and calucate weigthed sum of squares
  sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);

  // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
  double short_RMS = sqrt(double(sum_sqr_SPL) / SAMPLES_SHORT);
  double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);

  // In case of acoustic overload or below noise floor measurement, report infinty Leq value
  if (short_SPL_dB > MIC_OVERLOAD_DB) {
    Leq_sum_sqr = MIC_OVERLOAD_DB;
  } else if (std::isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB)) {
    Leq_sum_sqr = MIC_NOISE_DB;
  }

  // Accumulate Leq sum
  Leq_sum_sqr += sum_sqr_weighted;
  Leq_samples += SAMPLES_SHORT;

  // When we gather enough samples, calculate new Leq value
  if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD) {
    double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
    Leq_sum_sqr = 0;
    Leq_samples = 0;

    Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);
    return Leq_dB;
  } else {
    return {};
  }
}


#ifndef SPL_METER_H
#define SPL_METER_H

#include <dummy.h> // ESP32 core
#include <driver/i2s.h> // ESP32 core
#include "sos-iir-filter.h"
#include "board.h"

#include <cstdint>
#include <ctime>

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
#define MIC_OVERLOAD_DB 120.0  // dB - Acoustic overload point
#define MIC_NOISE_DB 29        // dB - Noise floor
#define MIC_BITS 24            // valid number of bits in I2S data
#define MIC_CONVERT(s) (s >> (SAMPLE_BITS - MIC_BITS))

//
// Sampling
//
#define SAMPLE_RATE 48000  // Hz, fixed to design of IIR filters
#define SAMPLE_BITS 32     // bits
#define SAMPLE_T int32_t
#define SAMPLES_SHORT (SAMPLE_RATE / 8)  // ~125ms
#define SAMPLES_LEQ (SAMPLE_RATE * LEQ_PERIOD)
#define DMA_BANK_SIZE (SAMPLES_SHORT / 16)
#define DMA_BANKS 32

// Data we push to 'samples_queue'
struct sum_queue_t {
  // Sum of squares of mic samples, after Equalizer filter
  float sum_sqr_SPL;
  // Sum of squares of weighted mic samples
  float sum_sqr_weighted;
};

// // Sampling Buffers & accumulators
// sum_queue_t q;
// uint32_t Leq_samples = 0;
// double Leq_sum_sqr = 0;
// double Leq_dB = 0;

// // Noise Level Readings
// int numberOfReadings = 0;
// float minReading = MIC_OVERLOAD_DB;
// float maxReading = MIC_NOISE_DB;
// float sumReadings = 0;

class SPLMeter
{
    // static constexpr unsigned StringSize = 64;

public:

    // Data we push to 'samples_queue'
struct sum_queue_t {
  // Sum of squares of mic samples, after Equalizer filter
  float sum_sqr_SPL;
  // Sum of squares of weighted mic samples
  float sum_sqr_weighted;
};

    // Prepares I2S Driver and mic config.
    void initMicrophone() noexcept;

    // Reads data to Microphones buffer
    void readMicrophoneData() const noexcept;

private:
    // // Calculates a CRC32 checksum of all stored settings.
    // uint32_t calculateChecksum() const noexcept;

    // // Gets the memory address/offset of the given entry.
    // constexpr unsigned addrOf(Entry entry) const noexcept {
    //     return static_cast<unsigned>(entry);
    // }
};

#endif // SPL_METER_H

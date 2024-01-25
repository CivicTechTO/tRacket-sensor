/*
   Display A-weighted sound level measured by I2S Microphone

   (c)2019 Ivan Kostoski

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
   Sketch samples audio data from I2S microphone, processes the data
   with digital IIR filters and calculates A or C weighted Equivalent
   Continuous Sound Level (Leq)

   I2S is setup to sample data at Fs=48000KHz (fixed value due to
   design of digital IIR filters). Data is read from I2S queue
   in 'sample blocks' (default 125ms block, equal to 6000 samples)
   by 'i2s_reader_task', filtered trough two IIR filters (equalizer
   and weighting), summed up and pushed into 'samples_queue' as
   sum of squares of filtered samples. The main task then pulls data
   from the queue and calculates decibel value relative to microphone
   reference amplitude, derived from datasheet sensitivity dBFS
   value, number of bits in I2S data, and the reference value for
   which the sensitivity is specified (typically 94dB, pure sine
   wave at 1KHz).

   Displays line on the small OLED screen with 'short' LAeq(125ms)
   response and numeric LAeq(1sec) dB value from the signal RMS.
*/
#include <dummy.h>
// #include <I2S.h>
// #include <esp8266/cores/esp8266/i2s.h>
//#include <i2s_reg.h>
#include <driver/i2s.h>
// #include <I2S.h>
#include "sos-iir-filter.h"

//
// Configuration
//

#define LEQ_PERIOD 1           // second(s)
#define WEIGHTING A_weighting  // Also avaliable: 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS "LAeq"       // customize based on above weighting used
#define DB_UNITS "dBA"         // customize based on above weighting used
#define USE_DISPLAY 0

// NOTE: Some microphones require at least DC-Blocker filter
#define MIC_EQUALIZER SPH0645LM4H_B_RB  // See below for defined IIR filters or set to 'None' to disable
#define MIC_OFFSET_DB 3.0103            // Default offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration

// Customize these values from microphone datasheet
#define MIC_SENSITIVITY -26    // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
#define MIC_REF_DB 94.0        // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_OVERLOAD_DB 116.0  // dB - Acoustic overload point
#define MIC_NOISE_DB 29        // dB - Noise floor
#define MIC_BITS 24            // valid number of bits in I2S data
#define MIC_CONVERT(s) (s >> (SAMPLE_BITS - MIC_BITS))
#define MIC_TIMING_SHIFT 0  // Set to one to fix MSB timing for some microphones, i.e. SPH0645LM4H-x

// Calculate reference amplitude value at compile time
constexpr double MIC_REF_AMPL = pow(10, double(MIC_SENSITIVITY) / 20) * ((1 << (MIC_BITS - 1)) - 1);

//
// I2S pins - Can be routed to almost any (unused) ESP32 pin.
//            SD can be any pin, inlcuding input only pins (36-39).
//            SCK (i.e. BCLK) and WS (i.e. L/R CLK) must be output capable pins
//
// Below ones are just example for my board layout, put here the pins you will use
//
#define I2S_WS 18
#define I2S_SCK 23
#define I2S_SD 19

// I2S peripheral to use (0 or 1)
#define I2S_PORT I2S_NUM_0


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
// QueueHandle_t samples_queue;

// Static buffer for block of samples
float samples[SAMPLES_SHORT] __attribute__((aligned(4)));

//
// I2S Microphone sampling setup
//
void mic_i2s_init() {
  // Setup I2S to sample mono channel for SAMPLE_RATE * SAMPLE_BITS
  // NOTE: Recent update to Arduino_esp32 (1.0.2 -> 1.0.3)
  //       seems to have swapped ONLY_LEFT and ONLY_RIGHT channels
  const i2s_config_t i2s_config = {
    mode: i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate: SAMPLE_RATE,
    bits_per_sample: i2s_bits_per_sample_t(SAMPLE_BITS),
    channel_format: I2S_CHANNEL_FMT_ONLY_RIGHT,
    communication_format: i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
    dma_buf_count: DMA_BANKS,
    dma_buf_len: DMA_BANK_SIZE,
    use_apll: true,
    tx_desc_auto_clear: false,
    fixed_mclk: 0
  };
  // I2S pin mapping
  const i2s_pin_config_t pin_config = {
    bck_io_num: I2S_SCK,
    ws_io_num: I2S_WS,
    data_out_num: -1,  // not used
    data_in_num: I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

#if (MIC_TIMING_SHIFT > 0)
  // Undocumented (?!) manipulation of I2S peripheral registers
  // to fix MSB timing issues with some I2S microphones
  REG_SET_BIT(I2S_TIMING_REG(I2S_PORT), BIT(9));
  REG_SET_BIT(I2S_CONF_REG(I2S_PORT), I2S_RX_MSB_SHIFT);
#endif

  i2s_set_pin(I2S_PORT, &pin_config);

  //FIXME: There is a known issue with esp-idf and sampling rates, see:
  //       https://github.com/espressif/esp-idf/issues/2634
  //       In the meantime, the below line seems to set sampling rate at ~47999.992Hz
  //       fifs_req=24576000, sdm0=149, sdm1=212, sdm2=5, odir=2 -> fifs_reached=24575996
  //NOTE:  This seems to be fixed in ESP32 Arduino 1.0.4, esp-idf 3.2
  //       Should be safe to remove...
  //#include <soc/rtc.h>
  //rtc_clk_apll_enable(1, 149, 212, 5, 2);
}

void printArray(float arr[], unsigned long length) {
  Serial.print(length);
  Serial.print(" {");
  for (int i = 0; i < length; i++) {
    Serial.print(arr[i]);
    if (i < length - 1) Serial.print(", ");
  }
  Serial.println("}");
}

sum_queue_t q;
uint32_t Leq_samples = 0;
double Leq_sum_sqr = 0;
double Leq_dB = 0;
size_t bytes_read = 0;

//
// Setup and main loop
//
// Note: Use doubles, not floats, here unless you want to pin
//       the task to whichever core it happens to run on at the moment
//
void setup() {

  // If needed, now you can actually lower the CPU frquency,
  // i.e. if you want to (slightly) reduce ESP32 power consumption
  setCpuFrequencyMhz(80);  // It should run as low as 80MHz

  Serial.begin(115200);
  delay(1000);  // Safety
  initMicrophone();
}

void loop() {
  readMicrophoneData();
}

void initMicrophone() {
  mic_i2s_init();
  // Discard first block, microphone may have startup time (i.e. INMP441 up to 83ms)
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);
  // Equivalent to: i2s_read(0, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);
}

void readMicrophoneData() {
  // Block and wait for microphone values from I2S
  //
  // Data is moved from DMA buffers to our 'samples' buffer by the driver ISR
  // and when there is requested ammount of data, task is unblocked
  //
  // Note: i2s_read does not care it is writing in float[] buffer, it will write
  //       integer values to the given address, as received from the hardware peripheral.
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(SAMPLE_T), &bytes_read, portMAX_DELAY);

  // Convert (including shifting) integer microphone values to floats,
  // using the same buffer (assumed sample size is same as size of float),
  // to save a bit of memory
  SAMPLE_T* int_samples = (SAMPLE_T*)&samples;

  // float dummy_int_samples[] = {-981376.00, -982656.00, -982272.00, -982400.00, -983936.00, -983680.00, -984064.00, -983808.00, -985344.00, -985088.00, -986752.00, -986496.00, -988288.00, -987904.00, -988544.00, -988928.00};
  for (int i = 0; i < SAMPLES_SHORT; i++) samples[i] = MIC_CONVERT(int_samples[i]);
  // float dummy_converted_samples[] = {53248.00, 51840.00, 50816.00, 49408.00, 48896.00, 49408.00, 48256.00, 49280.00, 47616.00, 48128.00, 47616.00, 47360.00, 46464.00, 45696.00, 46464.00, 46336.00, 46592.00, 46080.00};

  // sum_queue_t q;
  // Apply equalization and calculate Z-weighted sum of squares,
  // writes filtered samples back to the same buffer.
  q.sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);
  // Serial.println(q.sum_sqr_SPL);
  // float dummy_sum_sqr_SPL = 224.16;

  // Apply weighting and calucate weigthed sum of squares
  q.sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);
  // float dummy_sum_sqr_weighted = 207.60;

  // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
  double short_RMS = sqrt(double(q.sum_sqr_SPL) / SAMPLES_SHORT);
  // double dummy_short_RMS = 5170750.81;
  double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);
  // double dummy_short_SPL_dB = 118.78;

  // In case of acoustic overload or below noise floor measurement, report infinty Leq value
  if (short_SPL_dB > MIC_OVERLOAD_DB) {
    Leq_sum_sqr = MIC_OVERLOAD_DB;
  } else if (isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB)) {
    Leq_sum_sqr = MIC_NOISE_DB;
  }

  // Accumulate Leq sum
  Leq_sum_sqr += q.sum_sqr_weighted;
  Leq_samples += SAMPLES_SHORT;

  // When we gather enough samples, calculate new Leq value
  if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD) {
    double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
    // double dummy_Leq_RMS = 1611509.20;
    Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);
    // double dummy_Leq_dB = 109.26;
    Leq_sum_sqr = 0;
    Leq_samples = 0;

    Serial.print(0);  // To freeze the lower limit
    Serial.print(" ");
    Serial.print(130);  // To freeze the upper limit
    Serial.print(" ");
    // Serial.printf("%.1f\n", Leq_dB);
    Serial.println(Leq_dB);
  }
}

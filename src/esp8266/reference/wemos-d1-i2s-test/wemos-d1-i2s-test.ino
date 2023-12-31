/**
 ******************************************************************************
 * @file    listener.ino
 * @author  Joe Todd
 * @version
 * @date    November 2017
 * @brief   I2S interface for ESP8266 and SPH0645 MEMS microphone.
 *
  ******************************************************************************/
// #include <ESP8266WiFi.h>
#include "sos-iir-filter.h"

extern "C" {
#include "user_interface.h"
#include "i2s_reg.h"
#include "slc_register.h"
#include "esp8266_peri.h"
  void rom_i2c_writeReg_Mask(int, int, int, int, int, int);
}

#define I2S_CLK_FREQ 160000000  // Hz
#define I2S_24BIT 3             // I2S 24 bit half data
#define I2S_LEFT 2              // I2S RX Left channel

#define I2SI_DATA 12  // I2S data on GPIO12
#define I2SI_BCK 13   // I2S clk on GPIO13
#define I2SI_WS 14    // I2S select on GPIO14

#define SLC_BUF_CNT 8   // Number of buffers in the I2S circular buffer
#define SLC_BUF_LEN 64  // Length of one buffer, in 32-bit words.

/**
 * Convert I2S data.
 * Data is 18 bit signed, MSBit first, two's complement.
 * Note: We can only send 31 cycles from ESP8266 so we only
 * shift by 13 instead of 14.
 * The 240200 is a magic calibration number I haven't figured
 * out yet.
 */
#define convert(sample) (((int32_t)(sample) >> 13) - 240200)

typedef struct {
  uint32_t blocksize : 12;
  uint32_t datalen : 12;
  uint32_t unused : 5;
  uint32_t sub_sof : 1;
  uint32_t eof : 1;
  volatile uint32_t owner : 1;

  uint32_t *buf_ptr;
  uint32_t *next_link_ptr;
} sdio_queue_t;

static sdio_queue_t i2s_slc_items[SLC_BUF_CNT];  // I2S DMA buffer descriptors
static uint32_t *i2s_slc_buf_pntr[SLC_BUF_CNT];  // Pointer to the I2S DMA buffer data
static volatile uint32_t rx_buf_cnt = 0;
static volatile uint32_t rx_buf_idx = 0;
static volatile bool rx_buf_flag = false;

void i2s_init();
void slc_init();
void i2s_set_rate(uint32_t rate);
void slc_isr(void *para);

float readoutMaxScale = 30000;

/* Main -----------------------------------------------------------------------*/
void setup() {
  Serial.println("START");
  rx_buf_cnt = 0;

  pinMode(I2SI_WS, OUTPUT);
  pinMode(I2SI_BCK, OUTPUT);
  pinMode(I2SI_DATA, INPUT);

  // WiFi.forceSleepBegin();
  // delay(500);

  Serial.begin(115200);

  slc_init();
  i2s_init();
}

void loop() {
  int32_t value;
  char withScale[256];

  if (rx_buf_flag) {
    for (int x = 0; x < SLC_BUF_LEN; x++) {
      if (i2s_slc_buf_pntr[rx_buf_idx][x] > 0) {

        // Serial.println(i2s_slc_buf_pntr[rx_buf_idx][x]);
        // 1968000000 at ambient room volume - Gets lower with SPL
        // ESP32: ~-250000000 at ambient room volume - increases with SPL
        value = convert(i2s_slc_buf_pntr[rx_buf_idx][x]);

        // value = i2s_slc_buf_pntr[rx_buf_idx][x];
        // sprintf(withScale, "-1 %f 1", (float)value / 4096.0f);
        // Serial.println(withScale);

        Serial.print(readoutMaxScale * -1);  // To freeze the lower limit
        Serial.print(" ");
        Serial.print(readoutMaxScale);  // To freeze the upper limit
        Serial.print(" ");
        Serial.println(value);
      }
    }
    rx_buf_flag = false;
  }
}

/* Function definitions -------------------------------------------------------*/

/**
 * Initialise I2S as a RX master.
 * This function initializes the I2S (Inter-IC Sound) interface on the ESP8266.
 */
void i2s_init() {
  // Config RX pin function
  // Configuring Pin Functions: The function configures the pin functions for the I2S interface by selecting the appropriate functionality for GPIO pins using PIN_FUNC_SELECT.
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_I2SI_DATA);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_I2SI_BCK);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_I2SI_WS);

  // Clock and Reset Configuration: It enables a 160MHz clock for the I2S interface, resets the I2S interface, and configures the DMA (Direct Memory Access) controller for I2S.
  // Enable a 160MHz clock
  I2S_CLK_ENABLE();
  // Reset I2S
  I2SC &= ~(I2SRST);
  I2SC |= I2SRST;
  I2SC &= ~(I2SRST);

  // Reset DMA
  // DMA Configuration: The function sets up the DMA for I2S, enabling it and specifying the data format (24-bit) for receiving data.
  I2SFC &= ~(I2SDE | (I2SRXFMM << I2SRXFM));
  // Enable DMA
  I2SFC |= I2SDE | (I2S_24BIT << I2SRXFM);

  // Channel Configuration: It configures the I2S channel for single-channel reception (left channel).
  // Set RX single channel (left)
  I2SCC &= ~((I2STXCMM << I2STXCM) | (I2SRXCMM << I2SRXCM));
  I2SCC |= (I2S_LEFT << I2SRXCM);

  // Sample Rate Configuration: Calls the i2s_set_rate function to set the sample rate. The default value used here is 16667 Hz.
  i2s_set_rate(16667);

  // Data Length Configuration: Sets the length of data to be received by specifying I2SRXEN to SLC_BUF_LEN (defined as 64).
  // Set RX data to be received
  I2SRXEN = SLC_BUF_LEN;

  // Bits Mode Configuration: Specifies the number of bits per sample in the I2S data. It sets the bits mode to 15 bits.

  // Bits mode
  I2SC |= (15 << I2SBM);

  // Start Receiver: Initiates the I2S receiver operation by setting the I2SRXS bit.
  // Start receiver
  I2SC |= I2SRXS;
}

/**
 * Set I2S clock.
 * This function sets the I2S clock rate based on the desired sample rate.
 * I2S bits mode only has space for 15 extra bits,
 * 31 in total.
 */
void i2s_set_rate(uint32_t rate) {
  // Clock Division Calculation: Calculates the values for I2S clock division based on the desired sample rate.
  uint32_t i2s_clock_div = (I2S_CLK_FREQ / (rate * 31 * 2)) & I2SCDM;

  // Configuration Settings: Configures the I2S control register (I2SC) with the calculated clock division values.
  uint32_t i2s_bck_div = (I2S_CLK_FREQ / (rate * i2s_clock_div * 31 * 2)) & I2SBDM;

#ifdef DEBUG
  Serial.printf("Rate %u Div %u Bck %u Freq %u\n",
                rate, i2s_clock_div, i2s_bck_div, I2S_CLK_FREQ / (i2s_clock_div * i2s_bck_div * 31 * 2));
#endif

  // RX master mode, RX MSB shift, right first, msb right
  I2SC &= ~(I2STSM | I2SRSM | (I2SBMM << I2SBM) | (I2SBDM << I2SBD) | (I2SCDM << I2SCD));
  I2SC |= I2SRF | I2SMR | I2SRMS | (i2s_bck_div << I2SBD) | (i2s_clock_div << I2SCD);
}

/**
 * Initialize the SLC module for DMA operation.
 * Counter intuitively, we use the TXLINK here to
 * receive data.
 * This function initializes the Serial Link Controller (SLC) module for DMA (Direct Memory Access) operation.
 */
void slc_init() {
  // Memory Allocation and Initialization: Allocates memory for buffer pointers and initializes buffer data.
  for (int x = 0; x < SLC_BUF_CNT; x++) {
    i2s_slc_buf_pntr[x] = (uint32_t *)malloc(SLC_BUF_LEN * 4);
    for (int y = 0; y < SLC_BUF_LEN; y++) i2s_slc_buf_pntr[x][y] = 0;

    i2s_slc_items[x].unused = 0;
    i2s_slc_items[x].owner = 1;
    i2s_slc_items[x].eof = 0;
    i2s_slc_items[x].sub_sof = 0;
    i2s_slc_items[x].datalen = SLC_BUF_LEN * 4;
    i2s_slc_items[x].blocksize = SLC_BUF_LEN * 4;
    i2s_slc_items[x].buf_ptr = (uint32_t *)&i2s_slc_buf_pntr[x][0];
    i2s_slc_items[x].next_link_ptr = (uint32_t *)((x < (SLC_BUF_CNT - 1)) ? (&i2s_slc_items[x + 1]) : (&i2s_slc_items[0]));
  }

  // Reset DMA
  // DMA Reset: Disables SLC interrupts, resets DMA related registers, and clears interrupt status.
  ETS_SLC_INTR_DISABLE();
  SLCC0 |= SLCRXLR | SLCTXLR;
  SLCC0 &= ~(SLCRXLR | SLCTXLR);
  SLCIC = 0xFFFFFFFF;

  // Configure DMA
  // DMA Configuration: Configures the DMA mode and enables specific DMA features.
  SLCC0 &= ~(SLCMM << SLCM);     // Clear DMA MODE
  SLCC0 |= (1 << SLCM);          // Set DMA MODE to 1
  SLCRXDC |= SLCBINR | SLCBTNR;  // Enable INFOR_NO_REPLACE and TOKEN_NO_REPLACE

  // Feed DMA the 1st buffer desc addr
  SLCTXL &= ~(SLCTXLAM << SLCTXLA);
  SLCTXL |= (uint32_t)&i2s_slc_items[0] << SLCTXLA;

  // Interrupt Configuration: Attaches the SLC interrupt service routine (slc_isr) and enables the End of Frame (EOF) interrupt.
  ETS_SLC_INTR_ATTACH(slc_isr, NULL);

  // Enable EOF interrupt
  SLCIE = SLCITXEOF;
  ETS_SLC_INTR_ENABLE();

  // Start transmission
  // Transmission Start: Initiates DMA transmission by setting the SLCTXLS bit.
  SLCTXL |= SLCTXLS;
}

/**
 * Triggered when SLC has finished writing
 * to one of the buffers.
 * This function is the Interrupt Service Routine (ISR) for the SLC module. It is triggered when the SLC has finished writing to one of the buffers.
 */
void ICACHE_RAM_ATTR
slc_isr(void *para) {
  // Interrupt Handling: Reads and clears the interrupt status register.
  uint32_t status;

  status = SLCIS;
  SLCIC = 0xFFFFFFFF;

  if (status == 0) {
    return;
  }

  // EOF Check: Checks if the interrupt is an End of Frame (EOF) interrupt.
  if (status & SLCITXEOF) {
    // We have received a frame
    ETS_SLC_INTR_DISABLE();
    sdio_queue_t *finished = (sdio_queue_t *)SLCTXEDA;

    // Buffer Handling: Updates buffer-related information, flags, and enables/dis
    finished->eof = 0;
    finished->owner = 1;
    finished->datalen = 0;

    for (int i = 0; i < SLC_BUF_CNT; i++) {
      if (finished == &i2s_slc_items[i]) {
        rx_buf_idx = i;
      }
    }
    rx_buf_cnt++;
    rx_buf_flag = true;
    ETS_SLC_INTR_ENABLE();
  }
}


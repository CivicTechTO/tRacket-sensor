#ifndef BOARD_H
#define BOARD_H

#if defined(BOARD_PCB_REV1)

// Pin definitions
#define PIN_LED1    (0)
#define PIN_LED2    (3)
#define PIN_BUTTON  (1)
#define I2S_WS      (4)
#define I2S_SCK     (5)
#define I2S_SD      (6)

// ESP32C3 only has one I2S peripheral
#define I2S_PORT    I2S_NUM_0
#define I2S_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT

// Use USB for serial log
#define SERIAL      USBSerial

#elif defined(BOARD_ESP32_BREADBOARD)

// Pin definitions
#define PIN_LED1    (36) // random choice
#define PIN_LED2    (39) // random choice
#define PIN_BUTTON  (5)
#define I2S_WS      (18)
#define I2S_SCK     (23)
#define I2S_SD      (19)

// ESP32 has two I2S peripherals
#define I2S_PORT    I2S_NUM_0
#define I2S_FORMAT  I2S_CHANNEL_FMT_ONLY_RIGHT

#define SERIAL      Serial

#else
#error "Please select a board from the list in board.h!"
#endif

#endif // BOARD_H


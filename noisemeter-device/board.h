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
#ifndef BOARD_H
#define BOARD_H

#include "config.h"

#undef SERIAL

#if defined(BOARD_ESP32_PCB)

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

#if defined(BUILD_PLATFORMIO)
#include <HWCDC.h>
extern HWCDC USBSerial;
#endif

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


/// @file
/// @brief Pre-defined hardware configurations
///
/// Each supported board must have a defined section here to specify the
/// hardware pins and peripherals being used. Selecting a board for
/// compilation is done through PlatformIO.
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

#undef SERIAL

#if defined(BOARD_REV4)

/** Pin number for the board's LED. */
#define PIN_LED1    GPIO_NUM_17
/** Pin number for the board's factory reset button. */
#define PIN_BUTTON  GPIO_NUM_15
/** Pin number for the microphone's clock pin. */
#define I2S_SCK     GPIO_NUM_5
/** Pin number for the microphone's data out pin. */
#define I2S_SD      GPIO_NUM_6

/** I2S peripheral instance to be used. */
#define I2S_PORT    I2S_NUM_0
/** Channel format for the incoming microphone data.
 * There is only one microphone, so this must be either only left or right. */
#define I2S_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT

/** Serial instance to use for logging output. */
#define SERIAL      USBSerial

#define USE_MIC_IM64D130A

#include <HWCDC.h>
extern HWCDC USBSerial;

#elif defined(BOARD_REV123)

#define PIN_LED1    GPIO_NUM_0
#define PIN_BUTTON  GPIO_NUM_1
#define I2S_WS      GPIO_NUM_4
#define I2S_SCK     GPIO_NUM_5
#define I2S_SD      GPIO_NUM_6
#define I2S_PORT    I2S_NUM_0
#define I2S_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT
#define SERIAL      USBSerial

#define USE_MIC_SPH0645

#include <HWCDC.h>
extern HWCDC USBSerial;

#elif defined(BOARD_BREADBOARD)

#define PIN_LED1    GPIO_NUM_36 // random choice
#define PIN_LED2    GPIO_NUM_39 // random choice
#define PIN_BUTTON  GPIO_NUM_5
#define I2S_WS      GPIO_NUM_18
#define I2S_SCK     GPIO_NUM_23
#define I2S_SD      GPIO_NUM_19
#define I2S_PORT    I2S_NUM_0
#define I2S_FORMAT  I2S_CHANNEL_FMT_ONLY_RIGHT
#define SERIAL      Serial

#define USE_MIC_SPH0645

#else
#error "Please select a board from the list in board.h!"
#endif

#endif // BOARD_H


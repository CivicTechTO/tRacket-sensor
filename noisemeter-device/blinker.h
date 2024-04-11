/// @file
/// @brief Utility to indicate status with the board's LED.
/* noisemeter-device - Firmware for CivicTechTO's Noisemeter Device
 * Copyright (C) 2024  Clyne Sullivan
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
#ifndef BLINKER_H
#define BLINKER_H

#include "board.h"

#include <Ticker.h>

/**
 * Automates the blinking of the board's LED indicator.
 */
class Blinker : public Ticker
{
public:
  /**
   * Starts a timer to toggle the LED.
   * @param ms Milliseconds to delay between toggles.
   */
  Blinker(unsigned ms):
    Ticker()
  {
    attach_ms(ms, callback, &state);
  }

  /**
   * Detaches the LED toggle routine.
   */
  ~Blinker() {
    // turn off the LED
    digitalWrite(PIN_LED1, HIGH);
  }

private:
  /** State of the LED */
  int state = HIGH;

  /**
   * Ticker callback to toggle the LED every time interval.
   * @param st Pointer to the active Blinker instance's state
   */
  static void callback(int *st) {
    digitalWrite(PIN_LED1, *st);
    *st ^= HIGH;
  }
};

#endif // BLINKER_H


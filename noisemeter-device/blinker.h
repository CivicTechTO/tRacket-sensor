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

class Blinker : public Ticker
{
public:
  Blinker(unsigned ms):
    Ticker()
  {
    attach_ms(ms, callback, &state);
  }

  ~Blinker() {
    // turn off the LED
    digitalWrite(PIN_LED1, HIGH);
  }

private:
  int state = HIGH;

  static void callback(int *st) {
    digitalWrite(PIN_LED1, *st);
    *st ^= HIGH;
  }
};

#endif // BLINKER_H


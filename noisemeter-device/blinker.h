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


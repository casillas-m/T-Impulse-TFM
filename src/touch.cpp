#include "config.h"
#include "STM32LowPower.h"

int TouchCallback(void) {
  int TouchMillis = millis();
  if (digitalRead(TOUCH_PAD_PIN)) {
    while (digitalRead(TOUCH_PAD_PIN)) {
      if ((int)millis() - TouchMillis > 3000)
        break;
    }
    return ((int)millis() - TouchMillis);
  }
  return 0;
}
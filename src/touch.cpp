#include "config.h"
#include "STM32LowPower.h"

/**
 * @brief Checks the touchpad input and calculates the press duration.
 *
 * This function checks if the touchpad is pressed and calculates the duration of the press.
 * If the touchpad is pressed for more than 3 seconds, it breaks the loop and returns the
 * total press duration in milliseconds. If the touchpad is not pressed, it returns 0.
 *
 * @return The duration of the touchpad press in milliseconds, or 0 if not pressed.
 */
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
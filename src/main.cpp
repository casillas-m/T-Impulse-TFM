#include <Wire.h> // Only needed for Arduino 1.6.5 and earlier
#include <SPI.h>
#include "loramac.h"
#include "config.h"
#include "oled.h"
#include "gps.h"
#include "Bat.h"
#include "energy_mgmt.h"
#include "touch.h"

void setup()
{
    BoardInit();
    delay(500);
    Serial.println("LoRaWan Demo");
    setupLMIC();
}

void loop()
{
    int touch_press_time = TouchCallback();
    if (touch_press_time > 3000) // Long press for 3 seconds to enter sleep mode
    {
        Board_Sleep();
    }
    loopLMIC();
    bat_loop();
    gps_loop();
}

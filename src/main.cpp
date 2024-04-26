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
    else if (touch_press_time > 200)
    {
        //Toggle TX fast mode
        bool tx_fast_flag = getTXFast();
        if (tx_fast_flag)
        {
            setTXFast(false);
            if (u8g2)
            {
                u8g2->clearBuffer();
                u8g2->setFont(u8g2_font_IPAandRUSLCD_tr);
                u8g2->drawStr(0, 12, "TX Fast OFF");
                u8g2->sendBuffer();
            }
        }
        else
        {
            setTXFast(true);
            if (u8g2)
            {
                u8g2->clearBuffer();
                u8g2->setFont(u8g2_font_IPAandRUSLCD_tr);
                u8g2->drawStr(0, 12, "TX Fast ON");
                u8g2->sendBuffer();
            }
        }
    }

    loopLMIC();
    bat_loop();
    gps_loop();
}

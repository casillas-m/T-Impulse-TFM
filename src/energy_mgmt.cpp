#include "energy_mgmt.h"
#include "gps.h"
#include "oled.h"
#include "config.h"
#include "Bat.h"
#include "loramac.h"
#include <SPI.h>
#include <Wire.h>
#include "STM32LowPower.h"

void LoraWanInit(void)
{
    SPI.setMISO(LORA_MISO);
    SPI.setMOSI(LORA_MOSI);
    SPI.setSCLK(LORA_SCK);
    SPI.begin();

    pinMode(RADIO_ANT_SWITCH_RXTX, OUTPUT);
    pinMode(GPS_EN, OUTPUT);

    digitalWrite(RADIO_ANT_SWITCH_RXTX, HIGH);
    digitalWrite(GPS_EN, HIGH);
}

void BoardInit(void)
{
    pinMode(PWR_1_8V_PIN, OUTPUT);
    digitalWrite(PWR_1_8V_PIN, HIGH);
    pinMode(PWR_GPS_PIN, OUTPUT);
    digitalWrite(PWR_GPS_PIN, HIGH);
    Serial.begin(115200);

    Wire.setSCL(IICSCL);
    Wire.setSDA(IICSDA);
    Wire.begin();

    gps_init();
    oled_init();
    bat_init();
    pinMode(BAT_VOLT_PIN, INPUT_ANALOG);

    LoraWanInit();

    pinMode(TTP223_VDD_PIN, OUTPUT);
    pinMode(TOUCH_PAD_PIN, INPUT);
    digitalWrite(TTP223_VDD_PIN, HIGH);

    LowPower.attachInterruptWakeup(TOUCH_PAD_PIN, NULL, RISING, DEEP_SLEEP_MODE);
}

void Board_Sleep(void) {
  gps_sleep();
  oled_sleep();
  Serial.println(F("MCU Sleep"));
  pinMode(PWR_1_8V_PIN, OUTPUT);
  digitalWrite(PWR_1_8V_PIN, LOW);
  Serial.end();
  SPI.end();
  Wire.end();
  bat_sleep();
  LowPower.deepSleep();
  //After wakeup
  BoardInit();
  delay(500);
  Serial.println(F("Wakeup"));  
  //setupLMIC();
}
#include "energy_mgmt.h"
#include "gps.h"
#include "oled.h"
#include "config.h"
#include "Bat.h"
#include "loramac.h"
#include <SPI.h>
#include <Wire.h>
#include "STM32LowPower.h"

/**
 * @brief Initializes the LoRaWAN communication interface.
 *
 * This function configures the SPI settings for the LoRa module by setting
 * the appropriate pins for MISO, MOSI, and SCLK. It also sets up the pin modes
 * and initial states for the radio antenna switch and GPS enable pin.
 */
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

/**
 * @brief Initializes the board and its peripherals.
 *
 * This function sets up power pins, serial communication, I2C interface,
 * GPS, OLED, battery voltage measurement, and touchpad. It ensures that
 * all necessary peripherals are configured and initialized properly.
 * It also configures the wakeup interrupt for deep sleep mode.
 */
void BoardInit(void)
{
    pinMode(PWR_1_8V_PIN, OUTPUT);
    digitalWrite(PWR_1_8V_PIN, HIGH);
    pinMode(PWR_GPS_PIN, OUTPUT);
    digitalWrite(PWR_GPS_PIN, HIGH);
    Serial.begin(115200);

    //I2C for OLED
    Wire.setSCL(IICSCL);
    Wire.setSDA(IICSDA);
    Wire.begin();

    if (!getDEV_INTERIOR()) gps_init(); //Init only if Dev not in interiors
    oled_init();
    bat_init();
    pinMode(BAT_VOLT_PIN, INPUT_ANALOG);

    LoraWanInit();

    pinMode(TTP223_VDD_PIN, OUTPUT);
    pinMode(TOUCH_PAD_PIN, INPUT);
    digitalWrite(TTP223_VDD_PIN, HIGH);

    LowPower.attachInterruptWakeup(TOUCH_PAD_PIN, NULL, RISING, DEEP_SLEEP_MODE);
}

/**
 * @brief Puts the board into sleep mode to save power.
 *
 * This function puts various peripherals like GPS and OLED into sleep mode,
 * stops serial communication, SPI, and I2C interfaces, and stops battery
 * voltage measurement. Finally, it puts the MCU into deep sleep mode and
 * reinitializes the board after waking up.
 */
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
}
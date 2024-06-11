#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include "config.h"
#include <CayenneLPP.h>
#include "STM32LowPower.h"

#include "oled.h"
#include "gps.h"
#include "Bat.h"

#include "../.secrets/secrets.h"

CayenneLPP lpp(200);

// Chose LSB mode on the console and then copy it here.
static const u1_t PROGMEM APPEUI[8] = APPEUI_SECRET;
// LSB mode
static const u1_t PROGMEM DEVEUI[8] = DEVEUI_SECRET;
// MSB mode
static const u1_t PROGMEM APPKEY[16] = APPKEY_SECRET;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = LORA_NSS,
    .rxtx = RADIO_ANT_SWITCH_RXTX,
    .rst = LORA_RST,
    .dio = {LORA_DIO0, LORA_DIO1_PIN, LORA_DIO2_PIN},
};

static osjob_t sendjob;
static int spreadFactor = DR_SF7;
static int joinStatus = EV_JOINING;
static const unsigned TX_INTERVAL = 15;
static const unsigned TX_INTERVAL_FAST = 5;
static const unsigned TX_RETRY_INTERVAL = 15;
static const unsigned JOIN_RETRY_INTERVAL = 15;
static bool tx_fast_flag = false;
static bool dev_interior = false;
static const int TX_CHANNEL_QTY = 4;
static int latest_tx_channels[4] = {-1, -1, -1, -1};
static int tx_channel_pos = 0;

void setupLMIC(void);

/**
 * @brief Sets the transmission mode to fast or normal.
 *
 * This function updates the transmission mode flag to enable or disable fast
 * transmission mode.
 *
 * @param mode Boolean indicating whether to enable (true) or disable (false) fast transmission mode.
 */
void setTXFast(bool mode)
{
    tx_fast_flag = mode;
}

/**
 * @brief Retrieves the current transmission mode.
 *
 * This function returns the current state of the fast transmission mode flag.
 *
 * @return Boolean indicating the current state of the fast transmission mode flag.
 */
bool getTXFast()
{
    return tx_fast_flag;
}

/**
 * @brief Retrieves the device interior status.
 *
 * This function returns whether the device is currently in an interior environment.
 *
 * @return Boolean indicating if the device is in an interior environment.
 */
bool getDEV_INTERIOR()
{
    return dev_interior;
}

/**
 * @brief Collects and prints various sensor data.
 *
 * This function gathers data from GPS, interior status, recent transmission channels,
 * and battery level, then formats and adds it to a CayenneLPP payload for transmission.
 */
void printVariables()
{
    lpp.reset();

    if (gps != nullptr && gps->location.isUpdated() && gps->altitude.isUpdated() && gps->satellites.isUpdated())
    {
        double gps_lat = gps->location.lat();
        double gps_lng = gps->location.lng();
        double gps_alt = gps->altitude.meters();
        lpp.addGPS(3, (float)gps_lat, (float)gps_lng, (float)gps_alt);
    }

    if (dev_interior)
        lpp.addDigitalInput(4, 1); // Interior
    else
        lpp.addDigitalInput(4, 0); // Exterior

    // Write channels used in the last 4 tx
    // Bits position represent the channel, B0->Ch0... 1s means channel used
    Serial.printf("Channel next TX: %d\n", LMIC.txChnl);
    latest_tx_channels[tx_channel_pos] = LMIC.txChnl; // Save tx ch
    tx_channel_pos = (tx_channel_pos + 1) % TX_CHANNEL_QTY;
    int tx_channels_used = 0;
    Serial.printf("latest_tx_channels = ");
    for (int i = 0; i < TX_CHANNEL_QTY; i++)
    {
        Serial.printf("%d,", latest_tx_channels[i]);
        // At begining array is filled with -1. If to ignore them.
        if (latest_tx_channels[i] > -1)
        {
            tx_channels_used |= 1 << latest_tx_channels[i];
        }
    }
    Serial.printf("\n");
    lpp.addDigitalInput(5, tx_channels_used);
    Serial.printf("tx_channels_used: %d\n", tx_channels_used);

    float batt_lvl = float((Volt * 3.3 * 2) / 4096);
    Serial.printf("BatteryVol : %f\n", batt_lvl);
    lpp.addAnalogInput(8, batt_lvl);
}

void os_getArtEui(u1_t *buf)
{
    memcpy_P(buf, APPEUI, 8);
}

void os_getDevEui(u1_t *buf)
{
    memcpy_P(buf, DEVEUI, 8);
}

void os_getDevKey(u1_t *buf)
{
    memcpy_P(buf, APPKEY, 16);
}

/**
 * @brief Sends data over LoRaWAN.
 *
 * This function checks the join status and current operation mode, prepares the data
 * for transmission, and schedules the next transmission. It also updates the display
 * with the current transmission status and battery level.
 *
 * @param j Pointer to the job structure.
 */
void do_send(osjob_t *j)
{
    if (joinStatus == EV_JOINING)
    {
        Serial.println(F("Not joined yet"));
        // Check if there is not a current TX/RX job running
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_RETRY_INTERVAL), do_send);
    }
    else if (LMIC.opmode & OP_TXRXPEND)
    {
        Serial.println(F("OP_TXRXPEND, not sending"));
    }
    else
    {
        // Sending process
        Serial.println(F("OP_TXRXPEND,sending ..."));
        u8g2->sleepOff();

        printVariables();
        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);

        // Schedule again the send process, this task is supposed to be override by a task schedule at the end of the TX event
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_RETRY_INTERVAL), do_send);

        if (u8g2)
        {
            // Draw OLED screen
            char buf[256];
            u8g2->clearBuffer();
            float batt_lvl = float((Volt * 3.3 * 2) / 4096);
            snprintf(buf, sizeof(buf), "Batt: %.2f", batt_lvl);
            u8g2->drawStr(0, 7, buf);
            if (gps != nullptr)
            {
                if (!dev_interior)
                {
                    snprintf(buf, sizeof(buf), "#GPS: %d", gps->satellites.value());
                }
                else
                {
                    snprintf(buf, sizeof(buf), "#GPS: Sleep");
                }
            }
            else
            {
                snprintf(buf, sizeof(buf), "#GPS: N/A");
            }
            u8g2->drawStr(0, 17, buf);
            snprintf(buf, sizeof(buf), "Sending");
            u8g2->drawStr(0, 27, buf);
            u8g2->sendBuffer();
        }
    }
}

/**
 * @brief Handles events from the LoRaWAN stack.
 *
 * This function processes various events such as join complete, transmission complete,
 * and data received, and takes appropriate actions like updating the display, scheduling
 * the next transmission, and putting the device into sleep mode.
 *
 * @param ev The event type.
 */
void onEvent(ev_t ev)
{
    uint32_t sleep_ms = (tx_fast_flag ? TX_INTERVAL_FAST : TX_INTERVAL) * 1000;
    switch (ev)
    {
    case EV_TXCOMPLETE:
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
        if (u8g2)
        {
            char buf[256];
            u8g2->setDrawColor(2);
            snprintf(buf, sizeof(buf), "Sending");
            u8g2->drawStr(0, 27, buf);
            u8g2->setDrawColor(1);
            snprintf(buf, sizeof(buf), "Sended");
            u8g2->drawStr(0, 27, buf);
            u8g2->sendBuffer();
            delay(200);
            u8g2->sleepOn();
        }
        if (LMIC.txrxFlags & TXRX_ACK)
        {
            Serial.println(F("Received ack"));
        }

        if (LMIC.dataLen)
        {
            // data received in rx slot after tx
            // Serial.print(F("Data Received: "));
            int port = LMIC.dataBeg - 1;
            Serial.printf("Puerto: %d\n", port);
            Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
            Serial.println();
            Serial.print(LMIC.dataLen);
            Serial.println(F(" bytes of payload"));
            if (*(LMIC.frame + LMIC.dataBeg) == 'I')
            {
                Serial.println("Interior");
                dev_interior = true;
                gps_sleep();
            }
            else if (*(LMIC.frame + LMIC.dataBeg) == 'E')
            {
                Serial.println("Exterior");
                dev_interior = false;
                gps_init();
            }
            else
            {
                Serial.println("RX_PAYLOAD_ERR");
            }
        }
        // Schedule next transmission
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(1), do_send);
        LowPower.deepSleep(sleep_ms);
        break;
    case EV_JOINING:
        Serial.println(F("EV_JOINING: -> Joining..."));
        joinStatus = EV_JOINING;

        if (u8g2)
        {
            u8g2->clearBuffer();
            u8g2->setFont(u8g2_font_IPAandRUSLCD_tr);
            u8g2->drawStr(0, 20, "JOINING");
            u8g2->sendBuffer();
        }

        break;
    case EV_JOIN_FAILED:
        Serial.println(F("EV_JOIN_FAILED: -> Joining failed"));

        if (u8g2)
        {
            u8g2->clearBuffer();
            u8g2->drawStr(0, 12, "OTAA joining failed");
            u8g2->sendBuffer();
        }

        break;
    case EV_JOINED:
        Serial.println(F("EV_JOINED"));
        joinStatus = EV_JOINED;

        if (u8g2)
        {
            u8g2->clearBuffer();
            u8g2->drawStr(0, 12, "Joined TTN!");
            u8g2->sendBuffer();
        }

        delay(3);
        // Disable link check validation (automatically enabled
        // during join, but not supported by TTN at this time).
        LMIC_setLinkCheckMode(0);

        do_send(&sendjob);
        break;
    case EV_RXCOMPLETE:
        Serial.println(F("EV_RXCOMPLETE"));
        break;
    case EV_LINK_DEAD:
        Serial.println(F("EV_LINK_DEAD"));
        break;
    case EV_LINK_ALIVE:
        Serial.println(F("EV_LINK_ALIVE"));
        break;
    case EV_TXSTART:
        Serial.println(F("EV_TXSTART"));
        break;
    case EV_JOIN_TXCOMPLETE:
        Serial.println(F("EV_JOIN_TXCOMPLETE"));
        Serial.println(F("Restarting JOIN process"));
        setupLMIC();
        break;
    default:
        Serial.printf("Unknown event (%d)\n", ev);
        break;
    }
}

/**
 * @brief Configures the SPI and pins for the LoRaWAN module.
 *
 * This function sets the pins for MISO, MOSI, and SCLK for the SPI interface
 * and initializes the SPI communication. It also configures the pin modes and
 * initial states for the radio antenna switch and GPS enable pin.
 */
void setupLMIC(void)
{
    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI); // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);   // g2-band

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(spreadFactor, 14);

    // Start job
    LMIC_startJoining();
}

/**
 * @brief Main loop for LMIC.
 *
 * This function runs the main loop of the LMIC stack, processing scheduled tasks and
 * events.
 */
void loopLMIC(void)
{
    os_runloop_once();
}

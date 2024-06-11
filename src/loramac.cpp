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
    //.rx_level = HIGH
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
static int latest_tx_channels[4] = {-1,-1,-1,-1};
static int tx_channel_pos = 0;

//static String lora_msg = "";
void setupLMIC(void);

void setTXFast(bool mode)
{
    tx_fast_flag = mode;
}

bool getTXFast()
{
    return tx_fast_flag;
}

bool getDEV_INTERIOR()
{
    return dev_interior;
}

/* Data to be uploaded to cayenne */
void printVariables()
{
    lpp.reset();

    if (gps!=nullptr && gps->location.isUpdated() && gps->altitude.isUpdated() && gps->satellites.isUpdated())
    {
        double gps_lat = gps->location.lat();
        double gps_lng = gps->location.lng();
        double gps_alt = gps->altitude.meters();
        lpp.addGPS(3, (float)gps_lat, (float)gps_lng, (float)gps_alt);
    }

    if (dev_interior) lpp.addDigitalInput(4, 1); //Interior
    else lpp.addDigitalInput(4, 0); //Exterior

    //Write channels used in the last 4 tx
    //Bits position represent the channel, B0->Ch0... 1s means channel used
    Serial.printf("Channel next TX: %d\n",LMIC.txChnl);
    latest_tx_channels[tx_channel_pos] = LMIC.txChnl; //Save tx ch
    tx_channel_pos = (tx_channel_pos + 1)%TX_CHANNEL_QTY;
    int tx_channels_used = 0;
    Serial.printf("latest_tx_channels = ");
    for (int i = 0; i < TX_CHANNEL_QTY; i++)
    {
        Serial.printf("%d,",latest_tx_channels[i]);
        //At begining array is filled with -1. If to ignore them.
        if (latest_tx_channels[i] > -1)
        {
            tx_channels_used |= 1<<latest_tx_channels[i];
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
        Serial.println(F("OP_TXRXPEND,sending ..."));
        u8g2->sleepOff();

        printVariables();
        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);

        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_RETRY_INTERVAL), do_send);

        if (u8g2)
        {
            char buf[256];
            u8g2->clearBuffer();
            float batt_lvl = float((Volt * 3.3 * 2) / 4096);
            snprintf(buf, sizeof(buf), "Batt: %.2f", batt_lvl);
            u8g2->drawStr(0, 7, buf);
            if (gps!=nullptr)
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

void onEvent(ev_t ev)
{
    uint32_t sleep_ms = (tx_fast_flag ? TX_INTERVAL_FAST : TX_INTERVAL)*1000;
    //Serial.print(os_getTime());
    //Serial.print(": ");
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
            //lora_msg = "Received ACK.";
        }

        //lora_msg = "rssi:" + String(LMIC.rssi) + " snr: " + String(LMIC.snr);

        if (LMIC.dataLen)
        {
            // data received in rx slot after tx
            //Serial.print(F("Data Received: "));
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
        //os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(tx_fast_flag ? TX_INTERVAL_FAST : TX_INTERVAL), do_send);
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(1), do_send);
        LowPower.deepSleep(sleep_ms);
        break;
    case EV_JOINING:
        Serial.println(F("EV_JOINING: -> Joining..."));
        //lora_msg = "OTAA joining....";
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
        //lora_msg = "OTAA Joining failed";

        if (u8g2)
        {
            u8g2->clearBuffer();
            u8g2->drawStr(0, 12, "OTAA joining failed");
            u8g2->sendBuffer();
        }

        break;
    case EV_JOINED:
        Serial.println(F("EV_JOINED"));
        //lora_msg = "Joined!";
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
        // data received in ping slot
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
        Serial.printf("Unknown event (%d)\n",ev);
        break;
    }
}

void setupLMIC(void)
{
    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.

    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI); // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);   // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(spreadFactor, 14);

    // Start job
    LMIC_startJoining();
}

void loopLMIC(void)
{
    os_runloop_once();
}

#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include "config.h"
#include <CayenneLPP.h>

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
static const unsigned TX_INTERVAL_FAST = 7;
static bool TX_FAST_FLAG = false;
static String lora_msg = "";

void setTXFast(bool mode)
{
    TX_FAST_FLAG = mode;
}

bool getTXFast()
{
    return TX_FAST_FLAG;
}

/* Data to be uploaded to cayenne */
void printVariables()
{
    lpp.reset();

    if (gps->location.isUpdated() && gps->altitude.isUpdated() && gps->satellites.isUpdated())
    {
        double gps_lat = gps->location.lat();
        double gps_lng = gps->location.lng();
        double gps_alt = gps->altitude.meters();
        lpp.addGPS(3, (float)gps_lat, (float)gps_lng, (float)gps_alt);

        // uint32_t Value = gps->satellites.value();
        // lpp.addGenericSensor(5, Value);
    }

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
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
    }
    else if (LMIC.opmode & OP_TXRXPEND)
    {
        Serial.println(F("OP_TXRXPEND, not sending"));
    }
    else
    {
        Serial.println(F("OP_TXRXPEND,sending ..."));

        printVariables();
        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);

        // static uint8_t mydata[] = "Hello, world!";
        // Prepare upstream data transmission at the next possible time.
        // LMIC_setTxData2(1, mydata, sizeof(mydata) - 1, 0);

        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_FAST_FLAG ? TX_INTERVAL_FAST : TX_INTERVAL), do_send);

        if (u8g2)
        {
            char buf[256];
            u8g2->clearBuffer();
            snprintf(buf, sizeof(buf), "[%lu]data sending!", millis() / 1000);
            u8g2->drawStr(0, 12, buf);
            u8g2->sendBuffer();
        }
    }
}

void onEvent(ev_t ev)
{
    Serial.print(os_getTime());
    Serial.print(": ");
    switch (ev)
    {
    case EV_TXCOMPLETE:
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));

        if (LMIC.txrxFlags & TXRX_ACK)
        {
            Serial.println(F("Received ack"));
            lora_msg = "Received ACK.";
        }

        lora_msg = "rssi:" + String(LMIC.rssi) + " snr: " + String(LMIC.snr);

        if (LMIC.dataLen)
        {
            // data received in rx slot after tx
            Serial.print(F("Data Received: "));
            // Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
            // Serial.println();
            Serial.println(LMIC.dataLen);
            Serial.println(F(" bytes of payload"));
        }
        // Schedule next transmission
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_FAST_FLAG ? TX_INTERVAL_FAST : TX_INTERVAL), do_send);
        break;
    case EV_JOINING:
        Serial.println(F("EV_JOINING: -> Joining..."));
        lora_msg = "OTAA joining....";
        joinStatus = EV_JOINING;

        if (u8g2)
        {
            u8g2->clearBuffer();
            u8g2->drawStr(0, 12, "OTAA joining....");
            u8g2->sendBuffer();
        }

        break;
    case EV_JOIN_FAILED:
        Serial.println(F("EV_JOIN_FAILED: -> Joining failed"));
        lora_msg = "OTAA Joining failed";

        if (u8g2)
        {
            u8g2->clearBuffer();
            u8g2->drawStr(0, 12, "OTAA joining failed");
            u8g2->sendBuffer();
        }

        break;
    case EV_JOINED:
        Serial.println(F("EV_JOINED"));
        lora_msg = "Joined!";
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
    default:
        Serial.println(F("Unknown event"));
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

    do_send(&sendjob); // Will fire up also the join
}

void loopLMIC(void)
{
    os_runloop_once();
}

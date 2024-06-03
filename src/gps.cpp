#include <TinyGPS++.h>
#include "config.h"

TinyGPSPlus *gps = nullptr;
HardwareSerial gpsPort(GPS_RX, GPS_TX);
bool GPS_SLEEP_FLAG = true;

void GPS_WaitAck(String cmd, String arg = "")
{
    while (1)
    {
        if (arg != "")
        {
            gpsPort.print(cmd);
            gpsPort.print(" ");
            gpsPort.println(arg);
        }
        else
        {
            gpsPort.println(cmd);
        }
        String ack = "";
        uint32_t smap = millis() + 500;
        while (millis() < smap)
        {
            if (gpsPort.available() > 0)
            {
                ack = gpsPort.readStringUntil('\n');
                String acc = "[" + cmd.substring(1) + "] " + "Done";
                if (ack.startsWith(acc))
                {
                    return;
                }
            }
        }
    }
}
void gps_init(void)
{
    if (GPS_SLEEP_FLAG){
        gps = new TinyGPSPlus();
        gpsPort.begin(GPS_BAUD_RATE);
        pinMode(GPS_EN, OUTPUT);
        digitalWrite(GPS_EN, HIGH);
        pinMode(GPS_RST, GPIO_PULLUP);
        // Set  Reset Pin as 0
        digitalWrite(GPS_RST, LOW);
        // Scope shows 1.12s (Low Period)
        delay(200);
        // Set  Reset Pin as 1
        digitalWrite(GPS_RST, HIGH);
        delay(500);

        GPS_WaitAck("@GSTP"); //Positioning stop
        GPS_WaitAck("@BSSL", "0x2EF"); //Output sentence select, all outputs
        GPS_WaitAck("@GSOP", "1 1000 0"); //Operation mode normal, position cycle, sleep time
        GPS_WaitAck("@GNS", "0x03"); // Use the GPS and GLONASS systems.
        //! Start GPS connamd
        GPS_WaitAck("@GSR");
        delay(200);
        GPS_SLEEP_FLAG = false;
    }
}

void gps_sleep(void) {
    if (!GPS_SLEEP_FLAG){
        gpsPort.flush();
        GPS_WaitAck("@GSTP"); //Positioning stop
        GPS_WaitAck("@SLP", "2");//Sleep mode 2
        Serial.println(F("GPS SLEEP!!"));
        gpsPort.end();
        GPS_SLEEP_FLAG = true;
    }
}

void gps_loop(void)
{
    if (!GPS_SLEEP_FLAG)
    {
        while (gpsPort.available() > 0)
        {
            gps->encode(gpsPort.read());
        }
        /*if (gps->charsProcessed() < 10)
        {
            Serial.println(F("WARNING: No GPS data.  Check wiring."));
        }*/
    }
}
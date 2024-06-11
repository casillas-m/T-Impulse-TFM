#include <TinyGPS++.h>
#include "config.h"

TinyGPSPlus *gps = nullptr;
HardwareSerial gpsPort(GPS_RX, GPS_TX);
bool GPS_SLEEP_FLAG = true;

/**
 * @brief Sends a command to the GPS module and waits for an acknowledgment.
 *
 * This function sends a command to the GPS module through the serial port and waits
 * for an acknowledgment response. It optionally sends an argument with the command.
 * The function continuously checks for the acknowledgment until it receives the expected
 * response.
 *
 * @param cmd The command to be sent to the GPS module.
 * @param arg An optional argument to be sent with the command.
 */
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

/**
 * @brief Initializes the GPS module.
 *
 * This function sets up the GPS module by configuring the necessary pins, enabling the
 * GPS module, resetting it, and sending initialization commands to set various
 * parameters. It ensures the GPS module is ready for operation.
 */
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

/**
 * @brief Puts the GPS module into sleep mode to save power.
 *
 * This function stops the GPS positioning, puts the GPS module into sleep mode, and
 * ends the serial communication to save power. It also updates the GPS sleep flag.
 */
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

/**
 * @brief Processes GPS data in the main loop.
 *
 * This function reads available data from the GPS module and feeds it to the TinyGPSPlus
 * library for parsing. It ensures that the GPS data is continuously updated when the GPS
 * module is not in sleep mode.
 */
void gps_loop(void)
{
    if (!GPS_SLEEP_FLAG)
    {
        while (gpsPort.available() > 0)
        {
            gps->encode(gpsPort.read());
        }
    }
}
// Test code for Adafruit GPS modules using MTK3329/MTK3339 driver
//
// This code turns on the LOCUS built-in datalogger. The datalogger
// turns off when power is lost, so you MUST turn it on every time
// you want to use it!
//
// Tested and works great with the Adafruit GPS FeatherWing
// ------> https://www.adafruit.com/products/3133
// or Flora GPS
// ------> https://www.adafruit.com/products/1059
// but also works with the shield, breakout
// ------> https://www.adafruit.com/products/1272
// ------> https://www.adafruit.com/products/746
//
// Pick one up today at the Adafruit electronics shop
// and help support open source hardware & software! -ada


#include <Adafruit_GPS.h>

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  true

void setup()
{
  //while (!Serial);  // uncomment to have the sketch wait until Serial is ready

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  delay(1000);
  Serial.println("Adafruit GPS logging start test!");

  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);

  // You can adjust which sentences to have the module emit, below
  // Default is RMC + GGA
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Default is 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);
  // Ask for firmware version
  GPS.sendCommand(PMTK_Q_RELEASE);

  Serial.print("\nSTARTING LOGGING....");
  if (GPS.LOCUS_StartLogger())
    Serial.println(" STARTED!");
  else
    Serial.println(" no response :(");
}

uint32_t timer = 0;

void loop()                     // run over and over again
{
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if ((c) && (GPSECHO))
    Serial.write(c);

  if (millis() - timer > 1000)
  {
    timer = millis();
    if (GPS.LOCUS_ReadStatus()) {
       Serial.print("\n\nLog #");
       Serial.print(GPS.LOCUS_serial, DEC);
      if (GPS.LOCUS_type == LOCUS_OVERLAP)
        Serial.print(", Overlap, ");
      else if (GPS.LOCUS_type == LOCUS_FULLSTOP)
        Serial.print(", Full Stop, Logging");

      if (GPS.LOCUS_mode & 0x1) Serial.print(" AlwaysLocate");
      if (GPS.LOCUS_mode & 0x2) Serial.print(" FixOnly");
      if (GPS.LOCUS_mode & 0x4) Serial.print(" Normal");
      if (GPS.LOCUS_mode & 0x8) Serial.print(" Interval");
      if (GPS.LOCUS_mode & 0x10) Serial.print(" Distance");
      if (GPS.LOCUS_mode & 0x20) Serial.print(" Speed");

      Serial.print(", Content "); Serial.print((int)GPS.LOCUS_config);
      Serial.print(", Interval "); Serial.print((int)GPS.LOCUS_interval);
      Serial.print(" sec, Distance "); Serial.print((int)GPS.LOCUS_distance);
      Serial.print(" m, Speed "); Serial.print((int)GPS.LOCUS_speed);
      Serial.print(" m/s, Status ");
      if (GPS.LOCUS_status)
        Serial.print("LOGGING, ");
      else
        Serial.print("OFF, ");
      Serial.print((int)GPS.LOCUS_records); Serial.print(" Records, ");
      Serial.print((int)GPS.LOCUS_percent); Serial.print("% Used ");

    }//if (GPS.LOCUS_ReadStatus())
  }//if (millis() - timer > 1000)
}//loop

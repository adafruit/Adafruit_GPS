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

void loop()                     // run over and over again
{
  if (GPSSerial.available()) {
    char c = GPSSerial.read();
    Serial.write(c);
  }

  // Do other things here....
}

/******************************************************************/




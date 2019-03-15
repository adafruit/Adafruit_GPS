// Test code for Adafruit GPS modules using MTK3329/MTK3339 driver
//
// This code erases the LOCUS built-in datalogger storage
//
// Tested and works great with the Adafruit Ultimate GPS module
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/746
// Pick one up today at the Adafruit electronics shop
// and help support open source hardware & software! -ada

//This code is intended for use with Arduino Leonardo and other ATmega32U4-based Arduinos

#include <Adafruit_GPS.h>

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// If using software serial (sketch example default):
//   Connect the GPS TX (transmit) pin to Digital 8
//   Connect the GPS RX (receive) pin to Digital 7
// If using hardware serial:
//   Connect the GPS TX (transmit) pin to Arduino RX1 (Digital 0)
//   Connect the GPS RX (receive) pin to matching TX1 (Digital 1)

// If using software serial, keep these lines enabled
// (you can change the pin numbers to match your wiring):

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  true


void setup()
{
  while (!Serial) ;  //wait for serial port on Leonardo

  // connect at 115200 so we can read the GPS fast enuf and
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit GPS erase FLASH!");

  // 9600 NMEA is the default baud rate for MTK
  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_OFF);

  Serial.println("This code will ERASE the data log stored in the FLASH - Permanently!");
  Serial.print("Are you sure you want to do this? [Y/N]: ");
  while (Serial.read() != 'Y')   delay(10);
  Serial.println("\nERASING! UNPLUG YOUR ARDUINO WITHIN 5 SECONDS IF YOU DIDNT MEAN TO!");
  delay(5000);
  GPS.sendCommand(PMTK_LOCUS_ERASE_FLASH);
  Serial.println("Erased");
}

void loop()                     // run over and over again
{
  if (GPSSerial.available()) {
    char c = GPSSerial.read();
    if (GPSECHO)
      Serial.write(c);
  }
}

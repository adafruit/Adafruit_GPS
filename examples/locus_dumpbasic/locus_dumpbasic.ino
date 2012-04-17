// Test code for Adafruit GPS modules using MTK3329/MTK3339 driver
//
// This code turns on the LOCUS built-in datalogger. The datalogger
// turns off when power is lost, so you MUST turn it on every time
// you want to use it!
//
// Tested and works great with the Adafruit Ultimate GPS module
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/746
// Pick one up today at the Adafruit electronics shop 
// and help support open source hardware & software! -ada

#include <Adafruit_GPS.h>

// these are for Arduino 1.0
#include <SoftwareSerial.h>
SoftwareSerial mySerial(3, 2);

// if using Arduino v23 or earlier, uncomment these
// two lines and comment out the above. You will
// need to install NewSoftSerial
//  #include <NewSoftSerial.h>
//  NewSoftSerial mySerial(3, 2);

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to Digital 3
// Connect the GPS RX (receive) pin to Digital 2
Adafruit_GPS GPS(&mySerial);

void setup()  
{    
  // connect at 115200 so we can read the GPS fast enuf and
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit GPS logging start test!");

  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);
  
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_OFF);
  
  while (mySerial.available())
     mySerial.read();

  delay(1000);
  GPS.sendCommand("$PMTK622,1*29");
  Serial.println("----------------------------------------------------");
}


void loop()                     // run over and over again
{  
  if (mySerial.available()) {
    char c = mySerial.read();
      if (c) UDR0 = c;  
  }
}



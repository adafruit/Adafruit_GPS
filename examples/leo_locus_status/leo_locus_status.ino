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

//This code is intended for use with Arduino Leonardo and other ATmega32U4-based Arduinos

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

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
//SoftwareSerial mySerial(8, 7);
//Adafruit_GPS GPS(&mySerial);

// If using hardware serial, comment
// out the above two lines and enable these two lines instead:
Adafruit_GPS GPS(&Serial1);
HardwareSerial mySerial = Serial1;

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false

void setup()  
{    
  // connect at 115200 so we can read the GPS fast enuf and
  // also spit it out
  Serial.begin(115200);
  delay(5000);
  Serial.println("Adafruit GPS logging start test!");

  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);
  
  // You can adjust which sentences to have the module emit, below
  // Default is RMC + GGA
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Default is 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  while (true) {
    Serial.print("Starting logging....");
    if (GPS.LOCUS_StartLogger()) {
      Serial.println(" STARTED!");
      break;
    } else {
      Serial.println(" no response :(");
    }
  }
}

uint32_t updateTime = 1000;

void loop()                     // run over and over again
{
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if ((c) && (GPSECHO))
    Serial.write(c); 
    
  if (millis() > updateTime)
  {
    updateTime = millis() + 1000;
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
  }//if (millis() > updateTime)
}//loop




// Test code for Adafruit GPS modules using MTK3329/MTK3339 driver
//
// This code just echos whatever is coming from the GPS unit to the
// serial monitor, handy for debugging!
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
SoftwareSerial GPS_Serial(8, 7);

// If using hardware serial, comment
// out the above two lines and enable these two lines instead:
//HardwareSerial GPS_Serial = Serial1;

void setup() {

  Serial.begin(57600); // this baud rate doesn't actually matter!
  while (!Serial); // wait for leo to be ready
  GPS_Serial.begin(9600);
  delay(2000);
  Serial.println("Get version!");
  GPS_Serial.println(PMTK_Q_RELEASE);
  
  // you can send various commands to get it started
  //GPS_Serial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS_Serial.println(PMTK_SET_NMEA_OUTPUT_ALLDATA);

  GPS_Serial.println(PMTK_SET_NMEA_UPDATE_1HZ);
 }


void loop() {
  if (Serial.available()) {
   char c = Serial.read();
   Serial.write(c);
   GPS_Serial.write(c);
  }
  if (GPS_Serial.available()) {
    char c = GPS_Serial.read();
    Serial.write(c);
  }
}

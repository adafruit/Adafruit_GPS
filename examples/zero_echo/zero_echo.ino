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

// This code is intended for use with Arduino Zero

#include <Adafruit_GPS.h>
#include "wiring_private.h"

// If using the ultimate gps shield, cut the traces between 
//   TX and TX and RX and RX on the shield. Solder 2 jumpers;
//   one between the TX pin (by pin 7), and pin 5. The other
//   one between the RX pin (by pin 6), and pin 4.
// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to Digital 5 (Zero new RX)
// Connect the GPS RX (receive) pin to Digital 4 (Zero new TX)

Uart mySerial(&sercom2, 5, 4, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM2_Handler()
{
  mySerial.IrqHandler();
}

#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"

// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn off output
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

#define PMTK_Q_RELEASE "$PMTK605*31"

void setup() {
  Serial.begin(57600); // this baud rate doesn't actually matter!
  mySerial.begin(9600);
  delay(2000);

  // Assign pins 5 & 4 SERCOM functionality
  pinPeripheral(5, PIO_SERCOM); // rx
  pinPeripheral(4, PIO_SERCOM_ALT); // tx

  Serial.println("Get version!");
  mySerial.println(PMTK_Q_RELEASE);
  
  // you can send various commands to get it started
  //mySerial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  mySerial.println(PMTK_SET_NMEA_OUTPUT_ALLDATA);

  mySerial.println(PMTK_SET_NMEA_UPDATE_1HZ);
 }


void loop() {
  if (Serial.available()) {
   char c = Serial.read();
   Serial.write(c);
   mySerial.write(c);
  }
  if (mySerial.available()) {
    char c = mySerial.read();
    Serial.write(c);
  }
}

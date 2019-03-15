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
#include <SoftwareSerial.h>

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to Digital 8
// Connect the GPS RX (receive) pin to Digital 7

// you can change the pin numbers to match your wiring:
SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false

// this keeps track of whether we're using the interrupt
// off by default!
#ifndef ESP8266 // Sadly not on ESP8266
boolean usingInterrupt = false;
#endif

void setup()
{
  while (!Serial);  // the Leonardo will 'wait' until the USB plug is connected

  // connect at 115200 so we can read the GPS fast enuf and
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit GPS logging start test!");

  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);

  // You can adjust which sentences to have the module emit, below
  // Default is RMC + GGA
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Default is 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
#ifndef ESP8266 // Not on ESP8266
  useInterrupt(true);
#endif

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


/******************************************************************/
// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
#ifndef ESP8266 // Not on ESP8266
ISR(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  // writing direct to UDR0 is much much faster than Serial.print
  // but only one character can be written at a time.
  UDR0 = c;
#else
  Serial.print(c);
#endif
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}
#endif // ESP8266

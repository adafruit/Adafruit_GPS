#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SdFat_Adafruit_Fork.h>
#include <avr/sleep.h>

// Ladyada's logger modified by Bill Greiman to use the SdFat library
//
// This code shows how to listen to the GPS module in an interrupt
// which allows the program to have more 'freedom' - just parse
// when a new NMEA sentence is available! Then access data when
// desired.
//
// Tested and works great with the Adafruit Ultimate GPS Shield
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/
// Pick one up today at the Adafruit electronics shop
// and help support open source hardware & software! -ada
// Fllybob added 10 sec logging option
SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  true
/* set to true to only log to SD when GPS has a fix, for debugging, keep it false */
#define LOG_FIXONLY false

// this keeps track of whether we're using the interrupt
// off by default!
#ifndef ESP8266 // Sadly not on ESP8266
bool usingInterrupt = false;
#endif

// Set the pins used
#define chipSelect 10
#define ledPin 13

SdFat SD;
// Raw NMEA logging only needs the base file API. Avoid the Arduino Stream
// wrapper, whose virtual methods also pull unused read/seek code into flash.
SdBaseFile logfile;

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink out an error code
void error(uint8_t errno) {
  /*
  if (SD.errorCode()) {
   putstring("SD error: ");
   Serial.print(card.errorCode(), HEX);
   Serial.print(',');
   Serial.println(card.errorData(), HEX);
   }
   */
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

// Keep SD initialization buffers off the stack once logging starts.
void setup() __attribute__((noinline));
void setup() {
  // for Leonardos, if you want to debug SD issues, uncomment this line
  // to see serial output
  //while (!Serial);

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println(F("\r\nUltimate GPSlogger Shield"));
  pinMode(ledPin, OUTPUT);

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card init. failed!"));
    error(2);
  }
  char filename[15];
  strcpy(filename, "GPSLOG0000.TXT");
  // Find an unused long filename, then create it without opening an old log.
  while (SD.exists(filename)) {
    // Carry through the four digits, keeping the filename as the counter.
    int8_t digit = 9;
    while (digit >= 6 && filename[digit] == '9') {
      filename[digit] = '0';
      digit--;
    }
    if (digit < 6) {
      // All 10000 names exist. Exclusive creation below will fail safely.
      break;
    }
    filename[digit]++;
  }

  if (!logfile.open(filename, O_WRONLY | O_CREAT | O_EXCL)) {
    Serial.print(F("Couldnt create "));
    Serial.println(filename);
    error(3);
  }
  Serial.print(F("Writing to "));
  Serial.println(filename);

  // connect to the GPS at the desired rate
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(F(PMTK_SET_NMEA_OUTPUT_RMCGGA));
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(F(PMTK_SET_NMEA_OUTPUT_RMCONLY));
  // For logging data, we don't suggest using anything but either RMC only or RMC+GGA
  // to keep the log files at a reasonable size
  // Set the update rate
  GPS.sendCommand(F(PMTK_SET_NMEA_UPDATE_1HZ));   // 100 millihertz (once every 10 seconds), 1Hz or 5Hz update rate

  // Disable antenna status on old and new modules, if the firmware permits it.
  GPS.sendCommand(F(PGCMD_NOANTENNA));
  GPS.sendCommand(F(CDCMD_NOANTENNA));

  // Drain startup command replies before the first SD write. Their rapid
  // arrival can otherwise reuse a GPS receive buffer while it is being saved.
  uint32_t start = millis();
  while (millis() - start < 2000) {
    GPS.read();
    if (GPS.newNMEAreceived()) {
      GPS.lastNMEA();
    }
  }

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
#ifndef ESP8266 // Not on ESP8266
  useInterrupt(true);
#endif

  Serial.println(F("Ready!"));
}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
#ifndef ESP8266 // Not on ESP8266
ISR(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  #ifdef UDR0
      if (GPSECHO)
        if (c) UDR0 = c;
      // writing direct to UDR0 is much much faster than Serial.print
      // but only one character can be written at a time.
  #endif
}

void useInterrupt(bool v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  }
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}
#endif // ESP8266

void loop() {
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
  }

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data

    // Don't call lastNMEA more than once between parse calls!  Calling lastNMEA
    // will clear the received flag and can cause very subtle race conditions if
    // new data comes in before parse is called again.
    char *stringptr = GPS.lastNMEA();

    if (!GPS.parse(stringptr))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another

    // Sentence parsed!
    Serial.println(F("OK"));
    if (LOG_FIXONLY && !GPS.fix) {
      Serial.print(F("No Fix"));
      return;
    }

    // Rad. lets log it!
    Serial.println(F("Log"));

    uint8_t stringsize = strlen(stringptr);
    if (stringsize != logfile.write((uint8_t *)stringptr, stringsize))    //write the string to the SD file
        error(4);
    if (strstr(stringptr, "RMC") || strstr(stringptr, "GGA"))   logfile.sync();
    Serial.println();
  }
}

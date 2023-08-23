/**************************************************************************/
/*!
  @file Adafruit_GPS.cpp

  @mainpage Adafruit Ultimate GPS Breakout

  @section intro Introduction

  This is the Adafruit GPS library - the ultimate GPS library
  for the ultimate GPS module!

  Tested and works great with the Adafruit Ultimate GPS module
  using MTK33x9 chipset
  ------> http://www.adafruit.com/products/746

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  @section author Author

  Written by Limor Fried/Ladyada for Adafruit Industries.

  @section license License

  BSD license, check license.txt for more information
  All text above must be included in any redistribution
*/
/**************************************************************************/

#include <Adafruit_GPS.h>

static bool strStartsWith(const char *str, const char *prefix);

/**************************************************************************/
/*!
    @brief Start the HW or SW serial port
    @param baud_or_i2caddr Baud rate if using serial, I2C address if using I2C
    @returns True on successful hardware init, False on failure
*/
/**************************************************************************/
bool Adafruit_GPS::begin(uint32_t baud_or_i2caddr) {
#if (defined(__AVR__) || ((defined(ARDUINO_UNOR4_WIFI) || defined(ESP8266)) && \
                          !defined(NO_SW_SERIAL)))
  if (gpsSwSerial) {
    if (!gpsSwSerial->begin(baud_or_i2caddr)) {
      return false;
    }
  }
#endif
  if (gpsHwSerial) {
    if (!gpsHwSerial->begin(baud_or_i2caddr)) {
      return false;
    }
  }
  if (gpsI2C) {
    gpsI2C->begin();
    if (baud_or_i2caddr > 0x7F) {
      _i2caddr = GPS_DEFAULT_I2C_ADDR;
    } else {
      _i2caddr = baud_or_i2caddr;
    }
    // A basic scanner, see if it ACK's
    gpsI2C->beginTransmission(_i2caddr);
    if (gpsI2C->endTransmission() != 0) {
      return false;
    }
  }
  if (gpsSPI) {
    gpsSPI->begin();
    gpsSPI_settings = SPISettings(baud_or_i2caddr, MSBFIRST, SPI_MODE0);
    if (gpsSPI_cs >= 0) {
      pinMode(gpsSPI_cs, OUTPUT);
      digitalWrite(gpsSPI_cs, HIGH);
    }
  }

  delay(10);
  return true;
}

/**************************************************************************/
/*!
    @brief Constructor when using SoftwareSerial
    @param ser Pointer to SoftwareSerial device
*/
/**************************************************************************/
#if (defined(__AVR__) || ((defined(ARDUINO_UNOR4_WIFI) || defined(ESP8266)) && \
                          !defined(NO_SW_SERIAL)))
Adafruit_GPS::Adafruit_GPS(SoftwareSerial *ser) {
  common_init();     // Set everything to common state, then...
  gpsSwSerial = ser; // ...override gpsSwSerial with value passed.
}
#endif

/**************************************************************************/
/*!
    @brief Constructor when using HardwareSerial
    @param ser Pointer to a HardwareSerial object
*/
/**************************************************************************/
Adafruit_GPS::Adafruit_GPS(HardwareSerial *ser) {
  common_init();     // Set everything to common state, then...
  gpsHwSerial = ser; // ...override gpsHwSerial with value passed.
}

/**************************************************************************/
/*!
    @brief Constructor when using Stream
    @param data Pointer to a Stream object
*/
/**************************************************************************/
Adafruit_GPS::Adafruit_GPS(Stream *data) {
  common_init();    // Set everything to common state, then...
  gpsStream = data; // ...override gpsStream with value passed.
}

/**************************************************************************/
/*!
    @brief Constructor when using I2C
    @param theWire Pointer to an I2C TwoWire object
*/
/**************************************************************************/
Adafruit_GPS::Adafruit_GPS(TwoWire *theWire) {
  common_init();    // Set everything to common state, then...
  gpsI2C = theWire; // ...override gpsI2C
}

/**************************************************************************/
/*!
    @brief Constructor when using SPI
    @param theSPI Pointer to an SPI device object
    @param cspin The pin connected to the GPS CS, can be -1 if unused
*/
/**************************************************************************/
Adafruit_GPS::Adafruit_GPS(SPIClass *theSPI, int8_t cspin) {
  common_init();   // Set everything to common state, then...
  gpsSPI = theSPI; // ...override gpsSPI
  gpsSPI_cs = cspin;
}

/**************************************************************************/
/*!
    @brief Constructor when there are no communications attached
*/
/**************************************************************************/
Adafruit_GPS::Adafruit_GPS() {
  common_init(); // Set everything to common state, then...
  noComms = true;
}

/**************************************************************************/
/*!
    @brief Initialization code used by all constructor types
*/
/**************************************************************************/
void Adafruit_GPS::common_init(void) {
#if (defined(__AVR__) || ((defined(ARDUINO_UNOR4_WIFI) || defined(ESP8266)) && \
                          !defined(NO_SW_SERIAL)))
  gpsSwSerial = NULL; // Set both to NULL, then override correct
#endif
  gpsHwSerial = NULL; // port pointer in corresponding constructor
  gpsStream = NULL;   // port pointer in corresponding constructor
  gpsI2C = NULL;
  gpsSPI = NULL;
  recvdflag = false;
  paused = false;
  lineidx = 0;
  currentline = line1;
  lastline = line2;

  hour = minute = seconds = year = month = day = fixquality = fixquality_3d =
      satellites = antenna = 0; // uint8_t
  lat = lon = mag = 0;          // char
  fix = false;                  // bool
  milliseconds = 0;             // uint16_t
  latitude = longitude = geoidheight = altitude = speed = angle = magvariation =
      HDOP = VDOP = PDOP = 0.0; // nmea_float_t
#ifdef NMEA_EXTENSIONS
  data_init();
#endif
}

/**************************************************************************/
/*!
    @brief    Destroy the object.
    @return   none
*/
/**************************************************************************/
Adafruit_GPS::~Adafruit_GPS() {
#ifdef NMEA_EXTENSIONS
  for (int i = 0; i < (int)NMEA_MAX_INDEX; i++)
    removeHistory((nmea_index_t)i); // to free any history mallocs
#endif
}

/**************************************************************************/
/*!
    @brief How many bytes are available to read - part of 'Print'-class
   functionality
    @return Bytes available, 0 if none
*/
/**************************************************************************/
size_t Adafruit_GPS::available(void) {
  if (paused || noComms)
    return 0;

#if (defined(__AVR__) || ((defined(ARDUINO_UNOR4_WIFI) || defined(ESP8266)) && \
                          !defined(NO_SW_SERIAL)))
  if (gpsSwSerial) {
    return gpsSwSerial->available();
  }
#endif
  if (gpsHwSerial) {
    return gpsHwSerial->available();
  }
  if (gpsStream) {
    return gpsStream->available();
  }
  if (gpsI2C || gpsSPI) {
    return 1; // I2C/SPI doesn't have 'availability' so always has a byte at
              // least to read!
  }
  return 0;
}

/**************************************************************************/
/*!
    @brief Write a byte to the underlying transport - part of 'Print'-class
   functionality
    @param c A single byte to send
    @return Bytes written - 1 on success, 0 on failure
*/
/**************************************************************************/
size_t Adafruit_GPS::write(uint8_t c) {
#if (defined(__AVR__) || ((defined(ARDUINO_UNOR4_WIFI) || defined(ESP8266)) && \
                          !defined(NO_SW_SERIAL)))
  if (gpsSwSerial) {
    return gpsSwSerial->write(c);
  }
#endif
  if (gpsHwSerial) {
    return gpsHwSerial->write(c);
  }
  if (gpsStream) {
    return gpsStream->write(c);
  }
  if (gpsI2C) {
    gpsI2C->beginTransmission(_i2caddr);
    if (gpsI2C->write(c) != 1) {
      return 0;
    }
    if (gpsI2C->endTransmission(true) == 0) {
      return 1;
    }
  }
  if (gpsSPI) {
    gpsSPI->beginTransaction(gpsSPI_settings);
    if (gpsSPI_cs >= 0) {
      digitalWrite(gpsSPI_cs, LOW);
    }
    c = gpsSPI->transfer(c);
    if (gpsSPI_cs >= 0) {
      digitalWrite(gpsSPI_cs, HIGH);
    }
    gpsSPI->endTransaction();
    return 1;
  }

  return 0;
}

/**************************************************************************/
/*!
    @brief Read one character from the GPS device.

    Call very frequently and multiple times per opportunity or the buffer
    may overflow if there are frequent NMEA sentences. An 82 character NMEA
    sentence 10 times per second will require 820 calls per second, and
    once a loop() may not be enough. Check for newNMEAreceived() after at
    least every 10 calls, or you may miss some short sentences.
    @return The character that we received, or 0 if nothing was available
*/
/**************************************************************************/
char Adafruit_GPS::read(void) {
  static uint32_t firstChar = 0; // first character received in the current sentence
  uint32_t tStart = millis();    // as close as we can get to the time the char was sent
  char c = 0;

  if (paused || noComms)
    return c;

#if (defined(__AVR__) || ((defined(ARDUINO_UNOR4_WIFI) || defined(ESP8266)) && \
                          !defined(NO_SW_SERIAL)))
  if (gpsSwSerial) {
    if (!gpsSwSerial->available())
      return c;
    c = gpsSwSerial->read();
  }
#endif
  if (gpsHwSerial) {
    if (!gpsHwSerial->available())
      return c;
    c = gpsHwSerial->read();
  }
  if (gpsStream) {
    if (!gpsStream->available())
      return c;
    c = gpsStream->read();
  }
  if (gpsI2C) {
    if (_buff_idx <= _buff_max) {
      c = _i2cbuffer[_buff_idx];
      _buff_idx++;
    } else {
      // refill the buffer!
      if (gpsI2C->requestFrom((uint8_t)0x10, (uint8_t)GPS_MAX_I2C_TRANSFER,
                              (uint8_t) true) == GPS_MAX_I2C_TRANSFER) {
        // got data!
        _buff_max = 0;
        char curr_char = 0;
        for (int i = 0; i < GPS_MAX_I2C_TRANSFER; i++) {
          curr_char = gpsI2C->read();
          if ((curr_char == 0x0A) && (last_char != 0x0D)) {
            // skip duplicate 0x0A's - but keep as part of a CRLF
            continue;
          }
          last_char = curr_char;
          _i2cbuffer[_buff_max] = curr_char;
          _buff_max++;
        }
        _buff_max--; // back up to the last valid slot
        if ((_buff_max == 0) && (_i2cbuffer[0] == 0x0A)) {
          _buff_max = -1; // ahh there was nothing to read after all
        }
        _buff_idx = 0;
      }
      return c;
    }
  }

  if (gpsSPI) {
    do {
      gpsSPI->beginTransaction(gpsSPI_settings);
      if (gpsSPI_cs >= 0) {
        digitalWrite(gpsSPI_cs, LOW);
      }
      c = gpsSPI->transfer(0xFF);
      if (gpsSPI_cs >= 0) {
        digitalWrite(gpsSPI_cs, HIGH);
      }
      gpsSPI->endTransaction();
      // skip duplicate 0x0A's - but keep as part of a CRLF
    } while (((c == 0x0A) && (last_char != 0x0D)) ||
             (!isprint(c) && !isspace(c)));
    last_char = c;
  }

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH -
              1; // ensure there is someplace to put the next received character

  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    lineidx = 0;
    recvdflag = true;
    recvdTime = millis(); // time we got the end of the string
    sentTime = firstChar;
    firstChar = 0; // there are no characters yet
    return c;      // wait until the next character to set time
  }

  if (firstChar == 0)
    firstChar = tStart;
  return c;
}

/**************************************************************************/
/*!
    @brief Send a command to the GPS device
    @param str Pointer to a string holding the command to send
*/
/**************************************************************************/
void Adafruit_GPS::sendCommand(const char *str) { println(str); }

/**************************************************************************/
/*!
    @brief Check to see if a new NMEA line has been received
    @return True if received, false if not
*/
/**************************************************************************/
bool Adafruit_GPS::newNMEAreceived(void) { return recvdflag; }

/**************************************************************************/
/*!
    @brief Pause/unpause receiving new data
    @param p True = pause, false = unpause
*/
/**************************************************************************/
void Adafruit_GPS::pause(bool p) { paused = p; }

/**************************************************************************/
/*!
    @brief Returns the last NMEA line received and unsets the received flag
    @return Pointer to the last line string
*/
/**************************************************************************/
char *Adafruit_GPS::lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
}

/**************************************************************************/
/*!
    @brief Wait for a specified sentence from the device
    @param wait4me Pointer to a string holding the desired response
    @param maxwait Maximum time to wait (ms)
    @return True if the sentence was received, false if timeout occurred
*/
/**************************************************************************/
bool Adafruit_GPS::waitForSentence(const char *wait4me, uint8_t maxwait) {
  char *response;
  uint8_t len = strlen(wait4me);
  uint32_t timer = millis();
  bool found = false;

  while (millis() - timer < maxwait * 1000UL) {
    if (newNMEAreceived()) {
      response = lastNMEA();
      if (strStartsWith(response, wait4me)) {
        found = true;
        break;
      }
    }
  }

  return found;
}

/**************************************************************************/
/*!
    @brief Check if a string starts with a given prefix
    @param str Pointer to the string to check
    @param prefix Pointer to the prefix string
    @return True if the string starts with the prefix, false otherwise
*/
/**************************************************************************/
static bool strStartsWith(const char *str, const char *prefix) {
  while (*prefix) {
    if (*prefix++ != *str++)
      return false;
  }
  return true;
}

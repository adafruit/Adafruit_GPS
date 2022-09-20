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
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  if (gpsSwSerial) {
    gpsSwSerial->begin(baud_or_i2caddr);
  }
#endif
  if (gpsHwSerial) {
    gpsHwSerial->begin(baud_or_i2caddr);
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
    return (gpsI2C->endTransmission() == 0);
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
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
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
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
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
  if (paused)
    return 0;

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
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
    return 1; // I2C/SPI doesnt have 'availability' so always has a byte at
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
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
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
  static uint32_t firstChar = 0; // first character received in current sentence
  uint32_t tStart = millis();    // as close as we can get to time char was sent
  char c = 0;

  if (paused || noComms)
    return c;

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
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
  // Serial.print(c);

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

    // Serial.println("----");
    // Serial.println((char *)lastline);
    // Serial.println("----");
    lineidx = 0;
    recvdflag = true;
    recvdTime = millis(); // time we got the end of the string
    sentTime = firstChar;
    firstChar = 0; // there are no characters yet
    return c;      // wait until next character to set time
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
    @param max How long to wait, default is MAXWAITSENTENCE
    @param usingInterrupts True if using interrupts to read from the GPS
   (default is false)
    @return True if we got what we wanted, false otherwise
*/
/**************************************************************************/
bool Adafruit_GPS::waitForSentence(const char *wait4me, uint8_t max,
                                   bool usingInterrupts) {
  uint8_t i = 0;
  while (i < max) {
    if (!usingInterrupts)
      read();

    if (newNMEAreceived()) {
      char *nmea = lastNMEA();
      i++;

      if (strStartsWith(nmea, wait4me))
        return true;
    }
  }

  return false;
}

/**************************************************************************/
/*!
    @brief Start the LOCUS logger
    @return True on success, false if it failed
*/
/**************************************************************************/
bool Adafruit_GPS::LOCUS_StartLogger(void) {
  sendCommand(PMTK_LOCUS_STARTLOG);
  recvdflag = false;
  return waitForSentence(PMTK_LOCUS_STARTSTOPACK);
}

/**************************************************************************/
/*!
    @brief Stop the LOCUS logger
    @return True on success, false if it failed
*/
/**************************************************************************/
bool Adafruit_GPS::LOCUS_StopLogger(void) {
  sendCommand(PMTK_LOCUS_STOPLOG);
  recvdflag = false;
  return waitForSentence(PMTK_LOCUS_STARTSTOPACK);
}

/**************************************************************************/
/*!
    @brief Read the logger status
    @return True if we read the data, false if there was no response
*/
/**************************************************************************/
bool Adafruit_GPS::LOCUS_ReadStatus(void) {
  sendCommand(PMTK_LOCUS_QUERY_STATUS);

  if (!waitForSentence("$PMTKLOG"))
    return false;

  char *response = lastNMEA();
  uint16_t parsed[10];
  uint8_t i;

  for (i = 0; i < 10; i++)
    parsed[i] = -1;

  response = strchr(response, ',');
  for (i = 0; i < 10; i++) {
    if (!response || (response[0] == 0) || (response[0] == '*'))
      break;
    response++;
    parsed[i] = 0;
    while ((response[0] != ',') && (response[0] != '*') && (response[0] != 0)) {
      parsed[i] *= 10;
      char c = response[0];
      if (isDigit(c))
        parsed[i] += c - '0';
      else
        parsed[i] = c;
      response++;
    }
  }
  LOCUS_serial = parsed[0];
  LOCUS_type = parsed[1];
  if (isAlpha(parsed[2])) {
    parsed[2] = parsed[2] - 'a' + 10;
  }
  LOCUS_mode = parsed[2];
  LOCUS_config = parsed[3];
  LOCUS_interval = parsed[4];
  LOCUS_distance = parsed[5];
  LOCUS_speed = parsed[6];
  LOCUS_status = !parsed[7];
  LOCUS_records = parsed[8];
  LOCUS_percent = parsed[9];

  return true;
}

/**************************************************************************/
/*!
    @brief Standby Mode Switches
    @return False if already in standby, true if it entered standby
*/
/**************************************************************************/
bool Adafruit_GPS::standby(void) {
  if (inStandbyMode) {
    return false; // Returns false if already in standby mode, so that you do
                  // not wake it up by sending commands to GPS
  } else {
    inStandbyMode = true;
    sendCommand(PMTK_STANDBY);
    // return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast
    // enough to catch the message, or something else just is not working
    return true;
  }
}

/**************************************************************************/
/*!
    @brief Wake the sensor up
    @return True if woken up, false if not in standby or failed to wake
*/
/**************************************************************************/
bool Adafruit_GPS::wakeup(void) {
  if (inStandbyMode) {
    inStandbyMode = false;
    sendCommand(""); // send byte to wake it up
    return waitForSentence(PMTK_AWAKE);
  } else {
    return false; // Returns false if not in standby mode, nothing to wakeup
  }
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last position fix was obtained. The
    time returned is limited to 2^32 milliseconds, which is about 49.7 days.
    It will wrap around to zero if no position fix is received
    for this long.
    @return nmea_float_t value in seconds since last fix.
*/
/**************************************************************************/
nmea_float_t Adafruit_GPS::secondsSinceFix() {
  return (millis() - lastFix) / 1000.;
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS time was obtained. The time
    returned is limited to 2^32 milliseconds, which is about 49.7 days. It
    will wrap around to zero if no GPS time is received for this long.
    @return nmea_float_t value in seconds since last GPS time.
*/
/**************************************************************************/
nmea_float_t Adafruit_GPS::secondsSinceTime() {
  return (millis() - lastTime) / 1000.;
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS date was obtained. The time
    returned is limited to 2^32 milliseconds, which is about 49.7 days. It
    will wrap around to zero if no GPS date is received for this long.
    @return nmea_float_t value in seconds since last GPS date.
*/
/**************************************************************************/
nmea_float_t Adafruit_GPS::secondsSinceDate() {
  return (millis() - lastDate) / 1000.;
}

/**************************************************************************/
/*!
    @brief Fakes time of receipt of a sentence. Use between build() and parse()
    to make the timing look like the sentence arrived from the GPS.
*/
/**************************************************************************/
void Adafruit_GPS::resetSentTime() { sentTime = millis(); }

/**************************************************************************/
/*!
    @brief Checks whether a string starts with a specified prefix
    @param str Pointer to a string
    @param prefix Pointer to the prefix
    @return True if str starts with prefix, false otherwise
*/
/**************************************************************************/
static bool strStartsWith(const char *str, const char *prefix) {
  while (*prefix) {
    if (*prefix++ != *str++)
      return false;
  }
  return true;
}

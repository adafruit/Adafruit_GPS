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

static boolean strStartsWith(const char *str, const char *prefix);

/**************************************************************************/
/*!
    @brief Parse a NMEA string
    @param nmea Pointer to the NMEA string
    @return True if we parsed it, false if it has an invalid checksum or invalid
   data
*/
/**************************************************************************/
boolean Adafruit_GPS::parse(char *nmea) {
  // do checksum check
  if (!check(nmea))
    return false;
  // passed the check, so there's a valid source in thisSource and a valid
  // sentence in thisSentence

  // look for a few common sentences
  char *p = nmea; // Pointer to move through the sentence -- good parsers are
                  // non-destructive
  p = strchr(p, ',') +
      1; // Skip to the character after the next comma, then check sentence.

  if (!strcmp(thisSentence, "GGA")) {
    // found GGA
    // get time
    parseTime(p);

    // parse out latitude
    p = strchr(p, ',') + 1;
    parseLat(p);
    p = strchr(p, ',') + 1;
    if (!parseLatDir(p))
      return false;

    // parse out longitude
    p = strchr(p, ',') + 1;
    parseLon(p);
    p = strchr(p, ',') + 1;
    if (!parseLonDir(p))
      return false;

    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      fixquality = atoi(p);
      if (fixquality > 0) {
        fix = true;
        lastFix = sentTime;
      } else
        fix = false;
    }

    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      satellites = atoi(p);
    }

    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      HDOP = atof(p);
    }

    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      altitude = atof(p);
    }

    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      geoidheight = atof(p);
    }
  }

  else if (!strcmp(thisSentence, "RMC")) {
    // found RMC
    // get time
    parseTime(p);

    // fix or no fix
    p = strchr(p, ',') + 1;
    if (!parseFix(p))
      return false;

    // parse out latitude
    p = strchr(p, ',') + 1;
    parseLat(p);
    p = strchr(p, ',') + 1;
    if (!parseLatDir(p))
      return false;

    // parse out longitude
    p = strchr(p, ',') + 1;
    parseLon(p);
    p = strchr(p, ',') + 1;
    if (!parseLonDir(p))
      return false;

    // speed
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      speed = atof(p);
    }

    // angle
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      angle = atof(p);
    }

    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      uint32_t fulldate = atof(p);
      day = fulldate / 10000;
      month = (fulldate % 10000) / 100;
      year = (fulldate % 100);
      lastDate = sentTime;
    }
  }

  else if (!strcmp(thisSentence, "GLL")) {
    // found GLL
    // parse out latitude
    parseLat(p);
    p = strchr(p, ',') + 1;
    if (!parseLatDir(p))
      return false;

    // parse out longitude
    p = strchr(p, ',') + 1;
    parseLon(p);
    p = strchr(p, ',') + 1;
    if (!parseLonDir(p))
      return false;

    // get time
    p = strchr(p, ',') + 1;
    parseTime(p);

    // fix or no fix
    p = strchr(p, ',') + 1;
    if (!parseFix(p))
      return false;
  }

  else if (!strcmp(thisSentence, "GSA")) {
    // found GSA
    // parse out Auto selection, but ignore them
    // parse out 3d fixquality
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      fixquality_3d = atoi(p);
    }
    // skip 12 Satellite PDNs without interpreting them
    for (int i = 0; i < 12; i++)
      p = strchr(p, ',') + 1;

    // parse out PDOP
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      PDOP = atof(p);
    }
    // parse out HDOP, we also parse this from the GGA sentence. Chipset should
    // report the same for both
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      HDOP = atof(p);
    }
    // parse out VDOP
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      VDOP = atof(p);
    }
  }

#ifdef NMEA_EXTENSIONS // Sentences not required for basic GPS functionality
  else if (!strcmp(thisSentence, "TXT")) { //*******************************TXT
    if (!isEmpty(p))
      txtTot = atoi(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      txtN = atoi(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      txtID = atoi(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      parseStr(txtTXT, p, 61); // copy the text to NMEA TXT max of 61 characters
  }
#endif // NMEA_EXTENSIONS

  // we dont parse the remaining, yet!
  else
    return false;

  // Record the successful parsing of where the last data came from and when
  strcpy(lastSource, thisSource);
  strcpy(lastSentence, thisSentence);
  lastUpdate = millis();
  return true;
}

/**************************************************************************/
/*!
    @brief Check an NMEA string for basic format, valid source ID and valid
    and valid sentence ID. Update the values of thisCheck, thisSource and
    thisSentence.
    @param nmea Pointer to the NMEA string
    @return True if well formed, false if it has problems
*/
/**************************************************************************/
boolean Adafruit_GPS::check(char *nmea) {
  thisCheck = 0; // new check
  if (*nmea != '$')
    return false; // doesn't start with $
  else
    thisCheck += NMEA_HAS_DOLLAR;
  // do checksum check -- first look if we even have one -- ignore all but last
  // *
  char *ast = nmea; // not strchr(nmea,'*'); for first *
  while (*ast)
    ast++; // go to the end
  while (*ast != '*' && ast > nmea)
    ast--; // then back to * if it's there
  if (*ast != '*')
    return false; // there is no asterisk
  else {
    uint16_t sum = parseHex(*(ast + 1)) * 16; // extract checksum
    sum += parseHex(*(ast + 2));
    char *p = nmea; // check checksum
    for (char *p1 = p + 1; p1 < ast; p1++)
      sum ^= *p1;
    if (sum != 0)
      return false; // bad checksum :(
    else
      thisCheck += NMEA_HAS_CHECKSUM;
  }
  // extract source of variable length
  char *p = nmea + 1;
  const char *src = tokenOnList(p, sources);
  if (src) {
    strcpy(thisSource, src);
    thisCheck += NMEA_HAS_SOURCE;
  } else
    return false;
  p += strlen(src);
  // extract sentence id and check if parsed
  const char *snc = tokenOnList(p, sentences_parsed);
  if (snc) {
    strcpy(thisSentence, snc);
    thisCheck += NMEA_HAS_SENTENCE_P + NMEA_HAS_SENTENCE;
  } else { // check if known
    snc = tokenOnList(p, sentences_known);
    if (snc) {
      strcpy(thisSentence, snc);
      thisCheck += NMEA_HAS_SENTENCE;
      return false;
    }
  }
  return true; // passed all the tests
}

/**************************************************************************/
/*!
    @brief Check if a token at the start of a string is on a list.
    @param token Pointer to the string
    @param list A list of strings, with the final entry starting "ZZ"
    @return Pointer to the found token, or NULL if it fails
*/
/**************************************************************************/
const char *Adafruit_GPS::tokenOnList(char *token, const char **list) {
  int i = 0; // index in the list
  while (strncmp(list[i], "ZZ", 2) &&
         i < 1000) { // stop at terminator and don't crash without it
    // test for a match on the sentence name
    if (!strncmp((const char *)list[i], (const char *)token, strlen(list[i])))
      return list[i];
    i++;
  }
  return NULL; // couldn't find a match
}

/**************************************************************************/
/*!
    @brief Parse a string token from pointer p to the next comma, asterisk
    or end of string.
    @param buff Pointer to the buffer to store the string in
    @param p Pointer into a string
    @param n Max permitted size of string including terminating 0
    @return Pointer to the string buffer
*/
/**************************************************************************/
char *Adafruit_GPS::parseStr(char *buff, char *p, int n) {
  char *e = strchr(p, ',');
  int len = 0;
  if (e) {
    len = min(e - p, n - 1);
    strncpy(buff, p, len); // copy up to the comma
    buff[len] = 0;
  } else {
    e = strchr(p, '*');
    if (e) {
      len = min(e - p, n - 1);
      strncpy(buff, p, len); // or up to the *
      buff[e - p] = 0;
    } else {
      len = min((int)strlen(p), n - 1);
      strncpy(buff, p, len); // or to the end or max capacity
    }
  }
  return buff;
}

/**************************************************************************/
/*!
    @brief Is the field empty, or should we try conversion? Won't work
    for a text field that starts with an asterisk or a comma, but that
    probably violates the NMEA-183 standard.
    @param pStart Pointer to the location of the token in the NMEA string
    @return true if empty field, false if something there
*/
/**************************************************************************/
bool Adafruit_GPS::isEmpty(char *pStart) {
  if (',' != *pStart && '*' != *pStart && pStart != NULL)
    return false;
  else
    return true;
}

/**************************************************************************/
/*!
    @brief Add *CS where CS is the two character hex checksum for all but
    the first character in the string. The checksum is the result of an
    exclusive or of all the characters in the string. Also useful if you
    are creating new PMTK strings for controlling a GPS module and need a
    checksum added.
    @param buff Pointer to the string, which must be long enough
    @return none
*/
/**************************************************************************/
void Adafruit_GPS::addChecksum(char *buff) {
  char cs = 0;
  int i = 1;
  while (buff[i]) {
    cs ^= buff[i];
    i++;
  }
  sprintf(buff, "%s*%02X", buff, cs);
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for time
    @param p Pointer to the location of the token in the NMEA string
*/
/**************************************************************************/
void Adafruit_GPS::parseTime(char *p) {
  // get time
  uint32_t time = atol(p);
  hour = time / 10000;
  minute = (time % 10000) / 100;
  seconds = (time % 100);

  p = strchr(p, '.') + 1;
  milliseconds = atoi(p);
  lastTime = sentTime;
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for latitude angle
    @param p Pointer to the location of the token in the NMEA string
*/
/**************************************************************************/
void Adafruit_GPS::parseLat(char *p) {
  int32_t degree;
  long minutes;
  char degreebuff[10];
  if (!isEmpty(p)) {
    strncpy(degreebuff, p, 2);
    p += 2;
    degreebuff[2] = '\0';
    long degree = atol(degreebuff) * 10000000;
    strncpy(degreebuff, p, 2); // minutes
    p += 3;                    // skip decimal point
    strncpy(degreebuff + 2, p, 4);
    degreebuff[6] = '\0';
    long minutes = 50 * atol(degreebuff) / 3;
    latitude_fixed = degree + minutes;
    latitude = degree / 100000 + minutes * 0.000006F;
    latitudeDegrees = (latitude - 100 * int(latitude / 100)) / 60.0;
    latitudeDegrees += int(latitude / 100);
  }
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for latitude direction
    @param p Pointer to the location of the token in the NMEA string
    @return True if we parsed it, false if it has invalid data
*/
/**************************************************************************/
boolean Adafruit_GPS::parseLatDir(char *p) {
  if (p[0] == 'S') {
    lat = 'S';
    latitudeDegrees *= -1.0;
    latitude_fixed *= -1;
  } else if (p[0] == 'N') {
    lat = 'N';
  } else if (p[0] == ',') {
    lat = 0;
  } else {
    return false;
  }
  return true;
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for longitude angle
    @param p Pointer to the location of the token in the NMEA string
*/
/**************************************************************************/
void Adafruit_GPS::parseLon(char *p) {
  int32_t degree;
  long minutes;
  char degreebuff[10];
  if (!isEmpty(p)) {
    strncpy(degreebuff, p, 3);
    p += 3;
    degreebuff[3] = '\0';
    degree = atol(degreebuff) * 10000000;
    strncpy(degreebuff, p, 2); // minutes
    p += 3;                    // skip decimal point
    strncpy(degreebuff + 2, p, 4);
    degreebuff[6] = '\0';
    minutes = 50 * atol(degreebuff) / 3;
    longitude_fixed = degree + minutes;
    longitude = degree / 100000 + minutes * 0.000006F;
    longitudeDegrees = (longitude - 100 * int(longitude / 100)) / 60.0;
    longitudeDegrees += int(longitude / 100);
  }
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for longitude direction
    @param p Pointer to the location of the token in the NMEA string
    @return True if we parsed it, false if it has invalid data
*/
/**************************************************************************/
boolean Adafruit_GPS::parseLonDir(char *p) {
  if (!isEmpty(p)) {
    if (p[0] == 'W') {
      lon = 'W';
      longitudeDegrees *= -1.0;
      longitude_fixed *= -1;
    } else if (p[0] == 'E') {
      lon = 'E';
    } else if (p[0] == ',') {
      lon = 0;
    } else {
      return false;
    }
  }
  return true;
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for whether there is a fix
    @param p Pointer to the location of the token in the NMEA string
    @return True if we parsed it, false if it has invalid data
*/
/**************************************************************************/
boolean Adafruit_GPS::parseFix(char *p) {
  if (p[0] == 'A') {
    fix = true;
    lastFix = sentTime;
  } else if (p[0] == 'V')
    fix = false;
  else
    return false;
  return true;
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last position fix was obtained. Will
    fail by rolling over to zero after one millis() cycle, about 6-1/2 weeks.
    @return float value in seconds since last fix.
*/
/**************************************************************************/
float Adafruit_GPS::secondsSinceFix() { return (millis() - lastFix) / 1000.; }

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS time was obtained. Will fail
    by rolling over to zero after one millis() cycle, about 6-1/2 weeks.
    @return float value in seconds since last GPS time.
*/
/**************************************************************************/
float Adafruit_GPS::secondsSinceTime() { return (millis() - lastTime) / 1000.; }

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS date was obtained. Will fail
    by rolling over to zero after one millis() cycle, about 6-1/2 weeks.
    @return float value in seconds since last GPS date.
*/
/**************************************************************************/
float Adafruit_GPS::secondsSinceDate() { return (millis() - lastDate) / 1000.; }

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
    @brief Read one character from the GPS device
    @return The character that we received, or 0 if nothing was available
*/
/**************************************************************************/
char Adafruit_GPS::read(void) {
  static uint32_t firstChar = 0; // first character received in current sentence
  uint32_t tStart = millis();    // as close as we can get to time char was sent
  char c = 0;

  if (paused)
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
  if (gpsI2C) {
    if (_buff_idx <= _buff_max) {
      c = _i2cbuffer[_buff_idx];
      _buff_idx++;
    } else {
      // refill the buffer!
      if (gpsI2C->requestFrom(0x10, GPS_MAX_I2C_TRANSFER, true) ==
          GPS_MAX_I2C_TRANSFER) {
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
    @brief Initialization code used by all constructor types
*/
/**************************************************************************/
void Adafruit_GPS::common_init(void) {
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  gpsSwSerial = NULL; // Set both to NULL, then override correct
#endif
  gpsHwSerial = NULL; // port pointer in corresponding constructor
  gpsI2C = NULL;
  gpsSPI = NULL;
  recvdflag = false;
  paused = false;
  lineidx = 0;
  currentline = line1;
  lastline = line2;

  hour = minute = seconds = year = month = day = fixquality = fixquality_3d =
      satellites = 0;  // uint8_t
  lat = lon = mag = 0; // char
  fix = false;         // boolean
  milliseconds = 0;    // uint16_t
  latitude = longitude = geoidheight = altitude = speed = angle = magvariation =
      HDOP = VDOP = PDOP = 0.0; // float
}

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
boolean Adafruit_GPS::newNMEAreceived(void) { return recvdflag; }

/**************************************************************************/
/*!
    @brief Pause/unpause receiving new data
    @param p True = pause, false = unpause
*/
/**************************************************************************/
void Adafruit_GPS::pause(boolean p) { paused = p; }

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
    @brief Parse a hex character and return the appropriate decimal value
    @param c Hex character, e.g. '0' or 'B'
    @return Integer value of the hex character. Returns 0 if c is not a proper
   character
*/
/**************************************************************************/
// read a Hex value and return the decimal equivalent
uint8_t Adafruit_GPS::parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A') + 10;
  // if (c > 'F')
  return 0;
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
boolean Adafruit_GPS::waitForSentence(const char *wait4me, uint8_t max,
                                      boolean usingInterrupts) {
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
boolean Adafruit_GPS::LOCUS_StartLogger(void) {
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
boolean Adafruit_GPS::LOCUS_StopLogger(void) {
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
boolean Adafruit_GPS::LOCUS_ReadStatus(void) {
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
boolean Adafruit_GPS::standby(void) {
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
boolean Adafruit_GPS::wakeup(void) {
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
    @brief Checks whether a string starts with a specified prefix
    @param str Pointer to a string
    @param prefix Pointer to the prefix
    @return True if str starts with prefix, false otherwise
*/
/**************************************************************************/
static boolean strStartsWith(const char *str, const char *prefix) {
  while (*prefix) {
    if (*prefix++ != *str++)
      return false;
  }
  return true;
}

#ifdef NMEA_EXTENSIONS
/**************************************************************************/
/*!
    @brief Fakes time of receipt of a sentence. Use between build() and parse()
    to make the timing look like the sentence arrived from the GPS.
*/
/**************************************************************************/
void Adafruit_GPS::resetSentTime() { sentTime = millis(); }

/**************************************************************************/
/*!
    @brief Build an NMEA sentence string based on the relevant variables.
    Sentences start with a $, then a two character source identifier, then
    a three character sentence name that defines the format, then a comma
    and more comma separated fields defined by the sentence name. There are
    many sentences listed that are not yet supported. Most of these sentence
    definitions were found at http://fort21.ru/download/NMEAdescription.pdf

    build() will work with other lengths for source and sentence to allow
    extension to building proprietary sentences like $PMTK220,100*2F.

    build() will not work properly in an environment that does not support
    the %f floating point formatter in sprintf(), and will return NULL.

    build() adds Carriage Return and Line Feed to sentences to conform to
    NMEA-183, so send your output with a print, not a println.

    Some of the data in these test sentences may be arbitrary, e.g. for the
    TXT sentence which has a more complicated protocol for multiple lines
    sent as a message set. Also, the data in the class variables are presumed
    to be valid, so these sentences may contain values that are stale, or
    the result of initialization rather than measurement.

    @param nmea Pointer to the NMEA string buffer. Must be big enough to
                hold the sentence. No guarantee what will be in it if the
                building of the sentence fails.
    @param thisSource Pointer to the source name string (2 upper case)
    @param thisSentence Pointer to the sentence name string (3 upper case)
    @param ref Reference for the sentence, usually relative (R) or true (T)
    @return Pointer to sentence if successful, NULL if fails
*/
/**************************************************************************/
char *Adafruit_GPS::build(char *nmea, const char *thisSource,
                          const char *thisSentence, char ref) {
  sprintf(nmea, "%6.2f", 123.45); // fail if sprintf() doesn't handle floats
  if (strcmp(nmea, "123.45"))
    return NULL;
  *nmea = '$';
  char *p = nmea + 1; // Pointer to move through the sentence
  strncpy(p, thisSource, strlen(thisSource));
  p += strlen(thisSource);
  strncpy(p, thisSentence, strlen(thisSentence));
  p += strlen(thisSentence);
  *p = ',';
  p += 1; // Now $XXSSS, and need to add argument fields
  // This may look inefficient, but an M0 will get down the list in about 1 us /
  // strcmp()! Put the GPS sentences from Adafruit_GPS at the top to make
  // pruning excess code easier. Otherwise, keep them alphabetical for ease of
  // reading.

  if (!strcmp(thisSentence,
              "GGA")) { //********************************************GGA
    // GGA Global Positioning System Fix Data. Time, Position and fix related
    // data for a GPS receiver
    //       1         2       3 4        5 6 7  8   9  10 11 12 13  14  15
    //       |         |       | |        | | |  |   |   | |   | |   |    |
    //$--GGA,hhmmss.ss,ddmm.mm,a,dddmm.mm,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
    // 1) Time (UTC)
    // 2) Latitude
    // 3) N or S (North or South)
    // 4) Longitude
    // 5) E or W (East or West)
    // 6) GPS Quality Indicator, 0 - fix not available, 1 - GPS fix, 2 -
    // Differential GPS fix 7) Number of satellites in view, 00 - 12 8)
    // Horizontal Dilution of precision 9) Antenna Altitude above/below
    // mean-sea-level (geoid) 10) Units of antenna altitude, meters 11) Geoidal
    // separation, the difference between the WGS-84 earth
    //    ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level below
    //    ellipsoid
    // 12) Units of geoidal separation, meters
    // 13) Age of differential GPS data, time in seconds since last SC104
    //    type 1 or 9 update, null field when DGPS is not used
    // 14) Differential reference station ID, 0000-1023
    // 15) Checksum
    sprintf(p, "%09.2f,%09.4f,%c,%010.4f,%c,%d,%02d,%f,%f,M,%f,M,,",
            hour * 10000L + minute * 100L + seconds + milliseconds / 1000.,
            latitude, lat, longitude, lon, fixquality, satellites, HDOP,
            altitude, geoidheight);

  } else if (!strcmp(thisSentence,
                     "GLL")) { //********************************************GLL
    // GLL Geographic Position – Latitude/Longitude
    //       1       2 3        4 5         6 7
    //       |       | |        | |         | |
    //$--GLL,llll.ll,a,yyyyy.yy,a,hhmmss.ss,A*hh
    // 1) Latitude ddmm.mm format
    // 2) N or S (North or South)
    // 3) Longitude dddmm.mm format
    // 4) E or W (East or West)
    // 5) Time (UTC)
    // 6) Status A - Data Valid, V - Data Invalid
    // 7) Checksum
    sprintf(p, "%09.4f,%c,%010.4f,%c,%09.2f,A", latitude, lat, longitude, lon,
            hour * 10000L + minute * 100L + seconds + milliseconds / 1000.);

  } else if (!strcmp(thisSentence,
                     "GSA")) { //********************************************
    // GSA GPS DOP and active satellites
    //       1 2 3                        14 15  16  17 18
    //       | | |                         | |   |   |   |
    //$--GSA,a,a,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x.x,x.x,x.x*hh
    // 1) Selection mode
    // 2) Mode
    // 3) ID of 1st satellite used for fix
    // 4) ID of 2nd satellite used for fix
    // ...
    // 14) ID of 12th satellite used for fix
    // 15) PDOP in meters
    // 16) HDOP in meters
    // 17) VDOP in meters
    // 18) Checksum
    return NULL;

  } else if (!strcmp(thisSentence,
                     "RMC")) { //********************************************RMC
    // RMC Recommended Minimum Navigation Information
    //                                                            12
    //       1         2 3       4 5        6 7   8   9     10  11 |
    //       |         | |       | |        | |   |   |      |   | |
    //$--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxxxx,x.x,a*hh
    // 1) Time (UTC)
    // 2) Status, V = Navigation receiver warning
    // 3) Latitude
    // 4) N or S
    // 5) Longitude
    // 6) E or W
    // 7) Speed over ground, knots
    // 8) Track made good, degrees true
    // 9) Date, ddmmyy
    // 10) Magnetic Variation, degrees
    // 11) E or W
    // 12) Checksum
    sprintf(p, "%09.2f,A,%09.4f,%c,%010.4f,%c,%f,%f,%06d,%f,%c",
            hour * 10000L + minute * 100L + seconds + milliseconds / 1000.,
            latitude, lat, longitude, lon, speed, angle,
            day * 10000 + month * 100 + year, magvariation, mag);

  } else if (!strcmp(thisSentence,
                     "TXT")) { //********************************************TXT
    // as mentioned in https://github.com/adafruit/Adafruit_GPS/issues/95
    // TXT Text Transmission
    //       1  2  3  4    5
    //       |  |  |  |    |
    //$--TXT,xx,xx,xx,c--c*hh
    // 1) Total Number of Sentences 01-99
    // 2) Sentence Number 01-99
    // 3) Text Identifier 01-99
    // 4) Text String, max 61 characters
    // 5) Checksum
    sprintf(p, "01,01,23,This is the text of the sample message");

  } else {
    return NULL; // didn't find a match for the build request
  }

  addChecksum(nmea); // Successful completion
  sprintf(nmea, "%s\r\n",
          nmea); // Add Carriage Return and Line Feed to comply with NMEA-183
  return nmea;   // return pointer to finished product
}

#endif // NMEA_EXTENSIONS

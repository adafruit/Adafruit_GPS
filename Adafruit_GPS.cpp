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

static boolean strStartsWith(const char* str, const char* prefix);

/**************************************************************************/
/*!
    @brief Parse a NMEA string
    @param nmea Pointer to the NMEA string
    @return True if we parsed it, false if it has an invalid checksum or invalid data
*/
/**************************************************************************/
boolean Adafruit_GPS::parse(char *nmea) {
  // do checksum check

  // first look if we even have one
  char *ast = strchr(nmea,'*');
  if (ast != NULL) {
    uint16_t sum = parseHex(*(ast+1)) * 16;
    sum += parseHex(*(ast+2));
    // check checksum
    char *p = strchr(nmea,'$');
    if(p == NULL) return false;
    else{
      for (char *p1 = p+1; p1 < ast; p1++) {
        sum ^= *p1;
      }
      if (sum != 0) {
        // bad checksum :(
        return false;
      }
    }
  } else {
    return false;
  }
  // look for a few common sentences
  char *p = nmea;

  if (strStartsWith(nmea, "$GPGGA") || strStartsWith(nmea, "$GNGGA")) {
    // found GGA
    // get time
    p = strchr(p, ',')+1;
    parseTime(p);

    // parse out latitude
    p = strchr(p, ',')+1;
    parseLat(p);
    p = strchr(p, ',')+1;
    if(!parseLatDir(p)) return false;

    // parse out longitude
    p = strchr(p, ',')+1;
    parseLon(p);
    p = strchr(p, ',')+1;
    if(!parseLonDir(p)) return false;

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      fixquality = atoi(p);
      if(fixquality > 0){
        fix = true;
        lastFix = sentTime;
      } else
        fix = false;
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      satellites = atoi(p);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      HDOP = atof(p);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      altitude = atof(p);
    }

    p = strchr(p, ',')+1;
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      geoidheight = atof(p);
    }
    return true;
  }

  if (strStartsWith(nmea, "$GPRMC") || strStartsWith(nmea, "$GNRMC")) {
    // found RMC
    // get time
    p = strchr(p, ',')+1;
    parseTime(p);

    // fix or no fix
    p = strchr(p, ',')+1;
    if(!parseFix(p)) return false;

    // parse out latitude
    p = strchr(p, ',')+1;
    parseLat(p);
    p = strchr(p, ',')+1;
    if(!parseLatDir(p)) return false;

    // parse out longitude
    p = strchr(p, ',')+1;
    parseLon(p);
    p = strchr(p, ',')+1;
    if(!parseLonDir(p)) return false;

    // speed
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      speed = atof(p);
    }

    // angle
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      angle = atof(p);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      uint32_t fulldate = atof(p);
      day = fulldate / 10000;
      month = (fulldate % 10000) / 100;
      year = (fulldate % 100);
      lastDate = sentTime;
    }
    return true;
  }

  if (strStartsWith(nmea, "$GPGLL") || strStartsWith(nmea, "$GNGLL")) {
    // found GLL
    // parse out latitude
    p = strchr(p, ',')+1;
    parseLat(p);
    p = strchr(p, ',')+1;
    if(!parseLatDir(p)) return false;

    // parse out longitude
    p = strchr(p, ',')+1;
    parseLon(p);
    p = strchr(p, ',')+1;
    if(!parseLonDir(p)) return false;

    // get time
    p = strchr(p, ',')+1;
    parseTime(p);

    // fix or no fix
    p = strchr(p, ',')+1;
    if(!parseFix(p)) return false;

    return true;
  }

if (strStartsWith(nmea, "$GPGSA")) {
  // found GSA
  // parse out Auto selection, but ignore them
  p = strchr(p, ',')+1;
  // parse out 3d fixquality
  p = strchr(p, ',')+1;
  if (',' != *p)
  {
    fixquality_3d = atoi(p);
  }
  // parse out Satellite PDNs, but ignore them
  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  p = strchr(p, ',')+1;

  //parse out PDOP
  p = strchr(p, ',')+1;
  if (',' != *p)
  {
    PDOP = atof(p);
  }
  // parse out HDOP, we also parse this from the GGA sentence. Chipset should report the same for both
  p = strchr(p, ',')+1;
  if (',' != *p)
  {
    HDOP = atof(p);
  }
  // parse out VDOP
  p = strchr(p, ',')+1;
  if (',' != *p)
  {
    VDOP = atof(p);
  }
  return true;
}


  // we dont parse the remaining, yet!
  return false;
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

    p = strchr(p, '.')+1;
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
    if (',' != *p)
    {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      long degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      long minutes = 50 * atol(degreebuff) / 3;
      latitude_fixed = degree + minutes;
      latitude = degree / 100000 + minutes * 0.000006F;
      latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
      latitudeDegrees += int(latitude/100);
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
    if (',' != *p)
    {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      longitude_fixed = degree + minutes;
      longitude = degree / 100000 + minutes * 0.000006F;
      longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
      longitudeDegrees += int(longitude/100);
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
    if (',' != *p)
    {
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
    if (p[0] == 'A'){
      fix = true;
      lastFix = sentTime;
      }
    else if (p[0] == 'V')
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
float Adafruit_GPS::secondsSinceFix() {
    return (millis()-lastFix) / 1000.;
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS time was obtained. Will fail
    by rolling over to zero after one millis() cycle, about 6-1/2 weeks.
    @return float value in seconds since last GPS time.
*/
/**************************************************************************/
float Adafruit_GPS::secondsSinceTime() {
    return (millis()-lastTime) / 1000.;
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS date was obtained. Will fail
    by rolling over to zero after one millis() cycle, about 6-1/2 weeks.
    @return float value in seconds since last GPS date.
*/
/**************************************************************************/
float Adafruit_GPS::secondsSinceDate() {
    return (millis()-lastDate) / 1000.;
}

/**************************************************************************/
/*!
    @brief How many bytes are available to read - part of 'Print'-class functionality
    @return Bytes available, 0 if none
*/
/**************************************************************************/
size_t Adafruit_GPS::available(void) {
  if (paused) return 0;

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  if (gpsSwSerial) {
    return gpsSwSerial->available();
  }
#endif
  if (gpsHwSerial) {
    return gpsHwSerial->available();
  }
  if (gpsI2C || gpsSPI) {
    return 1; // I2C/SPI doesnt have 'availability' so always has a byte at least to read!
  }
  return 0;
}

/**************************************************************************/
/*!
    @brief Write a byte to the underlying transport - part of 'Print'-class functionality
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
  static uint32_t firstChar = 0;   // first character received in current sentence
  uint32_t tStart = millis();      // as close as we can get to time char was sent
  char c = 0;

  if (paused) return c;

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  if(gpsSwSerial) {
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
      if (gpsI2C->requestFrom(0x10, GPS_MAX_I2C_TRANSFER, true) == GPS_MAX_I2C_TRANSFER) {
	// got data!
	_buff_max = 0;
	char curr_char = 0;
	for (int i=0; i<GPS_MAX_I2C_TRANSFER; i++) {
	  curr_char = gpsI2C->read();
	  if ((curr_char == 0x0A) && (last_char != 0x0D)) {
	    // skip duplicate 0x0A's - but keep as part of a CRLF
	    continue;
	  }
	  last_char = curr_char;
	  _i2cbuffer[_buff_max] = curr_char;
	  _buff_max++;
	}
	_buff_max--;  // back up to the last valid slot
	if ((_buff_max == 0) && (_i2cbuffer[0] == 0x0A)) {
	  _buff_max = -1;  // ahh there was nothing to read after all
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
    } while (((c == 0x0A) && (last_char != 0x0D)) || (!isprint(c) && !isspace(c)) );
    last_char = c;
  }
  //Serial.print(c);

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH-1;      // ensure there is someplace to put the next received character

  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    //Serial.println("----");
    //Serial.println((char *)lastline);
    //Serial.println("----");
    lineidx = 0;
    recvdflag = true;
    recvdTime = millis();   // time we got the end of the string
    sentTime = firstChar;
    firstChar = 0;          // there are no characters yet
    return c;               // wait until next character to set time
  }

  if(firstChar == 0) firstChar = tStart;
  return c;
}

/**************************************************************************/
/*!
    @brief Constructor when using SoftwareSerial
    @param ser Pointer to SoftwareSerial device
*/
/**************************************************************************/
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
Adafruit_GPS::Adafruit_GPS(SoftwareSerial *ser)
{
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
  common_init();  // Set everything to common state, then...
  gpsHwSerial = ser; // ...override gpsHwSerial with value passed.
}

/**************************************************************************/
/*!
    @brief Constructor when using I2C
    @param theWire Pointer to an I2C TwoWire object
*/
/**************************************************************************/
Adafruit_GPS::Adafruit_GPS(TwoWire *theWire) {
  common_init();  // Set everything to common state, then...
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
  common_init();  // Set everything to common state, then...
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
  gpsI2C      = NULL;
  gpsSPI      = NULL;
  recvdflag   = false;
  paused      = false;
  lineidx     = 0;
  currentline = line1;
  lastline    = line2;

  hour = minute = seconds = year = month = day =
    fixquality = fixquality_3d = satellites = 0; // uint8_t
  lat = lon = mag = 0; // char
  fix = false; // boolean
  milliseconds = 0; // uint16_t
  latitude = longitude = geoidheight = altitude =
    speed = angle = magvariation = HDOP = VDOP = PDOP = 0.0; // float
}

/**************************************************************************/
/*!
    @brief Start the HW or SW serial port
    @param baud_or_i2caddr Baud rate if using serial, I2C address if using I2C
    @returns True on successful hardware init, False on failure
*/
/**************************************************************************/
bool Adafruit_GPS::begin(uint32_t baud_or_i2caddr)
{
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  if(gpsSwSerial) {
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
    return (gpsI2C->endTransmission () == 0);
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
void Adafruit_GPS::sendCommand(const char *str) {
  println(str);
}

/**************************************************************************/
/*!
    @brief Check to see if a new NMEA line has been received
    @return True if received, false if not
*/
/**************************************************************************/
boolean Adafruit_GPS::newNMEAreceived(void) {
  return recvdflag;
}

/**************************************************************************/
/*!
    @brief Pause/unpause receiving new data
    @param p True = pause, false = unpause
*/
/**************************************************************************/
void Adafruit_GPS::pause(boolean p) {
  paused = p;
}

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
    @return Integer value of the hex character. Returns 0 if c is not a proper character
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
       return (c - 'A')+10;
    // if (c > 'F')
    return 0;
}

/**************************************************************************/
/*!
    @brief Wait for a specified sentence from the device
    @param wait4me Pointer to a string holding the desired response
    @param max How long to wait, default is MAXWAITSENTENCE
    @param usingInterrupts True if using interrupts to read from the GPS (default is false)
    @return True if we got what we wanted, false otherwise
*/
/**************************************************************************/
boolean Adafruit_GPS::waitForSentence(const char *wait4me, uint8_t max, boolean usingInterrupts) {
  uint8_t i=0;
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

  if (! waitForSentence("$PMTKLOG"))
    return false;

  char *response = lastNMEA();
  uint16_t parsed[10];
  uint8_t i;

  for (i=0; i<10; i++) parsed[i] = -1;

  response = strchr(response, ',');
  for (i=0; i<10; i++) {
    if (!response || (response[0] == 0) || (response[0] == '*'))
      break;
    response++;
    parsed[i]=0;
    while ((response[0] != ',') &&
       (response[0] != '*') && (response[0] != 0)) {
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
    return false;  // Returns false if already in standby mode, so that you do not wake it up by sending commands to GPS
  }
  else {
    inStandbyMode = true;
    sendCommand(PMTK_STANDBY);
    //return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast enough to catch the message, or something else just is not working
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
    sendCommand("");  // send byte to wake it up
    return waitForSentence(PMTK_AWAKE);
  }
  else {
      return false;  // Returns false if not in standby mode, nothing to wakeup
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
static boolean strStartsWith(const char* str, const char* prefix)
{
  while (*prefix) {
    if (*prefix++ != *str++)
      return false;
  }
  return true;
}

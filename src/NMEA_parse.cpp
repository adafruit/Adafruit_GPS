/**************************************************************************/
/*!
  @file NMEA_parse.cpp

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


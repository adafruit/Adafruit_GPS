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
    @brief Parse a standard NMEA string and update the relevant variables.
   Sentences start with a $, then a two character source identifier, then a
   three character sentence identifier that defines the format, then a comma and
   more comma separated fields defined by the sentence name. There are many
   sentences listed that are not yet supported, including proprietary sentences
   that start with P, like the $PMTK commands to the GPS modules. See the
   build() function and http://fort21.ru/download/NMEAdescription.pdf for
   sentence descriptions.

   Encapsulated data sentences are supported by NMEA-183, and start with !
   instead of $. https://gpsd.gitlab.io/gpsd/AIVDM.html provides details
   about encapsulated data sentences used in AIS.

    parse() permits, but does not require Carriage Return and Line Feed at the
   end of sentences. The end of the sentence is recognized by the * for the
   checksum. parse() will not recognize a sentence without a valid checksum.

   NMEA_EXTENSIONS must be defined in order to parse more than basic
   GPS module sentences.

    @param nmea Pointer to the NMEA string
    @return True if successfully parsed, false if fails check or parsing
*/
/**************************************************************************/
bool Adafruit_GPS::parse(char *nmea) {
  if (!check(nmea))
    return false;
  // passed the check, so there's a valid source in thisSource and a valid
  // sentence in thisSentence
  char *p = nmea; // Pointer to move through the sentence -- good parsers are
                  // non-destructive
  p = strchr(p, ',') + 1; // Skip to char after the next comma, then check.

  // This may look inefficient, but an M0 will get down the list in about 1 us /
  // strcmp()! Put the GPS sentences from Adafruit_GPS at the top to make
  // pruning excess code easier. Otherwise, keep them alphabetical for ease of
  // reading.
  if (!strcmp(thisSentence, "GGA")) { //************************************GGA
    // Adafruit from Actisense NGW-1 from SH CP150C
    parseTime(p);
    p = strchr(p, ',') + 1; // parse time with specialized function
    // parse out both latitude and direction, then go to next field, or fail
    if (parseCoord(p, &latitudeDegrees, &latitude, &latitude_fixed, &lat))
      newDataValue(NMEA_LAT, latitudeDegrees);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction, then go to next field, or fail
    if (parseCoord(p, &longitudeDegrees, &longitude, &longitude_fixed, &lon))
      newDataValue(NMEA_LON, longitudeDegrees);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) { // if it's a , (or a * at end of sentence) the value is
                       // not included
      fixquality = atoi(p); // needs additional processing
      if (fixquality > 0) {
        fix = true;
        lastFix = sentTime;
      } else
        fix = false;
    }
    p = strchr(p, ',') + 1; // then move on to the next
    // Most can just be parsed with atoi() or atof(), then move on to the next.
    if (!isEmpty(p))
      satellites = atoi(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_HDOP, HDOP = atof(p));
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      altitude = atof(p);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1; // skip the units
    if (!isEmpty(p))
      geoidheight = atof(p); // skip the rest

  } else if (!strcmp(thisSentence, "RMC")) { //*****************************RMC
    // in Adafruit from Actisense NGW-1 from SH CP150C
    parseTime(p);
    p = strchr(p, ',') + 1;
    parseFix(p);
    p = strchr(p, ',') + 1;
    // parse out both latitude and direction, then go to next field, or fail
    if (parseCoord(p, &latitudeDegrees, &latitude, &latitude_fixed, &lat))
      newDataValue(NMEA_LAT, latitudeDegrees);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction, then go to next field, or fail
    if (parseCoord(p, &longitudeDegrees, &longitude, &longitude_fixed, &lon))
      newDataValue(NMEA_LON, longitudeDegrees);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_SOG, speed = atof(p));
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_COG, angle = atof(p));
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      uint32_t fulldate = atof(p);
      day = fulldate / 10000;
      month = (fulldate % 10000) / 100;
      year = (fulldate % 100);
      lastDate = sentTime;
    } // skip the rest

  } else if (!strcmp(thisSentence, "GLL")) { //*****************************GLL
    // in Adafruit from Actisense NGW-1 from SH CP150C
    // parse out both latitude and direction, then go to next field, or fail
    if (parseCoord(p, &latitudeDegrees, &latitude, &latitude_fixed, &lat))
      newDataValue(NMEA_LAT, latitudeDegrees);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction, then go to next field, or fail
    if (parseCoord(p, &longitudeDegrees, &longitude, &longitude_fixed, &lon))
      newDataValue(NMEA_LON, longitudeDegrees);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    parseTime(p);
    p = strchr(p, ',') + 1;
    parseFix(p); // skip the rest

  } else if (!strcmp(thisSentence, "GSA")) { //*****************************GSA
    // in Adafruit from Actisense NGW-1
    p = strchr(p, ',') + 1; // skip selection mode
    if (!isEmpty(p))
      fixquality_3d = atoi(p);
    p = strchr(p, ',') + 1;
    // skip 12 Satellite PDNs without interpreting them
    for (int i = 0; i < 12; i++)
      p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      PDOP = atof(p);
    p = strchr(p, ',') + 1;
    // parse out HDOP, we also parse this from the GGA sentence. Chipset should
    // report the same for both
    if (!isEmpty(p))
      newDataValue(NMEA_HDOP, HDOP = atof(p));
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      VDOP = atof(p); // last before checksum

  }
#ifdef NMEA_EXTENSIONS // Sentences not required for basic GPS functionality
  else if (!strcmp(thisSentence, "APB")) { //*******************************APB
    // from Actisense NGW-1 from SH CP150C
    return false;

  } else if (!strcmp(thisSentence, "DBT")) { //*****************************DBT
    // from Actisense NGW-1
    // feet, metres, fathoms below transducer coerced to water depth from
    // surface in metres
    if (!isEmpty(p))
      newDataValue(NMEA_DEPTH,
                   (nmea_float_t)atof(p) * 0.3048f + depthToTransducer);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_DEPTH, (nmea_float_t)atof(p) + depthToTransducer);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_DEPTH,
                   (nmea_float_t)atof(p) * 6 * 0.3048f + depthToTransducer);

  } else if (!strcmp(thisSentence, "DPT")) { //*****************************DPT
    // from Actisense NGW-1
    return false;

  } else if (!strcmp(thisSentence, "GSV")) { //*****************************GSV
    // from Actisense NGW-1
    return false;

  } else if (!strcmp(thisSentence, "HDG")) { //*****************************HDG
    // from Actisense NGW-1 from SH CP150C
    return false;

  } else if (!strcmp(thisSentence, "HDM")) { //*****************************HDM
    if (!isEmpty(p))
      newDataValue(NMEA_HDG, atof(p)); // skip the rest

  } else if (!strcmp(thisSentence, "HDT")) { //*****************************HDT
    if (!isEmpty(p))
      newDataValue(NMEA_HDT, atof(p)); // skip the rest

  } else if (!strcmp(thisSentence, "MDA")) { //*****************************MDA
    // from Actisense NGW-1
    if (!isEmpty(p))
      newDataValue(NMEA_BAROMETER, atof(p) * 3386.39);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_BAROMETER, atof(p) * 100000);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    nmea_float_t T = 100000.;
    char u = 'C';
    if (!isEmpty(p))
      T = atof(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      u = *p;
    p = strchr(p, ',') + 1;
    if (u != 'C') {
      T = (T - 32) / 1.8f;
      u = 'C';
    } // coerce to C
    if (T < 1000)
      newDataValue(NMEA_TEMPERATURE_AIR, T);
    T = 100000.;
    u = 'C';
    if (!isEmpty(p))
      T = atof(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      u = *p;
    p = strchr(p, ',') + 1;
    if (u != 'C') {
      T = (T - 32) / 1.8f;
      u = 'C';
    }
    if (T < 1000)
      newDataValue(NMEA_TEMPERATURE_WATER, T);
    if (!isEmpty(p))
      newDataValue(NMEA_HUMIDITY, atof(p)); // skip the rest

  } else if (!strcmp(thisSentence, "MTW")) { //*****************************MTW
    nmea_float_t T = 100000.;
    char u = 'C';
    if (!isEmpty(p))
      T = atof(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      u = *p; // last before checksum
    if (u != 'C') {
      T = (T - 32) / 1.8f;
      u = 'C';
    }
    if (T < 1000)
      newDataValue(NMEA_TEMPERATURE_WATER, T);

  } else if (!strcmp(thisSentence, "MWD")) { //*****************************MWD
    // from Actisense NGW-1
    return false;

  } else if (!strcmp(thisSentence, "MWV")) { //*****************************MWV
    // from Actisense NGW-1
    nmea_float_t ang = 100000.;
    char ref = 'T';
    if (!isEmpty(p))
      ang = atof(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      ref = *p;
    p = strchr(p, ',') + 1;
    nmea_float_t spd = 100000.;
    if (!isEmpty(p))
      spd = atof(p);
    p = strchr(p, ',') + 1;
    char units = 'N';
    if (!isEmpty(p))
      units = *p;
    p = strchr(p, ',') + 1;
    char stat = 'A';
    if (!isEmpty(p))
      stat = *p; // last before checksum
    if (units == 'K') {
      spd /= 1.6f;
      units = 'M';
    }
    if (units == 'M') {
      spd *= 5280.0f / 6000.0f;
      units = 'N';
    }
    if (ang > 180.0f)
      ang -= 360.0f;
    if (ref == 'R') {
      if (ang < 1000.0f && stat == 'A')
        newDataValue(NMEA_AWA, ang);
      if (spd < 1000.0f && stat == 'A')
        newDataValue(NMEA_AWS, spd);
    } else {
      if (ang < 1000.0f && stat == 'A')
        newDataValue(NMEA_TWA, ang);
      if (spd < 1000.0f && stat == 'A')
        newDataValue(NMEA_TWS, spd);
    }

  } else if (!strcmp(thisSentence, "RMB")) { //*****************************RMB
    // from Actisense NGW-1 from SH CP150C
    // RMB Recommended Minimum Navigation Information
    //       1 2   3 4    5    6       7 8        9 10  11 12  13 14
    //       | |   | |    |    |       | |        | |   |   |   | |
    //$--RMB,A,x.x,a,c--c,c--c,llll.ll,a,yyyyy.yy,a,x.x,x.x,x.x,A*hh
    // 1) Status, V = Navigation receiver warning
    // 2) Cross Track error - nautical miles
    // 3) Direction to Steer, Left or Right
    // 4) TO Waypoint ID
    // 5) FROM Waypoint ID
    // 6) Destination Waypoint Latitude 7) N or S
    // 8) Destination Waypoint Longitude 9) E or W
    // 10) Range to destination in nautical miles
    // 11) Bearing to destination in degrees True
    // 12) Destination closing velocity in knots
    // 13) Arrival Status, A = Arrival Circle Entered 14) Checksum
    p = strchr(p, ',') + 1; // skip status
    nmea_float_t xte = 100000.;
    char xteDir = 'X';
    if (!isEmpty(p))
      xte = atof(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      xteDir = *p;
    p = strchr(p, ',') + 1;
    if (xte < 10000.0f && xteDir != 'X') {
      if (xteDir == 'L')
        xte *= -1.0f;
      newDataValue(NMEA_XTE, xte);
    }
    if (!isEmpty(p))
      parseStr(toID, p, NMEA_MAX_WP_ID);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      parseStr(fromID, p, NMEA_MAX_WP_ID);
    p = strchr(p, ',') + 1;
    nmea_float_t latitudeWP = 0;
    nmea_float_t longitudeWP = 0;
    int32_t latitude_fixedWP = 0;
    int32_t longitude_fixedWP = 0;
    nmea_float_t latitudeDegreesWP = 0;
    nmea_float_t longitudeDegreesWP = 0;
    char latWP = 'X';
    char lonWP = 'X';

    // parse out both latitude and direction for WayPoint, then go to next
    // field, or fail
    if (!isEmpty(p)) {
      if (!parseCoord(p, &latitudeDegreesWP, &latitudeWP, &latitude_fixedWP,
                      &latWP))
        return false;
      else
        newDataValue(NMEA_LATWP, latitudeDegreesWP);
    }
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction for WayPoint, then go to next
    // field, or fail
    if (!isEmpty(p)) {
      if (!parseCoord(p, &longitudeDegreesWP, &longitudeWP, &longitude_fixedWP,
                      &lonWP))
        return false;
      else
        newDataValue(NMEA_LONWP, longitudeDegreesWP);
    }
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_DISTWP, atof(p));
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_COGWP, atof(p));
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_VMGWP, atof(p)); // skip arrival flag

  } else if (!strcmp(thisSentence, "ROT")) { //*****************************ROT
    return false;

  } else if (!strcmp(thisSentence, "RPM")) { //*****************************RPM
    return false;

  } else if (!strcmp(thisSentence, "RSA")) { //*****************************RSA
    // from Actisense NGW-1
    return false;

  } else if (!strcmp(thisSentence, "TXT")) { //*****************************TXT
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

  } else if (!strcmp(thisSentence, "VDR")) { //*****************************VDR
    // from Actisense NGW-1
    return false;

  } else if (!strcmp(thisSentence, "VHW")) { //*****************************VHW
    // from Actisense NGW-1
    if (!isEmpty(p))
      newDataValue(NMEA_HDT, atof(p));
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_HDG, atof(p));
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_VTW, atof(p)); // skip the other units

  } else if (!strcmp(thisSentence, "VLW")) { //*****************************VLW
    // from Actisense NGW-1
    if (!isEmpty(p))
      newDataValue(NMEA_LOG, atof(p));
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_LOGR, atof(p)); // skip the other units

  } else if (!strcmp(thisSentence, "VPW")) { //*****************************VPW
    // knots, metres/s coerced to knots
    nmea_float_t vmg = 100000.;
    if (!isEmpty(p))
      vmg = atof(p);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      vmg = atof(p) * 0.3048 * 3600. / 6000.; // skip units
    if (vmg < 1000.0f)
      newDataValue(NMEA_VMG, vmg);
  } else if (!strcmp(thisSentence, "VTG")) { //*****************************VTG
    // from Actisense NGW-1 from SH CP150C
    return false;

  } else if (!strcmp(thisSentence, "VWR")) { //*****************************VWR
    // from Actisense NGW-1
    nmea_float_t ang = 1000.;
    if (!isEmpty(p))
      ang = atof(p);
    p = strchr(p, ',') + 1;
    char ref = ' ';
    if (!isEmpty(p))
      ref = *p;
    p = strchr(p, ',') + 1;
    if (ref == 'L')
      ang *= -1;
    if (ang < 1000.0f)
      newDataValue(NMEA_AWA, ang);
    nmea_float_t ws = 0.0;
    char units = 'X';
    if (!isEmpty(p))
      ws = atof(p);
    p = strchr(p, ',') + 1; // knots
    if (!isEmpty(p))
      units = *p;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      ws = atof(p);
    p = strchr(p, ',') + 1; // meters / second
    if (!isEmpty(p))
      units = *p;
    p = strchr(p, ',') + 1; // M
    if (!isEmpty(p))
      ws = atof(p);
    p = strchr(p, ',') + 1; // kilometers / hour can be converted back to knots
    if (!isEmpty(p))
      units = *p; // last before checksum
    if (units == 'M') {
      ws *= 3.6f;
      units = 'K';
    } // convert m/s to km/h
    if (units == 'K') {
      ws /= 1.6f;
      units = 'M';
    } // convert km/h to miles / h
    if (units == 'M') {
      ws *= 5280.0f / 6000.0f;
      units = 'N';
    } // convert miles / hr to knots
    if (units == 'N')
      newDataValue(NMEA_AWS, ws); // store the final result

  } else if (!strcmp(thisSentence, "WCV")) { //*****************************WCV
    // from SH CP150C
    if (!isEmpty(p))
      newDataValue(NMEA_VMGWP, atof(p)); // skip the rest

  } else if (!strcmp(thisSentence, "XTE")) { //*****************************XTE
    // from Actisense NGW-1 from SH CP150C
    p = strchr(p, ',') + 1; // skip status 1
    p = strchr(p, ',') + 1; // skip status 2
    nmea_float_t xte = 100000.;
    char xteDir = 'X';
    if (!isEmpty(p))
      xte = atof(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      xteDir = *p;
    p = strchr(p, ',') + 1;
    if (xte < 10000.0f && xteDir != 'X') {
      if (xteDir == 'L')
        xte *= -1.0f;
      newDataValue(NMEA_XTE, xte);
    } // skip units

  } else if (!strcmp(thisSentence, "ZDA")) { //*****************************ZDA
    // from Actisense NGW-1
    return false;
  }
#endif // NMEA_EXTENSIONS

  else {
    return false; // didn't find the required sentence definition
  }

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
bool Adafruit_GPS::check(char *nmea) {
  thisCheck = 0; // new check
  *thisSentence = *thisSource = 0;
  if (*nmea != '$' && *nmea != '!')
    return false; // doesn't start with $ or !
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
      return false; // known but not parsed
    } else {
      parseStr(thisSentence, p, NMEA_MAX_SENTENCE_ID);
      return false; // unknown
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
    @brief Check if an NMEA string is valid and is on a list, perhaps to
    decide if it should be passed to a particular NMEA device.
    @param nmea Pointer to the NMEA string
    @param list A list of strings, with the final entry "ZZ"
    @return True if on the list, false if it fails check or is not on the list
*/
/**************************************************************************/
bool Adafruit_GPS::onList(char *nmea, const char **list) {
  if (!check(nmea)) // sets thisSentence if valid
    return false;   // not a valid sentence
  // stop at terminator with first two letters ZZ and don't crash without it
  for (int i = 0; strncmp(list[i], "ZZ", 2) && i < 1000; i++) {
    // test for a match on the sentence name
    if (!strcmp((const char *)list[i], (const char *)thisSentence))
      return true;
  }
  return false; // couldn't find a match
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for lat or lon angle and direction.
    Works for either DDMM.mmmm,N (latitude) or DDDMM.mmmm,W (longitude) format.
    Insensitive to number of decimal places present. Only fills the variables
    if it succeeds and the variable pointer is not NULL. This allows calling
    to fill only the variables of interest. Does rudimentary validation on
    angle range.

    Supersedes private functions parseLat(), parseLon(), parseLatDir(),
    parseLonDir(), all previously called from parse().
    @param pStart Pointer to the location of the token in the NMEA string
    @param angle Pointer to the angle to fill with value in degrees/minutes as
      received from the GPS (DDDMM.MMMM), unsigned
    @param angle_fixed Pointer to the fix point version latitude in decimal
      degrees * 10000000, signed
    @param angleDegrees Pointer to the angle to fill with decimal degrees,
      signed. As actual double on SAMD, etc. resolution is better than the
      fixed point version.
    @param dir Pointer to character to fill the direction N/S/E/W
    @return true if successful, false if failed or no value
*/
/**************************************************************************/
bool Adafruit_GPS::parseCoord(char *pStart, nmea_float_t *angleDegrees,
                              nmea_float_t *angle, int32_t *angle_fixed,
                              char *dir) {
  char *p = pStart;
  if (!isEmpty(p)) {
    // get the number in DDDMM.mmmm format and break into components
    char degreebuff[10];
    char *e = strchr(p, '.');
    if (e == NULL || e - p > 6)
      return false;                // no decimal point in range
    strncpy(degreebuff, p, e - p); // get DDDMM
    long dddmm = atol(degreebuff);
    long degrees = (dddmm / 100);         // truncate the minutes
    long minutes = dddmm - degrees * 100; // remove the degrees
    p = e;                                // start from the decimal point
    nmea_float_t decminutes = atof(e); // the fraction after the decimal point
    p = strchr(p, ',') + 1;            // go to the next field

    // get the NSEW direction as a character
    char nsew = 'X';
    if (!isEmpty(p))
      nsew = *p; // field is not empty
    else
      return false; // no direction provided

    // set the various numerical formats to their values
    long fixed = degrees * 10000000 + (minutes * 10000000) / 60 +
                 (decminutes * 10000000) / 60;
    nmea_float_t ang = degrees * 100 + minutes + decminutes;
    nmea_float_t deg = fixed / (nmea_float_t)10000000.;
    if (nsew == 'S' ||
        nsew == 'W') { // fixed and deg are signed, but DDDMM.mmmm is not
      fixed = -fixed;
      deg = -deg;
    }

    // reject directions that are not NSEW
    if (nsew != 'N' && nsew != 'S' && nsew != 'E' && nsew != 'W')
      return false;

    // reject angles that are out of range
    if (nsew == 'N' || nsew == 'S')
      if (abs(deg) > 90)
        return false;
    if (abs(deg) > 180)
      return false;

    // store in locations passed as args
    if (angle != NULL)
      *angle = ang;
    if (angle_fixed != NULL)
      *angle_fixed = fixed;
    if (angleDegrees != NULL)
      *angleDegrees = deg;
    if (dir != NULL)
      *dir = nsew;
  } else
    return false; // no number
  return true;
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
    @brief Parse a part of an NMEA string for time. Independent of number
    of decimal places after the '.'
    @param p Pointer to the location of the token in the NMEA string
    @return true if successful, false otherwise
*/
/**************************************************************************/
bool Adafruit_GPS::parseTime(char *p) {
  if (!isEmpty(p)) { // get time
    uint32_t time = atol(p);
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);
    char *dec = strchr(p, '.');
    char *comstar = min(strchr(p, ','), strchr(p, '*'));
    if (dec != NULL && comstar != NULL && dec < comstar)
      milliseconds = atof(p) * 1000;
    else
      milliseconds = 0;
    lastTime = sentTime;
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for whether there is a fix
    @param p Pointer to the location of the token in the NMEA string
    @return True if we parsed it, false if it has invalid data
*/
/**************************************************************************/
bool Adafruit_GPS::parseFix(char *p) {
  if (!isEmpty(p)) {
    if (p[0] == 'A') {
      fix = true;
      lastFix = sentTime;
    } else if (p[0] == 'V')
      fix = false;
    else
      return false;
    return true;
  }
  return false;
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

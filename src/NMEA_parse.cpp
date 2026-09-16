/**************************************************************************/
/*!
  @file NMEA_parse.cpp

  This is the Adafruit GPS library - the ultimate GPS library
  for the ultimate GPS module!

  Tested and works great with the Adafruit Ultimate GPS module
  using MTK33x9 chipset
  ------> http://www.adafruit.com/products/746

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  @author Limor Fried/Ladyada for Adafruit Industries.

  @copyright BSD license, check license.txt for more information
  All text above must be included in any redistribution
*/
/**************************************************************************/

#include <Adafruit_GNSS.h>
#include <Adafruit_GPS.h>

const char PROGMEM Adafruit_GPS::sources[][4] = {"II", "WI", "GP", "PG", "GL",
                                                 "GA", "GN", "HC", "TI", "SD",
                                                 "AI", "P",  "ZZZ"};
#ifdef NMEA_EXTENSIONS
const char PROGMEM Adafruit_GPS::sentences_parsed[][4] = {
    "GGA", "GLL", "GSA", "RMC", "CD",  "DBT", "HDM", "HDT", "MDA", "MTW", "MWV",
    "RMB", "TOP", "TXT", "VHW", "VLW", "VPW", "VWR", "WCV", "XTE", "ZZZ"};
const char PROGMEM Adafruit_GPS::sentences_known[][4] = {
    "APB", "DPT", "GSV", "HDG", "MWD", "ROT",
    "RPM", "RSA", "VDR", "VTG", "ZDA", "ZZZ"};
#else // make the lists short to save flash on small boards
const char PROGMEM Adafruit_GPS::sentences_parsed[][4] = {
    "GGA", "GLL", "GSA", "RMC", "TOP", "CD", "ZZZ"};
const char PROGMEM Adafruit_GPS::sentences_known[][4] = {"DBT", "HDM", "HDT",
                                                         "ZZZ"};
#endif
#include <ctype.h>

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
  // Count fields before parsing so a truncated sentence cannot change GPS data
  // or make a comma lookup run past the end of the sentence.
  size_t fields = 0;
  for (char *p = nmea; *p && *p != '*'; p++) {
    if (*p == ',')
      fields++;
  }
  // All current sentence handlers require fields. Revisit this guard if a
  // zero-field sentence is added; the comma lookup below also assumes a field.
  if (fields == 0)
    return false;
  // passed the check, so there's a valid source in thisSource and a valid
  // sentence in thisSentence
  char *p = nmea; // Pointer to move through the sentence -- good parsers are
                  // non-destructive
  p = strchr(p, ',') + 1; // Skip to char after the next comma, then check.

  // Validate every field consumed by a standard navigation decoder before
  // changing any fix data. The enclosing frame has already passed check().
  nmea_span_t type = {thisSentence, strlen(thisSentence)};
  nmea_span_t dataFields = {p, (size_t)(strchr(p, '*') - p)};
  gnss_sentence_status_t status = updatePosition(type, dataFields);
  if (status == GNSS_SENTENCE_UNSUPPORTED)
    status = Adafruit_GNSS::validateNavigation(type, dataFields).status;
  if (status != GNSS_SENTENCE_VALID && status != GNSS_SENTENCE_UNSUPPORTED)
    return false;

  // This may look inefficient, but an M0 will get down the list in about 1 us /
  // strcmp()! Put the GPS sentences from Adafruit_GPS at the top to make
  // pruning excess code easier. Otherwise, keep them alphabetical for ease of
  // reading.
  if (!strcmp(thisSentence, "GGA")) { //************************************GGA
    // Position, time, and fix are already decoded by the shared core.
    for (uint8_t i = 0; i < 6; i++)
      p = strchr(p, ',') + 1;
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
    for (uint8_t i = 0; i < 6; i++)
      p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_SOG, speed = atof(p));
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_COG, angle = atof(p));

  } else if (!strcmp(thisSentence, "GLL")) { //*****************************GLL
    // All GLL fields are handled by the shared position decoder.

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

  } else if (!strcmp(thisSentence, "TOP") ||
             !strcmp_P(thisSentence, PSTR("CD"))) { // Antenna status
    if (fields < 2)
      return false;
    bool cdtop = !strcmp_P(thisSentence, PSTR("CD"));
    // Only PCD subtype 11 describes the antenna; other CD messages differ.
    if (cdtop && strncmp_P(nmea, PSTR("$PCD,11,"), 8))
      return false;
    p = strchr(p, ',') + 1;
    if (!parseAntenna(p))
      return false;
    if (cdtop) {
      // PCD uses 1=internal, 2=external, 3=shorted. Preserve the public
      // PGTOP convention: 1=problem, 2=internal, 3=external.
      if (antenna == 1)
        antenna = 2;
      else if (antenna == 2)
        antenna = 3;
      else
        antenna = 1;
    }
  }

#ifdef NMEA_EXTENSIONS // Sentences not required for basic GPS functionality
  else if (!strcmp(thisSentence, "APB")) { //*******************************APB
    // from Actisense NGW-1 from SH CP150C
    return false;

  } else if (!strcmp(thisSentence, "DBT")) { //*****************************DBT
    if (fields < 5)
      return false;
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
    if (fields < 9)
      return false;
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
    if (fields < 2)
      return false;
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
    if (fields < 5)
      return false;
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
    if (fields < 12)
      return false;
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
    char *toIDField = p;
    p = strchr(p, ',') + 1;
    char *fromIDField = p;
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
    }
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction for WayPoint, then go to next
    // field, or fail
    if (!isEmpty(p)) {
      if (!parseCoord(p, &longitudeDegreesWP, &longitudeWP, &longitude_fixedWP,
                      &lonWP))
        return false;
    }

    // Any supplied coordinates passed validation. Save the cross-track error,
    // waypoint IDs, and coordinates now; a rejected sentence must leave the
    // previous data and history intact.
    if (xte < 10000.0f && xteDir != 'X') {
      if (xteDir == 'L')
        xte *= -1.0f;
      newDataValue(NMEA_XTE, xte);
    }
    if (!isEmpty(toIDField))
      parseStr(toID, toIDField, NMEA_MAX_WP_ID);
    if (!isEmpty(fromIDField))
      parseStr(fromID, fromIDField, NMEA_MAX_WP_ID);
    if (latWP != 'X')
      newDataValue(NMEA_LATWP, latitudeDegreesWP);
    if (lonWP != 'X')
      newDataValue(NMEA_LONWP, longitudeDegreesWP);
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
    if (fields < 4)
      return false;
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
    if (fields < 5)
      return false;
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
    if (fields < 3)
      return false;
    // from Actisense NGW-1
    if (!isEmpty(p))
      newDataValue(NMEA_LOG, atof(p));
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      newDataValue(NMEA_LOGR, atof(p)); // skip the other units

  } else if (!strcmp(thisSentence, "VPW")) { //*****************************VPW
    if (fields < 3)
      return false;
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
    if (fields < 8)
      return false;
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
    if (fields < 5)
      return false;
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
    @brief Validate NMEA framing and checksum, then recognize its source and
    complete sentence ID. Update thisCheck, thisSource and thisSentence.
    @param nmea Pointer to the NMEA string
    @return True if well formed, false if it has problems
*/
/**************************************************************************/
bool Adafruit_GPS::check(char *nmea) {
  thisCheck = 0; // new check
  *thisSentence = *thisSource = 0;
  if (!nmea || (*nmea != '$' && *nmea != '!'))
    return false; // doesn't start with $ or !
  else
    thisCheck += NMEA_HAS_DOLLAR;
  // Share framing and checksum rules with the byte receiver. Recognition
  // remains here so unknown proprietary replies are still valid NMEA frames.
  nmea_sentence_t sentence = Adafruit_NMEA::validate(nmea, strlen(nmea));
  if (sentence.status != NMEA_FRAME_VALID)
    return false;
  thisCheck += NMEA_HAS_CHECKSUM;
  // extract source of variable length
  char *p = nmea + 1;
  const char *src = tokenOnList(p, sources);
  if (src) {
    strcpy_P(thisSource, src);
    thisCheck += NMEA_HAS_SOURCE;
  } else
    return false;
  p += strlen_P(src);
  size_t sentenceLength = sentence.address.length - strlen_P(src);
  // extract sentence id and check if parsed
  const char *snc = tokenOnList(p, sentences_parsed);
  if (snc && strlen_P(snc) == sentenceLength) {
    strcpy_P(thisSentence, snc);
    thisCheck += NMEA_HAS_SENTENCE_P + NMEA_HAS_SENTENCE;
  } else { // check if known
    snc = tokenOnList(p, sentences_known);
    if (snc && strlen_P(snc) == sentenceLength) {
      strcpy_P(thisSentence, snc);
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
    @param list A table in program memory, with the final entry starting "ZZ"
    @return Program-memory pointer to the found token, or NULL if it fails
*/
/**************************************************************************/
const char *Adafruit_GPS::tokenOnList(char *token, const char list[][4]) {
  for (int i = 0; i < 1000; i++) {
    if (pgm_read_byte(list[i]) == 'Z' && pgm_read_byte(list[i] + 1) == 'Z')
      break; // stop at terminator
    // test for a match on the sentence name
    if (!strncmp_P(token, list[i], strlen_P(list[i])))
      return list[i];
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
    to fill only the variables of interest. Validates the complete coordinate
    and hemisphere fields, including minute and degree limits.

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
  if (!pStart)
    return false;
  // Bound just the coordinate and hemisphere, stopping before the next field.
  // Character comparisons avoid placing a delimiter string in AVR RAM.
  char *end = pStart;
  uint8_t commas = 0;
  while (*end && *end != '*') {
    if (*end == ',' && ++commas == 2)
      break;
    end++;
  }
  nmea_span_t remaining = {pStart, (size_t)(end - pStart)};
  nmea_span_t value = Adafruit_NMEA::nextField(remaining);
  nmea_span_t hemisphere = Adafruit_NMEA::nextField(remaining);
  gnss_coordinate_t coordinate =
      Adafruit_GNSS::parseCoordinate(value, hemisphere);
  if (coordinate.status != NMEA_NUMBER_VALID)
    return false;

  setCoordinate(coordinate, angleDegrees, angle, angle_fixed, dir);
  return true;
}

/**************************************************************************/
/*!
    @brief Convert exact components into the legacy coordinate representations.
    @param coordinate Validated coordinate components.
    @param angleDegrees Optional signed decimal degrees output.
    @param angle Optional unsigned degrees/minutes output.
    @param angle_fixed Optional signed E7 output.
    @param dir Optional hemisphere output.
*/
/**************************************************************************/
void Adafruit_GPS::setCoordinate(const gnss_coordinate_t &coordinate,
                                 nmea_float_t *angleDegrees,
                                 nmea_float_t *angle, int32_t *angle_fixed,
                                 char *dir) {
  // Derive convenience floats only after exact fixed-point conversion. Keep
  // their available precision instead of rebuilding them from the E7 value.
  nmea_float_t minutes = coordinate.minutes + coordinate.fractionalMinutes /
                                                  (nmea_float_t)1000000000.0;
  nmea_float_t degrees = coordinate.degrees + minutes / 60;
  if (coordinate.hemisphere == 'S' || coordinate.hemisphere == 'W')
    degrees = -degrees;
  if (angle)
    *angle = coordinate.degrees * 100 + minutes;
  if (angle_fixed)
    *angle_fixed = coordinate.degreesE7;
  if (angleDegrees)
    *angleDegrees = degrees;
  if (dir)
    *dir = coordinate.hemisphere;
}

/**************************************************************************/
/*!
    @brief Apply one validated shared position result to legacy GPS state.
    @param type Sentence type without its talker prefix.
    @param fields Bounded fields from the already checked frame.
    @return Shared validation status; rejected sentences leave data unchanged.

    Empty fields preserve previous values and timestamps. GGA quality and
    RMC/GLL validity remain independent, and only a positive fix refreshes
    lastFix. Exact components are converted only at the legacy float boundary.
*/
/**************************************************************************/
gnss_sentence_status_t Adafruit_GPS::updatePosition(nmea_span_t type,
                                                    nmea_span_t fields) {
  gnss_position_t position = Adafruit_GNSS::parsePosition(type, fields);
  if (position.validation.status != GNSS_SENTENCE_VALID)
    return position.validation.status;
  if (position.latitude.status == NMEA_NUMBER_VALID) {
    setCoordinate(position.latitude, &latitudeDegrees, &latitude,
                  &latitude_fixed, &lat);
    newDataValue(NMEA_LAT, latitudeDegrees);
  }
  if (position.longitude.status == NMEA_NUMBER_VALID) {
    setCoordinate(position.longitude, &longitudeDegrees, &longitude,
                  &longitude_fixed, &lon);
    newDataValue(NMEA_LON, longitudeDegrees);
  }
  if (position.time.status == NMEA_NUMBER_VALID) {
    hour = position.time.hour;
    minute = position.time.minute;
    seconds = position.time.second;
    milliseconds = position.time.millisecond;
    lastTime = sentTime;
  }
  if (position.date.status == NMEA_NUMBER_VALID) {
    day = position.date.day;
    month = position.date.month;
    year = position.date.year;
    lastDate = sentTime;
  }
  if (position.fixStatus == NMEA_NUMBER_VALID) {
    fix = position.fix;
    if (fix)
      lastFix = sentTime;
  }
  if (position.fixQualityStatus == NMEA_NUMBER_VALID)
    fixquality = position.fixQuality;
  return GNSS_SENTENCE_VALID;
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
  if (!e) {
    e = strchr(p, '*');
  }
  int len;
  if (e) {
    len = min((int)(e - p), n - 1);
  } else {
    len = min((int)strlen(p), n - 1);
  }
  memcpy(buff, p, len);
  buff[len] = 0;
  return buff;
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for antenna that is used
    @param p Pointer to the location of the token in the NMEA string
    @return True if the antenna status was recognized, false otherwise
*/
/**************************************************************************/
bool Adafruit_GPS::parseAntenna(char *p) {
  if (!isEmpty(p) && p[0] != '\0' &&
      (p[1] == ',' || p[1] == '*' || p[1] == '\0')) {
    if (p[0] == '3') {
      antenna = 3;
    } else if (p[0] == '2') {
      antenna = 2;
    } else if (p[0] == '1') {
      antenna = 1;
    } else
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
  if (pStart != NULL && ',' != *pStart && '*' != *pStart)
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

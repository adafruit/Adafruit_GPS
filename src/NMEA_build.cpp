/**************************************************************************/
/*!
  @file NMEA_build.cpp

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

#ifdef NMEA_EXTENSIONS
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
            (double)latitude, lat, (double)longitude, lon, fixquality,
            satellites, (double)HDOP, (double)altitude, (double)geoidheight);

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
    sprintf(p, "%09.4f,%c,%010.4f,%c,%09.2f,A", (double)latitude, lat,
            (double)longitude, lon,
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
            (double)latitude, lat, (double)longitude, lon, (double)speed,
            (double)angle, day * 10000 + month * 100 + year,
            (double)magvariation, mag);

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

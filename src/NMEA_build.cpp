/**************************************************************************/
/*!
  @file NMEA_build.cpp

  This is the Adafruit GPS library - the ultimate GPS library
  for the ultimate GPS module!

  Tested and works great with the Adafruit Ultimate GPS module
  using MTK33x9 chipset
      ------> http://www.adafruit.com/products/746
  Pick one up today at the Adafruit electronics shop
  and help support open source hardware & software! -ada

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  @author Limor Fried/Ladyada  for Adafruit Industries.

  @copyright BSD license, check license.txt for more information
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
    Floating point arguments to sprintf() are explicitly cast to double to
    avoid warnings in some compilers.

    build() adds Carriage Return and Line Feed to sentences to conform to
    NMEA-183, so send your output with a print, not a println.

    The resulting sentence may be corrupted if the input data is corrupt.
    In particular, the sentence will be truncated if any of the character
    data is 0, e.g. if lat is not set to 'N' or 'S'.

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
    @param noCRLF set true to disable adding CR/LF to comply with NMEA-183
    @return Pointer to sentence if successful, NULL if fails
*/
/**************************************************************************/
char *Adafruit_GPS::build(char *nmea, const char *thisSource,
                          const char *thisSentence, char ref, bool noCRLF) {
  sprintf(nmea, "%6.2f",
          (double)123.45); // fail if sprintf() doesn't handle floats
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

  if (!strcmp(thisSentence, "GGA")) { //************************************GGA
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
            (double)hour * 10000L + minute * 100L + seconds +
                milliseconds / 1000.,
            (double)latitude, lat, (double)longitude, lon, fixquality,
            satellites, (double)HDOP, (double)altitude, (double)geoidheight);

  } else if (!strcmp(thisSentence, "GLL")) { //*****************************GLL
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
            (double)hour * 10000L + minute * 100L + seconds +
                milliseconds / 1000.);

  } else if (!strcmp(thisSentence, "GSA")) { //*****************************GSA
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

  } else if (!strcmp(thisSentence, "RMC")) { //*****************************RMC
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
            (double)hour * 10000L + minute * 100L + seconds +
                milliseconds / 1000.,
            (double)latitude, lat, (double)longitude, lon, (double)speed,
            (double)angle, day * 10000 + month * 100 + year,
            (double)magvariation, mag);

  } else if (!strcmp(thisSentence, "APB")) { //*****************************APB
    // APB Autopilot Sentence "B"
    //                                       13    15
    //       1 2 3   4 5 6 7 8   9 10   11 12 |  14 |
    //       | | |   | | | | |   | |    |   | |   | |
    //$--APB,A,A,x.x,a,N,A,A,x.x,a,c--c,x.x,a,x.x,a*hh
    // 1) Status
    //    V = LORAN-C Blink or SNR warning
    //    A = general warning flag or other navigation systems when a reliable
    //    fix is not available
    // 2) Status
    //    V = Loran-C Cycle Lock warning flag
    //    A = OK or not used
    // 3) Cross Track Error Magnitude
    // 4) Direction to steer, L or R
    // 5) Cross Track Units, N = Nautical Miles
    // 6) Status
    //    A = Arrival Circle Entered
    // 7) Status
    //    A = Perpendicular passed at waypoint
    // 8) Bearing origin to destination
    // 9) M = Magnetic, T = True
    // 10) Destination Waypoint ID
    // 11) Bearing, present position to Destination
    // 12) M = Magnetic, T = True
    // 13) Heading to steer to destination waypoint
    // 14) M = Magnetic, T = True
    // 15) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "DBK")) { //*****************************DBT
    // DBK Depth Below Keel
    //       1   2 3   4 5   6 7
    //       |   | |   | |   | |
    //$--DBK,x.x,f,x.x,M,x.x,F*hh
    // 1) Depth, feet
    // 2) f = feet
    // 3) Depth, meters
    // 4) M = meters
    // 5) Depth, Fathoms
    // 6) F = Fathoms
    // 7) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "DBS")) { //*****************************DBT
    // DBS Depth Below Surface
    //       1   2 3   4 5   6 7
    //       |   | |   | |   | |
    //$--DBS,x.x,f,x.x,M,x.x,F*hh
    // 1) Depth, feet
    // 2) f = feet
    // 3) Depth, meters
    // 4) M = meters
    // 5) Depth, Fathoms
    // 6) F = Fathoms
    // 7) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "DBT")) { //*****************************DBT
    // DBT Depth Below Transducer
    //       1   2 3   4 5   6 7
    //       |   | |   | |   | |
    //$--DBT,x.x,f,x.x,M,x.x,F*hh
    // 1) Depth, feet
    // 2) f = feet
    // 3) Depth, meters
    // 4) M = meters
    // 5) Depth, Fathoms
    // 6) F = Fathoms
    // 7) Checksum
    double d = val[NMEA_DEPTH].latest - depthToTransducer;
    sprintf(p, "%f,f,%f,M,,,", d / 0.3048, d);

  } else if (!strcmp(thisSentence, "DPT")) { //*****************************DPT
    // DPT Heading – Deviation & Variation
    //       1   2   3
    //       |   |   |
    //$--DPT,x.x,x.x*hh
    // 1) Depth, meters
    // 2) Offset from transducer;
    //      positive means distance from transducer to water line,
    //      negative means distance from transducer to keel
    // 3) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "GSV")) { //*****************************GSV
    // GSV Satellites in view
    //       1 2 3 4 5 6 7     n
    //       | | | | | | |     |
    //$--GSV,x,x,x,x,x,x,x,...*hh
    // 1) total number of messages
    // 2) message number
    // 3) satellites in view
    // 4) satellite number
    // 5) elevation in degrees
    // 6) azimuth in degrees to true
    // 7) SNR in dB
    // more satellite infos like 4)-7)
    // n) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "HDG")) { //*****************************HDG
    //  HDG Heading – Deviation & Variation
    //       1   2   3 4   5 6
    //       |   |   | |   | |
    //$--HDG,x.x,x.x,a,x.x,a*hh
    // 1) Magnetic Sensor heading in degrees
    // 2) Magnetic Deviation, degrees
    // 3) Magnetic Deviation direction, E = Easterly, W = Westerly
    // 4) Magnetic Variation degrees
    // 5) Magnetic Variation direction, E = Easterly, W = Westerly
    // 6) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "HDM")) { //*****************************HDM
    // HDM Heading – Magnetic
    //       1   2 3
    //       |   | |
    //$--HDM,x.x,M*hh
    // 1) Heading Degrees, magnetic
    // 2) M = magnetic
    // 3) Checksum
    sprintf(p, "%f,M", (double)val[NMEA_HDG].latest);

  } else if (!strcmp(thisSentence, "HDT")) { //*****************************HDT
    // HDT Heading – True
    //       1   2 3
    //       |   | |
    //$--HDT,x.x,T*hh
    // 1) Heading Degrees, true
    // 2) T = True
    // 3) Checksum
    // starts with $II for integrated instrumentation
    sprintf(p, "%f,T", (double)val[NMEA_HDT].latest);

  } else if (!strcmp(thisSentence, "MDA")) { //*****************************MDA
    // MDA Meteorological Composite
    //       1   2 3   4 5   6 7   8 9 10 11  12
    //       |   | |   | |   | |   | |  |  |   |
    //$__MDA,x.x,I,x.x,B,x.x,C,x.x,C,x.x, ,x.x,C,,T,,M,,N,,M*hh
    //$IIMDA,,I,,B,,C,21.8,C,,,,C,,T,,M,,N,,M*0F     // sent by RayMarine i70s
    // Speed/Depth/Wind
    // 1) Barometric Pressure
    // 2) inches of Hg
    // 3) Barometric Pressure
    // 4) bar
    // 5) Atmospheric Temperature
    // 6) C or F
    // 7) Water Temperature
    // 8) C or F
    // 9) Relative Humidity
    // 10)
    // 11) Dew Point
    // 12) C or F
    return NULL;

  } else if (!strcmp(thisSentence, "MTW")) { //*****************************MTW
    // MTW Water Temperature
    //       1   2 3
    //       |   | |
    //$IIMTW,x.x,C*hh
    //$IIMTW,21.8,C*18     // sent by RayMarine i70s Speed/Depth/Wind
    // 1) Degrees
    // 2) Unit of Measurement, Celcius
    // 3) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "MWD")) { //*****************************MWD
    // MWD Wind Direction & Speed
    // Format unknown
    return NULL;

  } else if (!strcmp(thisSentence, "MWV")) { //*****************************MWV
    // MWV Wind Speed and Angle assuming values for True
    //       1   2 3   4 5 6
    //       |   | |   | | |
    //$IIMWV,x.x,a,x.x,a,a*hh
    //$WIMWV,276.94,R,0,N,A*03      // sent by RayMarine i70s Speed/Depth/Wind
    // 1) Wind Angle, 0 to 360 degrees
    // 2) Reference, R = Relative, T = True
    // 3) Wind Speed
    // 4) Wind Speed Units, K/M/N  kilometers/miles/knots
    // 5) Status, A = Data Valid
    // 6) Checksum
    if (ref == 'R')
      sprintf(p, "%f,%c,%f,N,A", (double)val[NMEA_AWA].latest, ref,
              (double)val[NMEA_AWS].latest);
    else
      sprintf(p, "%f,%c,%f,N,A", (double)val[NMEA_TWA].latest, 'T',
              (double)val[NMEA_TWS].latest);

  } else if (!strcmp(thisSentence, "RMB")) { //*****************************RMB
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
    sprintf(p, ",,,,,,,,,,,%f,A", (double)val[NMEA_VMGWP].latest);

  } else if (!strcmp(thisSentence, "ROT")) { //*****************************ROT
    // ROT Rate Of Turn
    //       1   2 3
    //       |   | |
    //$--ROT,x.x,A*hh
    // 1) Rate Of Turn, degrees per minute, "-" means bow turns to port
    // 2) Status, A means data is valid
    // 3) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "RPM")) { //*****************************RPM
    // RPM Revolutions
    //       1 2 3   4   5 6
    //       | | |   |   | |
    //$--RPM,a,x,x.x,x.x,A*hh
    // 1) Source; S = Shaft, E = Engine
    // 2) Engine or shaft number
    // 3) Speed, Revolutions per minute
    // 4) Propeller pitch, % of maximum, "-" means astern
    // 5) Status, A means data is valid
    // 6) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "RSA")) { //*****************************RSA
    //  RSA Rudder Sensor Angle
    //       1   2 3   4 5
    //       |   | |   | |
    //$--RSA,x.x,A,x.x,A*hh
    // 1) Starboard (or single) rudder sensor, "-" means Turn To Port
    // 2) Status, A means data is valid
    // 3) Port rudder sensor
    // 4) Status, A means data is valid
    // 5) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "TXT")) { //*****************************TXT
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

  } else if (!strcmp(thisSentence, "VDR")) { //*****************************VDR
    // VDR Set and Drift
    //       1   2 3   4 5   6 7
    //       |   | |   | |   | |
    //$--VDR,x.x,T,x.x,M,x.x,N*hh
    // 1) Degress True
    // 2) T = True
    // 3) Degrees Magnetic
    // 4) M = Magnetic
    // 5) Knots (speed of current)
    // 6) N = Knots
    // 7) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "VHW")) { //*****************************VHW
    // VHW Water Speed and Heading
    //       1   2 3   4 5   6 7   8 9
    //       |   | |   | |   | |   | |
    //$--VHW,x.x,T,x.x,M,x.x,N,x.x,K*hh
    //$IIVHW,,T,,M,0,N,0,K*55     // sent by RayMarine i70s Speed/Depth/Wind
    // 1) Degrees True
    // 2) T = True
    // 3) Degrees Magnetic
    // 4) M = Magnetic
    // 5) Knots (speed of vessel relative to the water) [66]
    // 6) N = Knots
    // 7) Kilometers (speed of vessel relative to the water)
    // 8) K = Kilometres
    // 9) Checksum
    sprintf(p, "%f,T,%f,M,%f,N,%f,K", (double)val[NMEA_HDT].latest,
            (double)val[NMEA_HDG].latest, (double)val[NMEA_VTW].latest,
            (double)val[NMEA_VTW].latest * 1.829);

  } else if (!strcmp(thisSentence, "VLW")) { //*****************************VLW
    // VLW Distance Traveled through Water
    //       1   2 3   4 5
    //       |   | |   | |
    //$--VLW,x.x,N,x.x,N*hh
    //$IIVLW,0,N,0,N,,N,,N*4D     // sent by RayMarine i70s Speed/Depth/Wind
    // not sure what the last two are?
    // 1) Total cumulative distance
    // 2) N = Nautical Miles
    // 3) Distance since Reset
    // 4) N = Nautical Miles
    // 5) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "VPW")) { //*****************************VPW
    // not supported by iNavX
    // VPW Speed – Measured Parallel to Wind
    //       1   2 3   4 5
    //       |   | |   | |
    //$--VPW,x.x,N,x.x,M*hh
    // 1) Speed, "-" means downwind
    // 2) N = Knots
    // 3) Speed, "-" means downwind
    // 4) M = Meters per second
    // 5) Checksum
    sprintf(p, "%f,N,,", (double)val[NMEA_VMG].latest);

  } else if (!strcmp(thisSentence, "VTG")) { //*****************************VTG
    // VTG Track Made Good and Ground Speed
    //       1   2 3   4 5   6 7   8 9
    //       |   | |   | |   | |   | |
    //$--VTG,x.x,T,x.x,M,x.x,N,x.x,K*hh
    // 1) Track Degrees               2) T = True
    // 3) Track Degrees               4) M = Magnetic
    // 5) Speed Knots                 6) N = Knots
    // 7) Speed Kilometers Per Hour   8) K = Kilometres Per Hour
    // 9) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "VWR")) { //*****************************VWR
    // VWR Relative Wind Speed and Angle
    //       1   2 3   4 5   6 7   8 9
    //       |   | |   | |   | |   | |
    //$--VWR,x.x,a,x.x,N,x.x,M,x.x,K*hh
    //$WIVWR,83.1,L,0,N,0,M,0,K*6D     // sent by RayMarine i70s
    // Speed/Depth/Wind
    // 1) Wind direction magnitude in degrees
    // 2) Wind direction Left/Right of bow
    // 3) Speed
    // 4) N = Knots
    // 5) Speed
    // 6) M = Meters Per Second
    // 7) Speed
    // 8) K = Kilometers Per Hour
    // 9) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "WCV")) { //*****************************WCV
    // WCV Waypoint Closure Velocity
    //       1   2 3    4
    //       |   | |    |
    //$--WCV,x.x,N,c--c*hh
    // 1) Velocity 2) N = knots 3) Waypoint ID 4) Checksum
    sprintf(p, "%f,N,home", (double)val[NMEA_VMG].latest);

  } else if (!strcmp(thisSentence, "XTE")) { //*****************************XTE
    // XTE Cross-Track Error – Measured
    //       1 2 3   4 5  6
    //       | | |   | |  |
    //$--XTE,A,A,x.x,a,N,*hh
    // 1) Status
    //    V = LORAN-C blink or SNR warning
    //    A = general warning flag or other navigation systems when a reliable
    //    fix is not available
    // 2) Status
    //    V = Loran-C cycle lock warning flag
    //    A = OK or not used
    // 3) Cross track error magnitude
    // 4) Direction to steer, L or R
    // 5) Cross track units. N = Nautical Miles
    // 6) Checksum
    return NULL;

  } else if (!strcmp(thisSentence, "ZDA")) { //*****************************ZDA
    // ZDA Time & Date – UTC, Day, Month, Year and Local Time Zone
    //       1         2  3  4    5  6  7
    //       |         |  |  |    |  |  |
    //$--ZDA,hhmmss.ss,xx,xx,xxxx,xx,xx*hh
    // 1) Local zone minutes description, same sign as local hours
    // 2) Local zone description, 00 to +/- 13 hours
    // 3) Year
    // 4) Month, 01 to 12
    // 5) Day, 01 to 31
    // 6) Time (UTC)
    // 7) Checksum
    return NULL;

  } else {
    return NULL; // didn't find a match for the build request
  }

  addChecksum(nmea); // Successful completion
  if (!noCRLF) { // Add Carriage Return and Line Feed to comply with NMEA-183
    sprintf(nmea, "%s\r\n", nmea);
  }
  return nmea; // return pointer to finished product
}

#endif // NMEA_EXTENSIONS

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

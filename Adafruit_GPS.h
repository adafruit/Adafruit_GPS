/**************************************************************************/
/*!
  @file Adafruit_GPS.h

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

  Written by Limor Fried/Ladyada  for Adafruit Industries.
  BSD license, check license.txt for more information
  All text above must be included in any redistribution
*/
/**************************************************************************/

// Fllybob added lines 34,35 and 40,41 to add 100mHz logging capability

#ifndef _ADAFRUIT_GPS_H
#define _ADAFRUIT_GPS_H

/**************************************************************************/
/**
 Comment out the definition of NMEA_EXTENSIONS to make the library use as
 little memory as possible for GPS functionality only. The ARDUINO_ARCH_AVR
 test should leave it out of any compilations for the UNO and similar. */
#ifndef ARDUINO_ARCH_AVR
#define NMEA_EXTENSIONS ///< if defined will include more NMEA sentences
#endif

#define USE_SW_SERIAL ///< comment this out if you don't want to include
                      ///< software serial in the library
#define GPS_DEFAULT_I2C_ADDR                                                   \
  0x10 ///< The default address for I2C transport of GPS data
#define GPS_MAX_I2C_TRANSFER                                                   \
  32 ///< The max number of bytes we'll try to read at once
#define GPS_MAX_SPI_TRANSFER                                                   \
  100                     ///< The max number of bytes we'll try to read at once
#define MAXLINELENGTH 120 ///< how long are max NMEA lines to parse?
#define NMEA_MAX_SENTENCE_ID                                                   \
  20 ///< maximum length of a sentence ID name, including terminating 0
#define NMEA_MAX_SOURCE_ID                                                     \
  3 ///< maximum length of a source ID name, including terminating 0

#include "Arduino.h"
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
#include <SoftwareSerial.h>
#endif
#include <SPI.h>
#include <Wire.h>

/**************************************************************************/
/**
 Different commands to set the update rate from once a second (1 Hz) to 10 times
 a second (10Hz) Note that these only control the rate at which the position is
 echoed, to actually speed up the position fix you must also send one of the
 position fix rate commands below too. */
#define PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ                                    \
  "$PMTK220,10000*2F" ///< Once every 10 seconds, 100 millihertz.
#define PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ                                    \
  "$PMTK220,5000*1B" ///< Once every 5 seconds, 200 millihertz.
#define PMTK_SET_NMEA_UPDATE_1HZ "$PMTK220,1000*1F" ///<  1 Hz
#define PMTK_SET_NMEA_UPDATE_2HZ "$PMTK220,500*2B"  ///<  2 Hz
#define PMTK_SET_NMEA_UPDATE_5HZ "$PMTK220,200*2C"  ///<  5 Hz
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F" ///< 10 Hz
// Position fix update rate commands.
#define PMTK_API_SET_FIX_CTL_100_MILLIHERTZ                                    \
  "$PMTK300,10000,0,0,0,0*2C" ///< Once every 10 seconds, 100 millihertz.
#define PMTK_API_SET_FIX_CTL_200_MILLIHERTZ                                    \
  "$PMTK300,5000,0,0,0,0*18" ///< Once every 5 seconds, 200 millihertz.
#define PMTK_API_SET_FIX_CTL_1HZ "$PMTK300,1000,0,0,0,0*1C" ///< 1 Hz
#define PMTK_API_SET_FIX_CTL_5HZ "$PMTK300,200,0,0,0,0*2F"  ///< 5 Hz
// Can't fix position faster than 5 times a second!

#define PMTK_SET_BAUD_115200 "$PMTK251,115200*1F" ///< 115200 bps
#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C"   ///<  57600 bps
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"     ///<   9600 bps

#define PMTK_SET_NMEA_OUTPUT_GLLONLY                                           \
  "$PMTK314,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on only the
                                                      ///< GPGLL sentence
#define PMTK_SET_NMEA_OUTPUT_RMCONLY                                           \
  "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on only the
                                                      ///< GPRMC sentence
#define PMTK_SET_NMEA_OUTPUT_VTGONLY                                           \
  "$PMTK314,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on only the
                                                      ///< GPVTG
#define PMTK_SET_NMEA_OUTPUT_GGAONLY                                           \
  "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on just the
                                                      ///< GPGGA
#define PMTK_SET_NMEA_OUTPUT_GSAONLY                                           \
  "$PMTK314,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on just the
                                                      ///< GPGSA
#define PMTK_SET_NMEA_OUTPUT_GSVONLY                                           \
  "$PMTK314,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on just the
                                                      ///< GPGSV
#define PMTK_SET_NMEA_OUTPUT_RMCGGA                                            \
  "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28" ///< turn on GPRMC and
                                                      ///< GPGGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGAGSA                                         \
  "$PMTK314,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29" ///< turn on GPRMC, GPGGA
                                                      ///< and GPGSA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA                                           \
  "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28" ///< turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_OFF                                               \
  "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28" ///< turn off output

// to generate your own sentences, check out the MTK command datasheet and use a
// checksum calculator such as the awesome
// http://www.hhhh.org/wiml/proj/nmeaxor.html

#define PMTK_LOCUS_STARTLOG "$PMTK185,0*22" ///< Start logging data
#define PMTK_LOCUS_STOPLOG "$PMTK185,1*23"  ///< Stop logging data
#define PMTK_LOCUS_STARTSTOPACK                                                \
  "$PMTK001,185,3*3C" ///< Acknowledge the start or stop command
#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"  ///< Query the logging status
#define PMTK_LOCUS_ERASE_FLASH "$PMTK184,1*22" ///< Erase the log flash data
#define LOCUS_OVERLAP                                                          \
  0 ///< If flash is full, log will overwrite old data with new logs
#define LOCUS_FULLSTOP 1 ///< If flash is full, logging will stop

#define PMTK_ENABLE_SBAS                                                       \
  "$PMTK313,1*2E" ///< Enable search for SBAS satellite (only works with 1Hz
                  ///< output rate)
#define PMTK_ENABLE_WAAS "$PMTK301,2*2E" ///< Use WAAS for DGPS correction data

#define PMTK_STANDBY                                                           \
  "$PMTK161,0*28" ///< standby command & boot successful message
#define PMTK_STANDBY_SUCCESS "$PMTK001,161,3*36" ///< Not needed currently
#define PMTK_AWAKE "$PMTK010,002*2D"             ///< Wake up

#define PMTK_Q_RELEASE "$PMTK605*31" ///< ask for the release and version

#define PGCMD_ANTENNA                                                          \
  "$PGCMD,33,1*6C" ///< request for updates on antenna status
#define PGCMD_NOANTENNA "$PGCMD,33,0*6D" ///< don't show antenna status messages

#define MAXWAITSENTENCE                                                        \
  10 ///< how long to wait when we're looking for a response
/**************************************************************************/

/// type for resulting code from running check()
typedef enum {
  NMEA_BAD = 0,            ///< passed none of the checks
  NMEA_HAS_DOLLAR = 1,     ///< has a dollar sign in the first position
  NMEA_HAS_CHECKSUM = 2,   ///< has a valid checksum at the end
  NMEA_HAS_NAME = 4,       ///< there is a token after the $ followed by a comma
  NMEA_HAS_SOURCE = 10,    ///< has a recognized source ID
  NMEA_HAS_SENTENCE = 20,  ///< has a recognized sentence ID
  NMEA_HAS_SENTENCE_P = 40 ///< has a recognized parseable sentence ID
} nmea_check_t;

/**************************************************************************/
/*!
    @brief  The GPS class
*/
class Adafruit_GPS : public Print {
public:
  bool begin(uint32_t baud_or_i2caddr);

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  Adafruit_GPS(SoftwareSerial *ser); // Constructor when using SoftwareSerial
#endif
  Adafruit_GPS(HardwareSerial *ser); // Constructor when using HardwareSerial
  Adafruit_GPS(TwoWire *theWire);    // Constructor when using I2C
  Adafruit_GPS(SPIClass *theSPI, int8_t cspin); // Constructor when using SPI

  char *lastNMEA(void);
  boolean newNMEAreceived();
  void common_init(void);

  void sendCommand(const char *);

  void pause(boolean b);

  uint8_t parseHex(char c);

  char read(void);
  size_t write(uint8_t);
  size_t available(void);

  boolean check(char *nmea);
  boolean parse(char *);
  void addChecksum(char *buff);
  float secondsSinceFix();
  float secondsSinceTime();
  float secondsSinceDate();

  boolean wakeup(void);
  boolean standby(void);

  int thisCheck = 0; ///< the results of the check on the current sentence
  char thisSource[NMEA_MAX_SOURCE_ID] = {
      0}; ///< the first two letters of the current sentence, e.g. WI, GP
  char thisSentence[NMEA_MAX_SENTENCE_ID] = {
      0}; ///< the next three letters of the current sentence, e.g. GLL, RMC
  char lastSource[NMEA_MAX_SOURCE_ID] = {
      0}; ///< the results of the check on the most recent successfully parsed
          ///< sentence
  char lastSentence[NMEA_MAX_SENTENCE_ID] = {
      0}; ///< the next three letters of the most recent successfully parsed
          ///< sentence, e.g. GLL, RMC

  uint8_t hour;          ///< GMT hours
  uint8_t minute;        ///< GMT minutes
  uint8_t seconds;       ///< GMT seconds
  uint16_t milliseconds; ///< GMT milliseconds
  uint8_t year;          ///< GMT year
  uint8_t month;         ///< GMT month
  uint8_t day;           ///< GMT day

  float latitude;  ///< Floating point latitude value in degrees/minutes as
                   ///< received from the GPS (DDMM.MMMM)
  float longitude; ///< Floating point longitude value in degrees/minutes as
                   ///< received from the GPS (DDDMM.MMMM)

  /** Fixed point latitude and longitude value with degrees stored in units of
    1/100000 degrees, and minutes stored in units of 1/100000 degrees.  See pull
    #13 for more details:
    https://github.com/adafruit/Adafruit-GPS-Library/pull/13 */
  int32_t latitude_fixed;  ///< Fixed point latitude in decimal degrees
  int32_t longitude_fixed; ///< Fixed point longitude in decimal degrees

  float latitudeDegrees;  ///< Latitude in decimal degrees
  float longitudeDegrees; ///< Longitude in decimal degrees
  float geoidheight;      ///< Diff between geoid height and WGS84 height
  float altitude;         ///< Altitude in meters above MSL
  float speed;            ///< Current speed over ground in knots
  float angle;            ///< Course in degrees from true north
  float magvariation;     ///< Magnetic variation in degrees (vs. true north)
  float HDOP;     ///< Horizontal Dilution of Precision - relative accuracy of
                  ///< horizontal position
  float VDOP;     ///< Vertical Dilution of Precision - relative accuracy of
                  ///< vertical position
  float PDOP;     ///< Position Dilution of Precision - Complex maths derives a
                  ///< simple, single number for each kind of DOP
  char lat = 'X'; ///< N/S
  char lon = 'X'; ///< E/W
  char mag = 'X'; ///< Magnetic variation direction
  boolean fix;    ///< Have a fix?
  uint8_t fixquality;    ///< Fix quality (0, 1, 2 = Invalid, GPS, DGPS)
  uint8_t fixquality_3d; ///< 3D fix quality (1, 3, 3 = Nofix, 2D fix, 3D fix)
  uint8_t satellites;    ///< Number of satellites in use

  boolean waitForSentence(const char *wait, uint8_t max = MAXWAITSENTENCE,
                          boolean usingInterrupts = false);
  boolean LOCUS_StartLogger(void);
  boolean LOCUS_StopLogger(void);
  boolean LOCUS_ReadStatus(void);

  uint16_t LOCUS_serial;  ///< Log serial number
  uint16_t LOCUS_records; ///< Log number of data record
  uint8_t LOCUS_type;     ///< Log type, 0: Overlap, 1: FullStop
  uint8_t LOCUS_mode;     ///< Logging mode, 0x08 interval logger
  uint8_t LOCUS_config;   ///< Contents of configuration
  uint8_t LOCUS_interval; ///< Interval setting
  uint8_t LOCUS_distance; ///< Distance setting
  uint8_t LOCUS_speed;    ///< Speed setting
  uint8_t LOCUS_status;   ///< 0: Logging, 1: Stop logging
  uint8_t LOCUS_percent;  ///< Log life used percentage

#ifdef NMEA_EXTENSIONS
  // NMEA additional public functions
  char *build(char *nmea, const char *thisSource, const char *thisSentence,
              char ref = 'R');
  void resetSentTime();

  // NMEA additional public variables
  char txtTXT[63] = {0}; ///< text content from most recent TXT sentence
  int txtTot = 0;        ///< total TXT sentences in group
  int txtID = 0;         ///< id of the text message
  int txtN = 0;          ///< the TXT sentence number
#endif                   // NMEA_EXTENSIONS

private:
  const char *tokenOnList(char *token, const char **list);
  char *parseStr(char *buff, char *p, int n);
  bool isEmpty(char *pStart);
  void parseTime(char *);
  void parseLat(char *);
  boolean parseLatDir(char *);
  void parseLon(char *);
  boolean parseLonDir(char *);
  boolean parseFix(char *);
  // used by check() for validity tests, room for future expansion
  const char *sources[5] = {"II", "WI", "GP", "GN",
                            "ZZZ"}; ///< valid source ids
  const char *sentences_parsed[5] = {"GGA", "GLL", "GSA", "RMC",
                                     "ZZZ"}; ///< parseable sentence ids
  const char *sentences_known[1] = {
      "ZZZ"}; ///< known, but not parseable sentence ids

  // Make all of these times far in the past by setting them near the middle of
  // the millis() range. Timing assumes that sentences are parsed promptly.
  uint32_t lastUpdate =
      2000000000L; ///< millis() when last full sentence successfully parsed
  uint32_t lastFix = 2000000000L;  ///< millis() when last fix received
  uint32_t lastTime = 2000000000L; ///< millis() when last time received
  uint32_t lastDate = 2000000000L; ///< millis() when last date received
  uint32_t recvdTime =
      2000000000L; ///< millis() when last full sentence received
  uint32_t sentTime = 2000000000L; ///< millis() when first character of last
                                   ///< full sentence received
  boolean paused;

  uint8_t parseResponse(char *response);
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  SoftwareSerial *gpsSwSerial;
#endif
  HardwareSerial *gpsHwSerial;
  TwoWire *gpsI2C;
  SPIClass *gpsSPI;
  int8_t gpsSPI_cs = -1;
  SPISettings gpsSPI_settings =
      SPISettings(1000000, MSBFIRST, SPI_MODE0); // default
  char _spibuffer[GPS_MAX_SPI_TRANSFER]; // for when we write data, we need to
                                         // read it too!
  uint8_t _i2caddr;
  char _i2cbuffer[GPS_MAX_I2C_TRANSFER];
  int8_t _buff_max = -1, _buff_idx = 0;
  char last_char = 0;

  volatile char line1[MAXLINELENGTH]; ///< We double buffer: read one line in
                                      ///< and leave one for the main program
  volatile char line2[MAXLINELENGTH]; ///< Second buffer
  volatile uint8_t lineidx = 0;   ///< our index into filling the current line
  volatile char *currentline;     ///< Pointer to current line buffer
  volatile char *lastline;        ///< Pointer to previous line buffer
  volatile boolean recvdflag;     ///< Received flag
  volatile boolean inStandbyMode; ///< In standby flag
};
/**************************************************************************/

#endif

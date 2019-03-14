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

#define USE_SW_SERIAL ///< comment this out if you don't want to include software serial in the library

#include "Arduino.h"
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  #include <SoftwareSerial.h>
#endif

/**************************************************************************/
/**
 Different commands to set the update rate from once a second (1 Hz) to 10 times a second (10Hz)
 Note that these only control the rate at which the position is echoed, to actually speed up the
 position fix you must also send one of the position fix rate commands below too. */
#define PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ  "$PMTK220,10000*2F"  ///< Once every 10 seconds, 100 millihertz.
#define PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ  "$PMTK220,5000*1B"   ///< Once every 5 seconds, 200 millihertz.
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"              ///<  1 Hz
#define PMTK_SET_NMEA_UPDATE_2HZ  "$PMTK220,500*2B"               ///<  2 Hz
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"               ///<  5 Hz
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"               ///< 10 Hz
// Position fix update rate commands.
#define PMTK_API_SET_FIX_CTL_100_MILLIHERTZ  "$PMTK300,10000,0,0,0,0*2C"  ///< Once every 10 seconds, 100 millihertz.
#define PMTK_API_SET_FIX_CTL_200_MILLIHERTZ  "$PMTK300,5000,0,0,0,0*18"   ///< Once every 5 seconds, 200 millihertz.
#define PMTK_API_SET_FIX_CTL_1HZ  "$PMTK300,1000,0,0,0,0*1C"              ///< 1 Hz
#define PMTK_API_SET_FIX_CTL_5HZ  "$PMTK300,200,0,0,0,0*2F"               ///< 5 Hz
// Can't fix position faster than 5 times a second!

#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C" ///< 57600 bps
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"   ///<  9600 bps

#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"  ///< turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"   ///< turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"  ///< turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"      ///< turn off output

// to generate your own sentences, check out the MTK command datasheet and use a checksum calculator
// such as the awesome http://www.hhhh.org/wiml/proj/nmeaxor.html

#define PMTK_LOCUS_STARTLOG  "$PMTK185,0*22"          ///< Start logging data
#define PMTK_LOCUS_STOPLOG "$PMTK185,1*23"            ///< Stop logging data
#define PMTK_LOCUS_STARTSTOPACK "$PMTK001,185,3*3C"   ///< Acknowledge the start or stop command
#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"         ///< Query the logging status
#define PMTK_LOCUS_ERASE_FLASH "$PMTK184,1*22"        ///< Erase the log flash data
#define LOCUS_OVERLAP 0                               ///< If flash is full, log will overwrite old data with new logs
#define LOCUS_FULLSTOP 1                              ///< If flash is full, logging will stop

#define PMTK_ENABLE_SBAS "$PMTK313,1*2E"              ///< Enable search for SBAS satellite (only works with 1Hz output rate)
#define PMTK_ENABLE_WAAS "$PMTK301,2*2E"              ///< Use WAAS for DGPS correction data

#define PMTK_STANDBY "$PMTK161,0*28"              ///< standby command & boot successful message
#define PMTK_STANDBY_SUCCESS "$PMTK001,161,3*36"  ///< Not needed currently
#define PMTK_AWAKE "$PMTK010,002*2D"              ///< Wake up

#define PMTK_Q_RELEASE "$PMTK605*31"              ///< ask for the release and version


#define PGCMD_ANTENNA "$PGCMD,33,1*6C"            ///< request for updates on antenna status
#define PGCMD_NOANTENNA "$PGCMD,33,0*6D"          ///< don't show antenna status messages

#define MAXWAITSENTENCE 10   ///< how long to wait when we're looking for a response
/**************************************************************************/


/**************************************************************************/
/*!
    @brief  The GPS class
*/
class Adafruit_GPS {
 public:
  void begin(uint32_t baud);

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  Adafruit_GPS(SoftwareSerial *ser); // Constructor when using SoftwareSerial
#endif
  Adafruit_GPS(HardwareSerial *ser); // Constructor when using HardwareSerial

  char *lastNMEA(void);
  boolean newNMEAreceived();
  void common_init(void);

  void sendCommand(const char *);

  void pause(boolean b);

  uint8_t parseHex(char c);

  char read(void);
  boolean parse(char *);

  boolean wakeup(void);
  boolean standby(void);

  uint8_t hour;                                     ///< GMT hours
  uint8_t minute;                                   ///< GMT minutes
  uint8_t seconds;                                  ///< GMT seconds
  uint16_t milliseconds;                            ///< GMT milliseconds
  uint8_t year;                                     ///< GMT year
  uint8_t month;                                    ///< GMT month
  uint8_t day;                                      ///< GMT day

  float latitude;   ///< Floating point latitude value in degrees/minutes as received from the GPS (DDMM.MMMM)
  float longitude;  ///< Floating point longitude value in degrees/minutes as received from the GPS (DDDMM.MMMM)

  /** Fixed point latitude and longitude value with degrees stored in units of 1/100000 degrees,
    and minutes stored in units of 1/100000 degrees.  See pull #13 for more details:
    https://github.com/adafruit/Adafruit-GPS-Library/pull/13 */
  int32_t latitude_fixed;   ///< Fixed point latitude in decimal degrees
  int32_t longitude_fixed;  ///< Fixed point longitude in decimal degrees

  float latitudeDegrees;    ///< Latitude in decimal degrees
  float longitudeDegrees;   ///< Longitude in decimal degrees
  float geoidheight;        ///< Diff between geoid height and WGS84 height
  float altitude;           ///< Altitude in meters above MSL
  float speed;              ///< Current speed over ground in knots
  float angle;              ///< Course in degrees from true north
  float magvariation;       ///< Magnetic variation in degrees (vs. true north)
  float HDOP;               ///< Horizontal Dilution of Precision - relative accuracy of horizontal position
  char lat;                 ///< N/S
  char lon;                 ///< E/W
  char mag;                 ///< Magnetic variation direction
  boolean fix;              ///< Have a fix?
  uint8_t fixquality;       ///< Fix quality (0, 1, 2 = Invalid, GPS, DGPS)
  uint8_t satellites;       ///< Number of satellites in use

  boolean waitForSentence(const char *wait, uint8_t max = MAXWAITSENTENCE);
  boolean LOCUS_StartLogger(void);
  boolean LOCUS_StopLogger(void);
  boolean LOCUS_ReadStatus(void);

  uint16_t LOCUS_serial;    ///< Log serial number
  uint16_t LOCUS_records;   ///< Log number of data record
  uint8_t LOCUS_type;       ///< Log type, 0: Overlap, 1: FullStop
  uint8_t LOCUS_mode;       ///< Logging mode, 0x08 interval logger
  uint8_t LOCUS_config;     ///< Contents of configuration
  uint8_t LOCUS_interval;   ///< Interval setting
  uint8_t LOCUS_distance;   ///< Distance setting
  uint8_t LOCUS_speed;      ///< Speed setting
  uint8_t LOCUS_status;     ///< 0: Logging, 1: Stop logging
  uint8_t LOCUS_percent;    ///< Log life used percentage

 private:
  boolean paused;

  uint8_t parseResponse(char *response);
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
  SoftwareSerial *gpsSwSerial;
#endif
  HardwareSerial *gpsHwSerial;
};
/**************************************************************************/

#endif

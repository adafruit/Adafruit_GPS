/***********************************************************************
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

Written by Limor Fried/Ladyada for Adafruit Industries.
Modified by Biagio Montaruli <biagio.hkr@gmail.com>
BSD license, check license.txt for more information
All text above must be included in any redistribution
************************************************************************/
// Fllybob added lines 34,35 and 40,41 to add 100mHz logging capability 

#ifndef _ADAFRUIT_GPS_H
#define _ADAFRUIT_GPS_H

//comment this out if you don't want to include software serial in the library
#define USE_SW_SERIAL

#if defined(__AVR__) && defined(USE_SW_SERIAL)
  #if ARDUINO >= 100
    #include <SoftwareSerial.h>
  #else
    #include <NewSoftSerial.h>
  #endif
#endif

// different commands to set the update rate from once a second (1 Hz) to 10 times a second (10Hz)
// Note that these only control the rate at which the position is echoed, to actually speed up the
// position fix you must also send one of the position fix rate commands below too.
#define PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ  "$PMTK220,10000*2F" // Once every 10 seconds, 100 millihertz.
#define PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ  "$PMTK220,5000*1B"  // Once every 5 seconds, 200 millihertz.
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_2HZ  "$PMTK220,500*2B"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"
// Position fix update rate commands.
#define PMTK_API_SET_FIX_CTL_100_MILLIHERTZ  "$PMTK300,10000,0,0,0,0*2C" // Once every 10 seconds, 100 millihertz.
#define PMTK_API_SET_FIX_CTL_200_MILLIHERTZ  "$PMTK300,5000,0,0,0,0*18"  // Once every 5 seconds, 200 millihertz.
#define PMTK_API_SET_FIX_CTL_1HZ  "$PMTK300,1000,0,0,0,0*1C"
#define PMTK_API_SET_FIX_CTL_5HZ  "$PMTK300,200,0,0,0,0*2F"
// Can't fix position faster than 5 times a second!


#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C"
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"

// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn off output
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

// to generate your own sentences, check out the MTK command datasheet and use a checksum calculator
// such as the awesome http://www.hhhh.org/wiml/proj/nmeaxor.html

#define PMTK_LOCUS_STARTLOG "$PMTK185,0*22"
#define PMTK_LOCUS_STOPLOG  "$PMTK185,1*23"
#define PMTK_LOCUS_STARTSTOPACK "$PMTK001,185,3*3C"
#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"
#define PMTK_LOCUS_ERASE_FLASH  "$PMTK184,1*22"
#define LOCUS_OVERLAP   0
#define LOCUS_FULLSTOP  1

#define PMTK_ENABLE_SBAS    "$PMTK313,1*2E"
#define PMTK_ENABLE_WAAS    "$PMTK301,2*2E"

// Standby mode command & boot successful message
#define PMTK_STANDBY    "$PMTK161,0*28"
#define PMTK_STANDBY_SUCCESS    "$PMTK001,161,3*36"
#define PMTK_AWAKE  "$PMTK010,002*2D"

// Backup mode commands:
// keeping GPS_EN signal low and sending PMTK command "$PMTK225,4*2F" will make L80 module enter
// into backup mode forever. To wake the GPS module up you must pull the GPS_EN signal high.
#define PMTK_BACKUP "$PMTK225,4*2F"

// AlwaysLocate mode
// AlwaysLocate standby commands:
#define PMTK_ALWAYSLOCATE_STANDBY   "$PMTK225,8*23"
#define PMTK_ALWAYSLOCATE_STANDBY_SUCCESS   "$PMTK001,225,3*35"
// AlwaysLocate backup command:
#define PMTK_ALWAYSLOCATE_BACKUP    "$PMTK225,9*22"
// AlwaysLocate exit command: the gps module returns into full on mode
#define PMTK_ALWAYSLOCATE_EXIT  "$PMTK225,0*2B"

// EASY function
// Enable EASY function
#define PMTK_EASY_ENABLE    "$PMTK869,1,1*35"
#define PMTK_EASY_DISABLE   "$PMTK869,1,0*34"
#define PMTK_EASY_CHECK_STATUS  "$PMTK869,0*29"
#define PMTK_EASY_STATUS_ENABLED    "$PMTK869,2,1*36"
#define PMTK_EASY_STATUS_DISABLED   "$PMTK869,2,0*37"

// ask for the release and version
#define PMTK_Q_RELEASE "$PMTK605*31"

// request for updates on antenna status 
#define PGCMD_ANTENNA "$PGCMD,33,1*6C"
#define PGCMD_NOANTENNA "$PGCMD,33,0*6D"

// AIC function:
#define PMTK_AIC_ENABLE "$PMTK286,1*23"
#define PMTK_AIC_DISABLE "$PMTK286,0*22"

// how long to wait when we're looking for a response
#define MAXWAITSENTENCE 10

// Maximum length of NMEA senstences
#define MAXLINELENGTH 120

#if ARDUINO >= 100
 #include "Arduino.h"
#if defined (__AVR__) && !defined(__AVR_ATmega32U4__)
 #include "SoftwareSerial.h"
#endif
#else
 #include "WProgram.h"
 #include "NewSoftSerial.h"
#endif


class Adafruit_GPS {

public:
  void begin(uint32_t baud); 

#if defined(__AVR__) && defined(USE_SW_SERIAL)
  #if ARDUINO >= 100 
    Adafruit_GPS(SoftwareSerial *ser); // Constructor when using SoftwareSerial
  #else
    Adafruit_GPS(NewSoftSerial  *ser); // Constructor when using NewSoftSerial
  #endif
#endif
  Adafruit_GPS(HardwareSerial *ser); // Constructor when using HardwareSerial

  char *lastNMEA(void);
  bool newNMEAreceived();
  void common_init(void);

  void sendCommand(const char *);
  
  void pause(boolean b);

  bool parseNMEA(char *response);
  uint8_t parseHex(char c);

  char read(void);
  bool parse(char *);
  
  uint8_t getHour(void);
  uint8_t getMinutes(void);
  uint8_t getSeconds(void);
  uint8_t getMilliseconds(void);
  
  uint8_t getYear(void);
  uint8_t getMonth(void);
  uint8_t getDay(void);
  
  float getLatitude(void);
  float getLongitude(void);
  int32_t getLatitudeFixed(void);
  int32_t getLongitudeFixed(void);
  float getLatitudeDegrees(void);
  float getLongitudeDegrees(void);
  float getAltitude(void);
  float getGeoidheight(void);
  float getSpeed(void);
  float getAngle(void);
  float getMagVariation(void);
  float getHDOP(void);
  
  char getLatCardinalDir(void);
  char getLonCardinalDir(void);
  char getMagCardinalDir(void);
  
  bool isFixed(void);
  uint8_t getQuality(void);
  
  uint8_t getSatellites(void);

  bool wakeupStandby(void);
  bool setStandbyMode(void);
  
  bool setAlwaysLocateMode(void);
  bool wakeupAlwaysLocate(void);

  bool waitForSentence(const char *wait, uint8_t max = MAXWAITSENTENCE);
  bool LOCUS_StartLogger(void);
  bool LOCUS_StopLogger(void);
  bool LOCUS_ReadStatus(void);
  
  uint16_t LOCUS_GetSerial();
  uint16_t LOCUS_GetRecords();
  uint8_t LOCUS_GetType();
  uint8_t LOCUS_GetMode();
  uint8_t LOCUS_GetConfig();
  uint8_t LOCUS_GetInterval();
  uint8_t LOCUS_GetDistance();
  uint8_t LOCUS_GetSpeed();
  uint8_t LOCUS_GetStatus();
  uint8_t LOCUS_GetPercent();

private:
  bool paused;
  bool inStandbyMode;
  bool inFullOnMode;
  bool inAlwaysLocateMode;
  
  bool recvdflag;
  // we double buffer: read one line in and leave one for the main program
  char line1[MAXLINELENGTH];
  char line2[MAXLINELENGTH];
  // our index into filling the current line
  uint8_t lineidx = 0;
  // pointers to the double buffers
  char *currentline;
  char *lastline;
  
  uint8_t hour, minute, seconds, year, month, day;
  uint16_t milliseconds;
  // Floating point latitude and longitude value in degrees.
  float latitude, longitude;
  // Fixed point latitude and longitude value with degrees stored in units of 1/100000 degrees,
  // and minutes stored in units of 1/100000 degrees.  See pull #13 for more details:
  //   https://github.com/adafruit/Adafruit-GPS-Library/pull/13
  int32_t latitude_fixed, longitude_fixed;
  float latitudeDegrees, longitudeDegrees;
  float geoidheight, altitude;
  float speed, angle, magvariation, HDOP;
  char lat, lon, mag;
  bool fix;
  uint8_t fixQuality, satellites;
  
  uint16_t LOCUS_serial, LOCUS_records;
  uint8_t LOCUS_type, LOCUS_mode, LOCUS_config, LOCUS_interval, LOCUS_distance, LOCUS_speed, LOCUS_status, LOCUS_percent;
  
  uint8_t parseResponse(char *response);
#if defined(__AVR__) && defined(USE_SW_SERIAL)
  #if ARDUINO >= 100
    SoftwareSerial *gpsSwSerial;
  #else
    NewSoftSerial  *gpsSwSerial;
  #endif
#endif
  HardwareSerial *gpsHwSerial;
};


#endif

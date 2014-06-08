/***********************************
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
****************************************/

#ifndef _ADAFRUIT_GPS_H
#define _ADAFRUIT_GPS_H

#ifdef __AVR__
  #if ARDUINO >= 100
    #include <SoftwareSerial.h>
  #else
    #include <NewSoftSerial.h>
  #endif
#endif

// different commands to set the update rate from once a second (1 Hz) to 10 times a second (10Hz)
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"


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

#define PMTK_LOCUS_STARTLOG  "$PMTK185,0*22"
#define PMTK_LOCUS_LOGSTARTED "$PMTK001,185,3*3C"
#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"
#define PMTK_LOCUS_ERASE_FLASH "$PMTK184,1*22"
#define LOCUS_OVERLAP 0
#define LOCUS_FULLSTOP 1

// standby command & boot successful message
#define PMTK_STANDBY "$PMTK161,0*28"
#define PMTK_STANDBY_SUCCESS "$PMTK001,161,3*36"  // Not needed currently
#define PMTK_AWAKE "$PMTK010,002*2D"

// ask for the release and version
#define PMTK_Q_RELEASE "$PMTK605*31"

// request for updates on antenna status 
#define PGCMD_ANTENNA "$PGCMD,33,1*6C" 
#define PGCMD_NOANTENNA "$PGCMD,33,0*6C" 

// how long to wait when we're looking for a response
#define MAXWAITSENTENCE 5

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
  void begin(uint16_t baud); 

#ifdef __AVR__
  #if ARDUINO >= 100 
    Adafruit_GPS(SoftwareSerial *ser); // Constructor when using SoftwareSerial
  #else
    Adafruit_GPS(NewSoftSerial  *ser); // Constructor when using NewSoftSerial
  #endif
#endif
  Adafruit_GPS(HardwareSerial *ser); // Constructor when using HardwareSerial

  char *lastNMEA(void);
  boolean newNMEAreceived();
  void common_init(void);
  void sendCommand(char *);
  void pause(boolean b);

  boolean parseNMEA(char *response);
  uint8_t parseHex(char c);

  char read(void);
  boolean parse(char *);
  void interruptReads(boolean r);

  boolean wakeup(void);
  boolean standby(void);

  uint8_t hour, minute, seconds, year, month, day;
  uint16_t milliseconds;
  float latitude, longitude, geoidheight, altitude;
  float speed, angle, magvariation, HDOP;
  char lat, lon, mag;
  boolean fix;
  uint8_t fixquality, satellites;

  boolean waitForSentence(char *wait, uint8_t max = MAXWAITSENTENCE);
  boolean LOCUS_StartLogger(void);
  boolean LOCUS_ReadStatus(void);

  uint16_t LOCUS_serial, LOCUS_records;
  uint8_t LOCUS_type, LOCUS_mode, LOCUS_config, LOCUS_interval, LOCUS_distance, LOCUS_speed, LOCUS_status, LOCUS_percent;
 private:
  boolean paused;
  
  uint8_t parseResponse(char *response);
#ifdef __AVR__
  #if ARDUINO >= 100
    SoftwareSerial *gpsSwSerial;
  #else
    NewSoftSerial  *gpsSwSerial;
  #endif
#endif
  HardwareSerial *gpsHwSerial;
};


#endif

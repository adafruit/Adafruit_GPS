/***********************************
This is a our GPS library

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/

#ifndef _ADAFRUIT_GPS_H
#define _ADAFRUIT_GPS_H

// different commands to set the update rate from once a second (1 Hz) to 10 times a second (10Hz)
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"

// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

// to generate your own sentences, check out the MTK command datasheet and use a checksum calculator
// such as the awesome http://www.hhhh.org/wiml/proj/nmeaxor.html

#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"



#if ARDUINO >= 100
 #include "Arduino.h"
 #include "SoftwareSerial.h"
#else
 #include "WProgram.h"
 #include "NewSoftSerial.h"
#endif

// how long are max NMEA lines to parse?
#define MAXLINELENGTH 100

class Adafruit_GPS {
 public:
  Adafruit_GPS(void); // Constructor when using SoftwareSerial

#if ARDUINO >= 100
  void begin(SoftwareSerial *ser, uint16_t baud); // Constructor when using SoftwareSerial
#else
  void begin(NewSoftSerial  *ser, uint16_t baud); // Constructor when using NewSoftSerial
#endif
  void begin(HardwareSerial *ser, uint16_t baud); // Constructor when using HardwareSerial

  char *lastNMEA(void);
  boolean newNMEAreceived();
  void common_init(void);
  void sendCommand(char *);
  void pause(boolean b);

  boolean parseNMEA(char *response);
  uint8_t parseHex(char c);

  void read(void);
  boolean parse(char *);
  void interruptReads(boolean r);

  uint8_t hour, minute, seconds, year, month, day;
  uint16_t milliseconds;
  float latitude, longitude, geoidheight, altitude;
  float speed, angle, magvariation, HDOP;
  char lat, lon, mag;
  boolean fix;
  uint8_t fixquality, satellites;

 private:
  boolean paused;
  boolean interrupt;
  
  uint8_t parseResponse(char *response);
#if ARDUINO >= 100
  SoftwareSerial *gpsSwSerial;
#else
  NewSoftSerial  *gpsSwSerial;
#endif
  HardwareSerial *gpsHwSerial;
};




extern Adafruit_GPS GPS;

#endif

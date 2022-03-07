/**************************************************************************/
/*!
  @file NMEA_data.h
*/
/**************************************************************************/
#ifndef _NMEA_DATA_H
#define _NMEA_DATA_H
#include "Arduino.h"

#define NMEA_MAX_WP_ID                                                         \
  20 ///< maximum length of a waypoint ID name, including terminating 0
#define NMEA_MAX_SENTENCE_ID                                                   \
  20 ///< maximum length of a sentence ID name, including terminating 0
#define NMEA_MAX_SOURCE_ID                                                     \
  3 ///< maximum length of a source ID name, including terminating 0

/*************************************************************************
  doubles and floats are identical on AVR processors like the UNO where space
  is tight. doubles avoid the roundoff errors that led to the fixed point mods
  in https://github.com/adafruit/Adafruit-GPS-Library/pull/13, provided the
  processor supports actual doubles like the SAMD series with more storage. The
  total penalty for going all double is under a few hundred bytes / instance or
  0 bytes / instance on an UNO. This typedef allows a switch to lower precision
  to save some storage if needed. A float carries 23 bits of fractional
  resolution, giving a resolution of at least 9 significant digits, thus 6
  significant digits in the decimal place of an angular value like latitude, and
  thus a resolution on earth of at least 110 mm. That's closer than GPS will
  hit, and closer than needed for navigation, so floats can be used to save a
  little storage.
 **************************************************************************/
#ifndef NMEA_FLOAT_T
#define NMEA_FLOAT_T float ///< let float be overidden on command line
#endif
typedef NMEA_FLOAT_T
    nmea_float_t; ///< the type of variables to use for floating point

/**************************************************************************/
/*!
  Struct to contain all the details associated with the history of an NMEA
  data value as an optional extension to the data value struct. The history
  is stored as scaled integers to save space while caputuring a reasonable
  level of resolution. The integer is set equal to scale * (X - offset) and
  can be converted back to an approximate float value with
  X = I / scale + offset

  Only some tags have history in order to save memory. Most of the memory
  cost is directly in the array.

  192 history values taken every 20 seconds covers just over an hour.
 **************************************************************************/
typedef struct {
  int16_t *data = NULL;          ///< array of ints, oldest first
  unsigned n = 0;                ///< number of history array elements
  uint32_t lastHistory = 0;      ///< millis() when history was last updated
  uint16_t historyInterval = 20; ///< seconds between history updates
  nmea_float_t scale = 1.0;      ///< history = (smoothed - offset) * scale
  nmea_float_t offset = 0.0;     ///< value = (float) history / scale + offset
} nmea_history_t;

/**************************************************************************/
/*!
    Type to characterize the type of value stored in a data value struct.
    The sine and cosine components of some angles allow
    for smoothing of those angles by averaging of sine and cosine values
    that are continuous, rather than angles that are discontinuous at
    -180/180 or 359/0 transitions. Types 10-19 must have three contiguous
    data value entries set up in the matrix to accommodate the extra sin
    and cos values.
 */
/**************************************************************************/
typedef enum {
  NMEA_SIMPLE_FLOAT = 0,  ///< A simple floating point number
  NMEA_COMPASS_ANGLE = 1, ///< A compass style angle from 0 to 360 degrees
  NMEA_BOAT_ANGLE = 2,    ///< An angle relative to the boat orientation
                          ///< from -180 (port) to 180 degrees
  NMEA_COMPASS_ANGLE_SIN =
      11, ///< A compass style angle from 0 to 360 degrees, with sin and cos
          ///< elements stored for averaging, etc.
  NMEA_BOAT_ANGLE_SIN =
      12, ///< An angle relative to the boat orientation from -180 (port) to 180
          ///< degrees, with sin and cos elements stored for averaging, etc.
  NMEA_DDMM = 20, ///< A latitude or longitude angle stored in DDMM.mmmm format
                  ///< like it comes in from the GPS
  NMEA_HHMMSS =
      30 ///< A time stored in HHMMSS format like it comes in from the GPS
} nmea_value_type_t;

/**************************************************************************/
/*!
    Struct to contain all the details associated with an NMEA data value that
    can be tracked through time to see how it changes, carries a label, units,
    and a format string to determine how it is displayed. Memory footprint
    of about 32 bytes per data value, so not tenable in small memory spaces.
*/
/**************************************************************************/
typedef struct {
  nmea_float_t latest = 0.0; ///< the most recently obtained value
  nmea_float_t smoothed =
      0.0;                  ///< smoothed value based on weight of dt/response
  uint32_t lastUpdate = 0;  ///< millis() when latest was last set
  uint16_t response = 1000; ///< time constant in millis for smoothing
  nmea_value_type_t type =
      NMEA_SIMPLE_FLOAT; ///< type of float data value represented
  byte ockam = 0; ///< the corresponding Ockam Instruments tag number, 0-128
  nmea_history_t *hist = NULL; ///< pointer to history, if any
  char *label = NULL;          ///< pointer to quantity label, if any
  char *unit = NULL;           ///< pointer to units label, if any
  char *fmt = NULL;            ///< pointer to format string, if any
} nmea_datavalue_t;

/**************************************************************************/
/*!
    Type to provide an index into the array of data values for different
    NMEA quantities. The sine and cosine components of some angles allow
    for smoothing of those angles by averaging of sine and cosine values
    that are continuous, rather than angles that are discontinuous at
    -180/180 or 359/0 transitions. Note that the enumerations are arranged
    so that NMEA_XXX_SIN = NMEA_XXX + 1, and NMEA_XXX_COS = NMEA_XXX + 2.
 */
/**************************************************************************/
typedef enum {
  NMEA_HDOP = 0, ///< Horizontal Dilution of Position
  NMEA_LAT,      ///< Latitude in signed decimal degrees -90 to 90
  NMEA_LON,      ///< Longitude in signed decimal degrees -180 to 180
  NMEA_LATWP,    ///< Waypoint Latitude in signed decimal degrees -90 to 90
  NMEA_LONWP,    ///< Waypoint Longitude in signed decimal degrees -180 to 180
  NMEA_SOG,      ///< Speed over Ground in knots
  NMEA_COG,      ///< Course over ground, 0 to 360 degrees true
  NMEA_COG_SIN,  ///< sine of Course over ground
  NMEA_COG_COS,  ///< cosine of Course over ground
  NMEA_COGWP,    ///< Course over ground to the waypoint, 0 to 360 degrees true
  NMEA_XTE,      ///< Cross track error for the current segment to the waypoint,
                 ///< Nautical Miles -ve to the left
  NMEA_DISTWP,   ///< Distance to the waypoint in nautical miles
  NMEA_AWA, ///< apparent wind angle relative to the boat -180 to 180 degrees
  NMEA_AWA_SIN, ///< sine of apparent wind angle relative to the boat
  NMEA_AWA_COS, ///< cosine of apparent wind angle relative to the boat
  NMEA_AWS,     ///< apparent wind speed, will be coerced to knots
  NMEA_TWA,     ///< true wind angle relative to the boat -180 to 180 degrees
  NMEA_TWA_SIN, ///< sine of true wind angle relative to the boat
  NMEA_TWA_COS, ///< cosine of true wind angle relative to the boat
  NMEA_TWD, ///< true wind compass direction, magnetic 0 to 360 degrees magnetic
  NMEA_TWD_SIN, ///< sine of true wind compass direction, magnetic
  NMEA_TWD_COS, ///< cosine of true wind compass direction, magnetic
  NMEA_TWS,     ///< true wind speed in knots TWS
  NMEA_VMG,     ///< velocity made good relative to the wind -ve means downwind,
                ///< knots
  NMEA_VMGWP,   ///< velocity made good relative to the waypoint, knots
  NMEA_HEEL,    ///< boat heel angle, -180 to 180 degrees to starboard
  NMEA_PITCH,   ///< boat pitch angle, -180 to 180 degrees bow up
  NMEA_HDG,     ///< magnetic heading, 0 to 360 degrees magnetic
  NMEA_HDG_SIN, ///< sine of magnetic heading
  NMEA_HDG_COS, ///< cosine of magnetic heading
  NMEA_HDT,     ///< true heading, 0 to 360 degrees true
  NMEA_HDT_SIN, ///< sine of true heading
  NMEA_HDT_COS, ///< cosine of true heading
  NMEA_VTW,     ///< Boat speed through the water in knots
  NMEA_LOG,     ///< Distance logged through the water in nautical miles
  NMEA_LOGR,    ///< Distance logged through the water in nautical miles since
                ///< reset
  NMEA_DEPTH,   ///< depth of water below the surface in metres
  NMEA_RPM_M1,  ///< rpm of motor 1
  NMEA_TEMPERATURE_M1,    ///< temperature of motor 1 in C
  NMEA_PRESSURE_M1,       ///< pressure of motor 1 in kPa
  NMEA_VOLTAGE_M1,        ///< voltage of motor 1 in Volts
  NMEA_CURRENT_M1,        ///< current of motor 1 in Amps
  NMEA_RPM_M2,            ///< rpm of motor 2
  NMEA_TEMPERATURE_M2,    ///< temperature of motor 2 in C
  NMEA_PRESSURE_M2,       ///< pressure of motor 2 in kPa
  NMEA_VOLTAGE_M2,        ///< voltage of motor 2 in Volts
  NMEA_CURRENT_M2,        ///< current of motor 2 in Amps
  NMEA_TEMPERATURE_AIR,   ///< outside temperature in C
  NMEA_TEMPERATURE_WATER, ///< sea water temperature in C
  NMEA_HUMIDITY,          ///< outside relative humidity in %
  NMEA_BAROMETER, ///< barometric pressure in Pa absolute -- not altitude
                  ///< corrected
  NMEA_USR_00,    ///< spaces for a user sketch to inject its own data
  NMEA_USR_01,    ///< spaces for a user sketch to inject its own data
  NMEA_USR_02,    ///< spaces for a user sketch to inject its own data
  NMEA_USR_03,
  NMEA_USR_04,
  NMEA_USR_05,
  NMEA_USR_06,
  NMEA_USR_07,
  NMEA_USR_08,
  NMEA_USR_09,
  NMEA_USR_10,
  NMEA_USR_11,
  NMEA_USR_12,
  NMEA_MAX_INDEX ///< the largest number in the enum type -- not for data,
                 ///< but does define size of data value array required.
} nmea_index_t;  ///< Indices for data values expected to change often with time

#endif // _NMEA_DATA_H

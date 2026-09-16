/**************************************************************************/
/*!
  @file Adafruit_GNSS.h
  @brief Transport-independent decoding of standard GNSS fields.

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#ifndef ADAFRUIT_GNSS_H
#define ADAFRUIT_GNSS_H

#include "Adafruit_NMEA.h"

/** Validated coordinate components, with no floating-point conversion.
 *  Every value except status is zero on failure. Southern and western
 *  coordinates have negative degreesE7; the other components are unsigned.
 */
typedef struct {
  nmea_number_status_t status; ///< Result of decoding the coordinate pair.
  int32_t degreesE7; ///< Decimal degrees times 10000000, truncated toward zero.
  uint16_t degrees;  ///< Whole unsigned degrees, 0 through 180.
  uint8_t minutes;   ///< Whole minutes, 0 through 59.
  uint32_t fractionalMinutes; ///< Fraction of a minute times 1000000000.
  char hemisphere;            ///< Exactly N, S, E, or W on success.
} gnss_coordinate_t;

/** Stateless standard GNSS decoding shared by receiver implementations.
 *  Inputs are borrowed spans. Decoding neither allocates memory nor performs
 *  transport I/O, and does not alter the supplied text or receiver state.
 */
class Adafruit_GNSS {
public:
  static gnss_coordinate_t parseCoordinate(nmea_span_t coordinate,
                                           nmea_span_t hemisphere);
};

#endif // ADAFRUIT_GNSS_H

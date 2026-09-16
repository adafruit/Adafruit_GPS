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

/// Decoded UTC time; all components are zero on failure.
typedef struct {
  nmea_number_status_t status; ///< Result of decoding the field.
  uint8_t hour;                ///< UTC hour, 0 through 23.
  uint8_t minute;              ///< Minute, 0 through 59.
  uint8_t second;              ///< Second, 0 through 60 for a leap second.
  uint16_t millisecond; ///< Fraction truncated to milliseconds, 0 to 999.
} gnss_time_t;

/// Decoded NMEA date; no century is inferred. Components are zero on failure.
typedef struct {
  nmea_number_status_t status; ///< Result of decoding the field.
  uint8_t day;                 ///< Day of month, 1 through 31.
  uint8_t month;               ///< Month, 1 through 12.
  uint8_t year;                ///< Two-digit year, 0 through 99.
} gnss_date_t;

/// Result of validating fields used by a standard navigation decoder.
typedef enum : uint8_t {
  GNSS_SENTENCE_VALID,          ///< Supported fields are valid or empty.
  GNSS_SENTENCE_UNSUPPORTED,    ///< This validator does not handle the type.
  GNSS_SENTENCE_MISSING_FIELDS, ///< A required field position is absent.
  GNSS_SENTENCE_INVALID_FIELD   ///< A populated field has invalid content.
} gnss_sentence_status_t;

/// Validation result; coordinate-pair errors identify the coordinate field.
typedef struct {
  gnss_sentence_status_t status; ///< Result of navigation field validation.
  uint8_t field; ///< One-based error field, or zero for valid/unsupported.
} gnss_validation_t;

/** Stateless standard GNSS decoding shared by receiver implementations.
 *  Inputs are borrowed spans. Decoding neither allocates memory nor performs
 *  transport I/O, and does not alter the supplied text or receiver state.
 */
class Adafruit_GNSS {
public:
  static gnss_coordinate_t parseCoordinate(nmea_span_t coordinate,
                                           nmea_span_t hemisphere);
  static gnss_time_t parseTime(nmea_span_t field);
  static gnss_date_t parseDate(nmea_span_t field);
  static gnss_validation_t validateNavigation(nmea_span_t type,
                                              nmea_span_t fields);
};

#endif // ADAFRUIT_GNSS_H

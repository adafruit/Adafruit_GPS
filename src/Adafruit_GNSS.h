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

/// Maximum coordinate text size: sign, three degrees, dot, 11 decimals, NUL.
#define GNSS_COORDINATE_TEXT_SIZE 17

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
  GNSS_SENTENCE_INVALID_FIELD,  ///< A populated field has invalid content.
  GNSS_SENTENCE_INVALID_FRAME   ///< No complete frame with valid checksum.
} gnss_sentence_status_t;

/// Validation result; coordinate-pair errors identify the coordinate field.
typedef struct {
  gnss_sentence_status_t status; ///< Result of navigation field validation.
  uint8_t field; ///< One-based error field, or zero without a field error.
} gnss_validation_t;

/** One sentence's position data, independent of earlier receiver state.
 *  Check validation first, then each field's status. A valid sentence does not
 *  imply a position fix or populated coordinates. Failed/unsupported decoding
 *  leaves all measurement values zero and all field statuses MISSING. */
typedef struct {
  gnss_validation_t validation; ///< Sentence status and first failing field.
  gnss_coordinate_t latitude;   ///< Exact latitude components.
  gnss_coordinate_t longitude;  ///< Exact longitude components.
  gnss_time_t time;             ///< UTC time, or EMPTY for a blank time field.
  gnss_date_t date; ///< RMC date; MISSING in GGA/GLL, possibly EMPTY.
  nmea_number_status_t fixStatus;        ///< Fix field status.
  bool fix;                              ///< Valid fix; check fixStatus first.
  nmea_number_status_t fixQualityStatus; ///< Quality field status.
  uint8_t fixQuality; ///< GGA quality, including RTK values; check its status.
} gnss_position_t;

/** Standard GNSS receiver and decoding shared by receiver implementations.
 *  Inherits bounded framing with caller-owned buffers and per-instance times.
 *  Position decoding is stateless: no fix is cached or merged across lines.
 *  Static decoding does not require receive storage. No heap or transport I/O.
 */
class Adafruit_GNSS : public Adafruit_NMEA {
public:
  Adafruit_GNSS(volatile char *firstBuffer = NULL,
                volatile char *secondBuffer = NULL, size_t capacity = 0);
  gnss_position_t lastPosition() const;
  static gnss_coordinate_t parseCoordinate(nmea_span_t coordinate,
                                           nmea_span_t hemisphere);
  static size_t formatCoordinate(char *output, size_t capacity,
                                 const gnss_coordinate_t &coordinate);
  static gnss_position_t parsePosition(nmea_span_t type, nmea_span_t fields);
  static gnss_position_t parsePosition(const nmea_sentence_t &sentence);
  static gnss_time_t parseTime(nmea_span_t field);
  static gnss_date_t parseDate(nmea_span_t field);
  static gnss_validation_t validateNavigation(nmea_span_t type,
                                              nmea_span_t fields);

private:
  static uint8_t sentenceType(nmea_span_t type);
  static gnss_validation_t decodeNavigation(uint8_t sentence,
                                            nmea_span_t fields,
                                            gnss_position_t *position);
  /// Sentence kinds handled by the navigation validator.
  enum {
    UNSUPPORTED, ///< Sentence has no standard navigation validation.
    GGA,         ///< Fix time, position, and altitude.
    RMC,         ///< Recommended minimum navigation data.
    GLL,         ///< Geographic position and time.
    GSA          ///< Fix type and dilution of precision.
  };
};

#endif // ADAFRUIT_GNSS_H

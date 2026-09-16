/**************************************************************************/
/*!
  @file Adafruit_GNSS.cpp
  @brief Transport-independent decoding of standard GNSS fields.

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#include "Adafruit_GNSS.h"

/**************************************************************************/
/*!
    @brief Decode a latitude or longitude field and its hemisphere.
    @param coordinate DDMM[.fraction] latitude or DDDMM[.fraction] longitude.
    @param hemisphere Exactly one N/S for latitude or E/W for longitude.
    @return Validated components and signed decimal degrees times 10000000.

    Missing spans take precedence over empty spans, then malformed syntax,
    then range errors. Signs, spaces, exponents, and a bare decimal point are
    invalid. Minutes must be below 60; latitude is limited to 90 degrees and
    longitude to 180, with zero minutes at either boundary. Fractional minutes
    retain the first nine digits, padded with zeros; further digits are
    validated and truncated. degreesE7 is exact at its stated resolution,
    without rounding or intermediate floating-point arithmetic.
*/
/**************************************************************************/
gnss_coordinate_t Adafruit_GNSS::parseCoordinate(nmea_span_t coordinate,
                                                 nmea_span_t hemisphere) {
  gnss_coordinate_t result = {};
  result.status = NMEA_NUMBER_MISSING;
  if (!coordinate.data || !hemisphere.data)
    return result;
  result.status = NMEA_NUMBER_EMPTY;
  if (!coordinate.length || !hemisphere.length)
    return result;
  result.status = NMEA_NUMBER_BAD_FORMAT;
  if (hemisphere.length != 1)
    return result;
  char direction = hemisphere.data[0];
  bool latitude = direction == 'N' || direction == 'S';
  if (!latitude && direction != 'E' && direction != 'W')
    return result;

  size_t wholeDigits = latitude ? 4 : 5;
  if (coordinate.length < wholeDigits)
    return result;
  uint32_t whole = 0;
  for (size_t i = 0; i < wholeDigits; i++) {
    char c = coordinate.data[i];
    if (c < '0' || c > '9')
      return result;
    whole = whole * 10 + c - '0';
  }
  uint32_t fraction = 0;
  uint8_t retained = 0;
  bool nonzeroFraction = false;
  if (coordinate.length > wholeDigits) {
    if (coordinate.data[wholeDigits] != '.' ||
        coordinate.length == wholeDigits + 1)
      return result;
    for (size_t i = wholeDigits + 1; i < coordinate.length; i++) {
      char c = coordinate.data[i];
      if (c < '0' || c > '9')
        return result;
      nonzeroFraction = nonzeroFraction || c != '0';
      if (retained < 9) {
        fraction = fraction * 10 + c - '0';
        retained++;
      }
    }
  }
  while (retained < 9) {
    fraction *= 10;
    retained++;
  }
  uint16_t degrees = whole / 100;
  uint8_t minutes = whole % 100;
  uint16_t limit = latitude ? 90 : 180;
  result.status = NMEA_NUMBER_OUT_OF_RANGE;
  if (minutes >= 60 || degrees > limit ||
      (degrees == limit && (minutes || nonzeroFraction)))
    return result;

  // Keeping seven fractional-minute digits is sufficient for an exact E7
  // truncation after division by 60. The largest intermediate fits uint32_t.
  uint32_t minuteE7 = minutes * 10000000UL + fraction / 100;
  int32_t fixed = degrees * 10000000L + minuteE7 / 60;
  if (direction == 'S' || direction == 'W')
    fixed = -fixed;
  result.status = NMEA_NUMBER_VALID;
  result.degreesE7 = fixed;
  result.degrees = degrees;
  result.minutes = minutes;
  result.fractionalMinutes = fraction;
  result.hemisphere = direction;
  return result;
}

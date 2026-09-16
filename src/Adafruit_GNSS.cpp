/**************************************************************************/
/*!
  @file Adafruit_GNSS.cpp
  @brief Transport-independent decoding of standard GNSS fields.

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#include "Adafruit_GNSS.h"

static nmea_number_status_t parseSixDigits(nmea_span_t field, uint8_t *pairs);
static bool validUnsignedByte(nmea_span_t field);

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

/**************************************************************************/
/*!
    @brief Decode a complete UTC hhmmss[.fraction] field.
    @param field Borrowed time field, with no terminator required.
    @return Status and validated components, zeroed on failure.

    Exactly six whole digits are required. A decimal point requires at least
    one fractional digit; all digits are checked, with the first three retained
    as milliseconds. Seconds may be 60 to represent a leap second. This checks
    field ranges, not whether a leap second occurred on a particular date.
*/
/**************************************************************************/
gnss_time_t Adafruit_GNSS::parseTime(nmea_span_t field) {
  gnss_time_t result = {};
  uint8_t pairs[3];
  nmea_span_t whole = field;
  if (whole.length > 6)
    whole.length = 6;
  result.status = parseSixDigits(whole, pairs);
  if (result.status != NMEA_NUMBER_VALID)
    return result;
  result.status = NMEA_NUMBER_BAD_FORMAT;
  uint16_t milliseconds = 0;
  uint8_t retained = 0;
  if (field.length > 6) {
    if (field.data[6] != '.' || field.length == 7)
      return result;
    for (size_t i = 7; i < field.length; i++) {
      char c = field.data[i];
      if (c < '0' || c > '9')
        return result;
      if (retained < 3) {
        milliseconds = milliseconds * 10 + c - '0';
        retained++;
      }
    }
  }
  while (retained < 3) {
    milliseconds *= 10;
    retained++;
  }
  result.status = NMEA_NUMBER_OUT_OF_RANGE;
  if (pairs[0] > 23 || pairs[1] > 59 || pairs[2] > 60)
    return result;
  result.status = NMEA_NUMBER_VALID;
  result.hour = pairs[0];
  result.minute = pairs[1];
  result.second = pairs[2];
  result.millisecond = milliseconds;
  return result;
}

/**************************************************************************/
/*!
    @brief Decode a complete ddmmyy date field.
    @param field Borrowed six-digit date, with no terminator required.
    @return Status and validated components, zeroed on failure.

    Validates month lengths and February 29 using year modulo four. The NMEA
    field has no century; year 00 is treated as a leap year, as in 2000.
    Applications requiring a full year must resolve the century separately.
*/
/**************************************************************************/
gnss_date_t Adafruit_GNSS::parseDate(nmea_span_t field) {
  gnss_date_t result = {};
  uint8_t pairs[3];
  result.status = parseSixDigits(field, pairs);
  if (result.status != NMEA_NUMBER_VALID)
    return result;
  result.status = NMEA_NUMBER_OUT_OF_RANGE;
  uint8_t day = pairs[0], month = pairs[1], year = pairs[2];
  if (!month || month > 12 || !day)
    return result;
  uint8_t days = 31;
  if (month == 2)
    days = year % 4 == 0 ? 29 : 28;
  else if (month == 4 || month == 6 || month == 9 || month == 11)
    days = 30;
  if (day > days)
    return result;
  result.status = NMEA_NUMBER_VALID;
  result.day = day;
  result.month = month;
  result.year = year;
  return result;
}

// Shared syntax check for hhmmss and ddmmyy, without a copied field buffer.
static nmea_number_status_t parseSixDigits(nmea_span_t field, uint8_t *pairs) {
  if (!field.data)
    return NMEA_NUMBER_MISSING;
  if (!field.length)
    return NMEA_NUMBER_EMPTY;
  if (field.length != 6)
    return NMEA_NUMBER_BAD_FORMAT;
  for (uint8_t i = 0; i < 6; i++) {
    char c = field.data[i];
    if (c < '0' || c > '9')
      return NMEA_NUMBER_BAD_FORMAT;
    if (i % 2 == 0)
      pairs[i / 2] = (c - '0') * 10;
    else
      pairs[i / 2] += c - '0';
  }
  return NMEA_NUMBER_VALID;
}

/**************************************************************************/
/*!
    @brief Validate standard fields before updating navigation data.
    @param type Three-character sentence type, without its talker prefix.
    @param fields Fields after the address comma and before the checksum.
    @return Status and the first invalid or missing one-based field position.

    Call after validating the enclosing NMEA frame. Checks the fields consumed
    by the existing GPS decoders: GGA through geoid separation, RMC through
    date, GLL through status, and GSA fix type and dilution values. Optional
    tails and the satellite IDs skipped by the GPS parser are left to their
    decoders. Empty fields are permitted, but a coordinate and its hemisphere
    must either both be empty or both be valid for the appropriate axis.
    Numeric fields must be complete representable decimals; speed, course,
    and dilution values cannot be negative. Fix quality and satellite count
    must fit uint8_t. Valid does not imply a position fix or populated fields.
    No receiver state changes, field arrays, or heap allocation are involved.
*/
/**************************************************************************/
gnss_validation_t Adafruit_GNSS::validateNavigation(nmea_span_t type,
                                                    nmea_span_t fields) {
  gnss_validation_t result = {GNSS_SENTENCE_UNSUPPORTED, 0};
  if (!type.data || type.length != 3)
    return result;
  bool gga = type.data[0] == 'G' && type.data[1] == 'G' && type.data[2] == 'A';
  bool rmc = type.data[0] == 'R' && type.data[1] == 'M' && type.data[2] == 'C';
  bool gll = type.data[0] == 'G' && type.data[1] == 'L' && type.data[2] == 'L';
  bool gsa = type.data[0] == 'G' && type.data[1] == 'S' && type.data[2] == 'A';
  if (!gga && !rmc && !gll && !gsa)
    return result;
  uint8_t required = 6, latitudeField = 1, timeField = 5;
  if (gga) {
    required = 11;
    latitudeField = 2;
    timeField = 1;
  } else if (rmc) {
    required = 9;
    latitudeField = 3;
    timeField = 1;
  } else if (gsa) {
    required = 17;
    timeField = 0;
  }
  for (uint8_t i = 1; i <= required; i++) {
    nmea_span_t field = Adafruit_NMEA::nextField(fields);
    result.field = i;
    result.status = GNSS_SENTENCE_MISSING_FIELDS;
    if (!field.data)
      return result;
    if (!gsa && (i == latitudeField || i == latitudeField + 2)) {
      nmea_span_t hemisphere = Adafruit_NMEA::nextField(fields);
      if (!hemisphere.data) {
        result.field = i + 1;
        return result;
      }
      result.status = GNSS_SENTENCE_INVALID_FIELD;
      if (field.length || hemisphere.length) {
        gnss_coordinate_t coordinate = parseCoordinate(field, hemisphere);
        bool latitude = i == latitudeField;
        if (coordinate.status != NMEA_NUMBER_VALID ||
            (latitude && coordinate.hemisphere != 'N' &&
             coordinate.hemisphere != 'S') ||
            (!latitude && coordinate.hemisphere != 'E' &&
             coordinate.hemisphere != 'W'))
          return result;
      }
      i++;
      continue;
    }
    if (!field.length)
      continue;
    result.status = GNSS_SENTENCE_INVALID_FIELD;
    if (gsa && (i == 1 || (i >= 3 && i <= 14))) {
      continue; // Selection mode and satellite IDs are not decoded by GPS.
    } else if (i == timeField) {
      if (parseTime(field).status != NMEA_NUMBER_VALID)
        return result;
    } else if (rmc && i == 9) {
      if (parseDate(field).status != NMEA_NUMBER_VALID)
        return result;
    } else if ((rmc && i == 2) || (gll && i == 6)) {
      if (field.length != 1 || (field.data[0] != 'A' && field.data[0] != 'V'))
        return result;
    } else if ((gga && (i == 6 || i == 7)) || (gsa && i == 2)) {
      if (!validUnsignedByte(field))
        return result;
    } else if (gga && i == 10) {
      if (field.length != 1 || field.data[0] != 'M')
        return result;
    } else {
      nmea_decimal_t number = Adafruit_NMEA::parseDecimal(field);
      bool signedValue = gga && (i == 9 || i == 11);
      if (number.status != NMEA_NUMBER_VALID ||
          (!signedValue && number.coefficient < 0))
        return result;
    }
  }
  result.status = GNSS_SENTENCE_VALID;
  result.field = 0;
  return result;
}

// Keep integer-only byte fields exact; accepting atof-style prefixes can wrap.
static bool validUnsignedByte(nmea_span_t field) {
  uint16_t value = 0;
  for (size_t i = 0; i < field.length; i++) {
    char c = field.data[i];
    if (c < '0' || c > '9')
      return false;
    value = value * 10 + c - '0';
    if (value > UINT8_MAX)
      return false;
  }
  return true;
}

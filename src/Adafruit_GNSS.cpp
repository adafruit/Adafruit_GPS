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
    @brief Create a GNSS receiver using the shared bounded NMEA framer.
    @param firstBuffer First caller-owned writable receive buffer.
    @param secondBuffer Second buffer, disjoint from the first.
    @param capacity Bytes per buffer, including NUL space.

    Storage must outlive the receiver. The default or invalid storage disables
    feed() as described by Adafruit_NMEA; static decoding remains available.
    Supply incoming bytes and their timestamps with the inherited feed().
    No position cache, transport, command handling, or allocation is added.
*/
/**************************************************************************/
Adafruit_GNSS::Adafruit_GNSS(volatile char *firstBuffer,
                             volatile char *secondBuffer, size_t capacity)
    : Adafruit_NMEA(firstBuffer, secondBuffer, capacity) {}

/**************************************************************************/
/*!
    @brief Decode the latest complete line into an independent position result.
    @return Exact GGA/RMC/GLL measurements, or validation diagnostics.

    Call after feed() reports a complete line, and synchronize with feed() if
    using interrupts. Each call validates and decodes the current line; it does
    not consume it. Before a line or after reset(), the result is INVALID_FRAME.
    Invalid completed lines replace earlier positions; proprietary replies and
    other sentences return UNSUPPORTED and remain accessible via lastSentence().
    Partial/overflowing input preserves the previous complete line, just as the
    framer does. Use sentenceStartedAt() and sentenceReceivedAt() for this line.
    Returned values own their data and survive later input or reset().
*/
/**************************************************************************/
gnss_position_t Adafruit_GNSS::lastPosition() const {
  return parsePosition(lastSentence());
}

/**************************************************************************/
/*!
    @brief Route a validated standard NMEA sentence to the position decoder.
    @param sentence Unmodified view returned by validate() or lastSentence().
    @return Independent values; INVALID_FRAME for non-VALID input, UNSUPPORTED
    for a valid frame outside the supported standard position sentences.

    Keep the borrowed storage readable and unchanged throughout the call.
    Trust the supplied frame status; use validate() first for arbitrary text.
    A standard position address has two uppercase talker letters followed by
    GGA, RMC, or GLL. Any such talker is accepted except the proprietary 'P'
    prefix. Encapsulated '!' messages and proprietary addresses are not routed.
    No receiver-specific address whitelist or command handling is imposed on
    the underlying framer. Invalid/unsupported input returns no measurements.
*/
/**************************************************************************/
gnss_position_t Adafruit_GNSS::parsePosition(const nmea_sentence_t &sentence) {
  nmea_span_t type = {NULL, 0}, fields = {NULL, 0};
  if (sentence.status == NMEA_FRAME_VALID && sentence.text.data &&
      sentence.text.length && sentence.text.data[0] == '$' &&
      sentence.address.data && sentence.address.length == 5) {
    const char *address = sentence.address.data;
    if (address[0] >= 'A' && address[0] <= 'Z' && address[0] != 'P' &&
        address[1] >= 'A' && address[1] <= 'Z') {
      type = {address + 2, 3};
      fields = sentence.fields;
    }
  }
  gnss_position_t result = parsePosition(type, fields);
  if (sentence.status != NMEA_FRAME_VALID)
    result.validation.status = GNSS_SENTENCE_INVALID_FRAME;
  return result;
}

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
    @brief Format exact coordinate components as signed decimal degrees.
    @param output Writable text buffer; must not overlap coordinate.
    @param capacity Buffer size including the NUL terminator. Use
    GNSS_COORDINATE_TEXT_SIZE to fit every valid coordinate.
    @param coordinate Validated components from parseCoordinate().
    @return Characters written, excluding NUL, or zero for invalid components
    or insufficient storage. On failure output[0] is cleared when possible;
    no other output bytes are changed.

    Writes exactly 11 fractional degree digits, truncated toward zero. This
    distinguishes every retained fractional-minute increment and introduces
    less than 0.00000000001 degree of formatting error (about 1.2 micrometers
    of latitude). Southern/western zero retains its minus sign. The components,
    not the lower-resolution degreesE7 member, supply the result.

    Integer long division uses at most 32-bit arithmetic, with no heap or
    floating-point conversion. AVR output retains the same precision as other
    targets, regardless of NMEA_FLOAT_T or the platform's double size.
*/
/**************************************************************************/
size_t Adafruit_GNSS::formatCoordinate(char *output, size_t capacity,
                                       const gnss_coordinate_t &coordinate) {
  if (!output || !capacity)
    return 0;
  output[0] = '\0';
  char direction = coordinate.hemisphere;
  bool latitude = direction == 'N' || direction == 'S';
  uint16_t limit = latitude ? 90 : 180;
  uint16_t degrees = coordinate.degrees;
  uint16_t remainder = coordinate.minutes;
  uint32_t fraction = coordinate.fractionalMinutes;
  if (coordinate.status != NMEA_NUMBER_VALID ||
      (!latitude && direction != 'E' && direction != 'W') || degrees > limit ||
      remainder >= 60 || fraction >= 1000000000UL ||
      (degrees == limit && (remainder || fraction)))
    return 0;
  bool negative = direction == 'S' || direction == 'W';
  uint8_t digits = 1;
  uint16_t divisor = 1;
  if (degrees >= 100) {
    digits = 3;
    divisor = 100;
  } else if (degrees >= 10) {
    digits = 2;
    divisor = 10;
  }
  // Account for the sign, decimal point, eleven fractional digits, and NUL.
  if (capacity < (size_t)(negative + digits + 13))
    return 0;
  size_t written = 0;
  if (negative)
    output[written++] = '-';
  while (divisor) {
    output[written++] = '0' + degrees / divisor;
    degrees %= divisor;
    divisor /= 10;
  }
  output[written++] = '.';
  uint32_t place = 100000000UL;
  for (uint8_t i = 0; i < 11; i++) {
    remainder *= 10;
    if (place) {
      remainder += fraction / place;
      fraction %= place;
      place /= 10;
    }
    output[written++] = '0' + remainder / 60;
    remainder %= 60;
  }
  output[written] = '\0';
  return written;
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
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t tens = field.data[2 * i] - '0';
    uint8_t ones = field.data[2 * i + 1] - '0';
    if (tens > 9 || ones > 9)
      return NMEA_NUMBER_BAD_FORMAT;
    pairs[i] = tens * 10 + ones;
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
  if (!type.data || type.length != 3)
    return {GNSS_SENTENCE_UNSUPPORTED, 0};
  return decodeNavigation(sentenceType(type), fields, NULL);
}

/**************************************************************************/
/*!
    @brief Validate navigation and optionally collect exact position fields.
    @param sentence Private sentence kind.
    @param fields Bounded sentence fields.
    @param position Optional scratch result; discard it if validation fails.
    @return Status and first failing field.
*/
/**************************************************************************/
gnss_validation_t Adafruit_GNSS::decodeNavigation(uint8_t sentence,
                                                  nmea_span_t fields,
                                                  gnss_position_t *position) {
  gnss_validation_t result = {GNSS_SENTENCE_UNSUPPORTED, 0};
  if (sentence == UNSUPPORTED)
    return result;
  uint8_t required = 6, latitudeField = 1, timeField = 5;
  if (sentence == GGA) {
    required = 11;
    latitudeField = 2;
    timeField = 1;
  } else if (sentence == RMC) {
    required = 9;
    latitudeField = 3;
    timeField = 1;
  } else if (sentence == GSA) {
    required = 17;
    timeField = 0;
  }
  for (uint8_t i = 1; i <= required; i++) {
    nmea_span_t field = Adafruit_NMEA::nextField(fields);
    result.field = i;
    result.status = GNSS_SENTENCE_MISSING_FIELDS;
    if (!field.data)
      return result;
    if (sentence != GSA && (i == latitudeField || i == latitudeField + 2)) {
      nmea_span_t hemisphere = Adafruit_NMEA::nextField(fields);
      if (!hemisphere.data) {
        result.field = i + 1;
        return result;
      }
      result.status = GNSS_SENTENCE_INVALID_FIELD;
      gnss_coordinate_t coordinate = parseCoordinate(field, hemisphere);
      bool latitude = i == latitudeField;
      if (field.length || hemisphere.length) {
        if (coordinate.status != NMEA_NUMBER_VALID ||
            (latitude && coordinate.hemisphere != 'N' &&
             coordinate.hemisphere != 'S') ||
            (!latitude && coordinate.hemisphere != 'E' &&
             coordinate.hemisphere != 'W'))
          return result;
      }
      if (position) {
        if (latitude)
          position->latitude = coordinate;
        else
          position->longitude = coordinate;
      }
      i++;
      continue;
    }
    if (!field.length) {
      if (position) {
        if (i == timeField)
          position->time.status = NMEA_NUMBER_EMPTY;
        else if (sentence == RMC && i == 9)
          position->date.status = NMEA_NUMBER_EMPTY;
        else if ((sentence == RMC && i == 2) || (sentence != RMC && i == 6)) {
          position->fixStatus = NMEA_NUMBER_EMPTY;
          if (sentence == GGA)
            position->fixQualityStatus = NMEA_NUMBER_EMPTY;
        }
      }
      continue;
    }
    result.status = GNSS_SENTENCE_INVALID_FIELD;
    if (sentence == GSA && (i == 1 || (i >= 3 && i <= 14))) {
      continue; // Selection mode and satellite IDs are not decoded by GPS.
    } else if (i == timeField) {
      gnss_time_t time = parseTime(field);
      if (time.status != NMEA_NUMBER_VALID)
        return result;
      if (position)
        position->time = time;
    } else if (sentence == RMC && i == 9) {
      gnss_date_t date = parseDate(field);
      if (date.status != NMEA_NUMBER_VALID)
        return result;
      if (position)
        position->date = date;
    } else if ((sentence == RMC && i == 2) || (sentence == GLL && i == 6)) {
      if (field.length != 1 || (field.data[0] != 'A' && field.data[0] != 'V'))
        return result;
      if (position) {
        position->fixStatus = NMEA_NUMBER_VALID;
        position->fix = field.data[0] == 'A';
      }
    } else if ((sentence == GGA && (i == 6 || i == 7)) ||
               (sentence == GSA && i == 2)) {
      if (!validUnsignedByte(field))
        return result;
      if (position && sentence == GGA && i == 6) {
        position->fixStatus = position->fixQualityStatus = NMEA_NUMBER_VALID;
        for (size_t j = 0; j < field.length; j++)
          position->fixQuality =
              position->fixQuality * 10 + field.data[j] - '0';
        position->fix = position->fixQuality > 0;
      }
    } else if (sentence == GGA && i == 10) {
      if (field.length != 1 || field.data[0] != 'M')
        return result;
    } else {
      bool signedValue = sentence == GGA && (i == 9 || i == 11);
      if (Adafruit_NMEA::validateDecimal(field, signedValue) !=
          NMEA_NUMBER_VALID)
        return result;
    }
  }
  result.status = GNSS_SENTENCE_VALID;
  result.field = 0;
  return result;
}

/**************************************************************************/
/*!
    @brief Identify a supported three-character standard sentence type.
    @param type Three-character identifier already bounded by the caller.
    @return Private sentence kind, or UNSUPPORTED.
*/
/**************************************************************************/
uint8_t Adafruit_GNSS::sentenceType(nmea_span_t type) {
  uint8_t sentence = UNSUPPORTED;
  if (type.data[0] == 'G' && type.data[1] == 'G' && type.data[2] == 'A')
    sentence = GGA;
  else if (type.data[0] == 'R' && type.data[1] == 'M' && type.data[2] == 'C')
    sentence = RMC;
  else if (type.data[0] == 'G' && type.data[1] == 'L' && type.data[2] == 'L')
    sentence = GLL;
  else if (type.data[0] == 'G' && type.data[1] == 'S' && type.data[2] == 'A')
    sentence = GSA;
  return sentence;
}

/**************************************************************************/
/*!
    @brief Decode a GGA, RMC, or GLL position without retaining receiver state.
    @param type Three-character sentence type without its talker prefix.
    @param fields Borrowed fields after the address comma and before '*'.
    @return Independent values and validation diagnostics. Invalid or
    unsupported input returns no partially decoded measurements.

    Call after validating the enclosing frame with Adafruit_NMEA::validate().
    Input storage must remain readable and unchanged throughout the call.
    Validate all consumed navigation fields before returning measurements,
    including fields this result does not expose (such as altitude).
    Optional tails retain validateNavigation()'s existing behavior.

    Populated coordinates retain all nine fractional-minute digits and can be
    passed directly to formatCoordinate(). Empty fields have EMPTY status;
    fields absent from this sentence type have MISSING status. The fix boolean
    is meaningful only with VALID fixStatus and does not imply populated
    coordinates. GGA quality is independent of RMC/GLL fix validity.

    Time/date and exact position components belong only to this sentence. No
    timestamps, previous-fix merging, allocation, or floating-point conversion
    occur here. GSA and other sentence types are UNSUPPORTED by this decoder.
*/
/**************************************************************************/
gnss_position_t Adafruit_GNSS::parsePosition(nmea_span_t type,
                                             nmea_span_t fields) {
  gnss_position_t result = {};
  result.validation.status = GNSS_SENTENCE_UNSUPPORTED;
  result.latitude.status = result.longitude.status = NMEA_NUMBER_MISSING;
  result.time.status = result.date.status = NMEA_NUMBER_MISSING;
  result.fixStatus = result.fixQualityStatus = NMEA_NUMBER_MISSING;
  if (!type.data || type.length != 3)
    return result;
  uint8_t sentence = sentenceType(type);
  if (sentence != GGA && sentence != RMC && sentence != GLL)
    return result;
  gnss_validation_t validation = decodeNavigation(sentence, fields, &result);
  if (validation.status != GNSS_SENTENCE_VALID) {
    // Discard every tentative value, including fields before the error.
    result = {};
    result.latitude.status = result.longitude.status = NMEA_NUMBER_MISSING;
    result.time.status = result.date.status = NMEA_NUMBER_MISSING;
    result.fixStatus = result.fixQualityStatus = NMEA_NUMBER_MISSING;
  }
  result.validation = validation;
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

/**************************************************************************/
/*!
  @file Adafruit_NMEA.cpp
  @brief Hardware-independent NMEA sentence and field utilities.

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#include "Adafruit_NMEA.h"
#include <string.h>

/**************************************************************************/
/*!
    @brief Create a receiver using two caller-owned buffers.
    @param firstBuffer First writable receive buffer.
    @param secondBuffer Second writable receive buffer, not overlapping first.
    @param capacity Size of each buffer, including space for a NUL terminator.

    Both buffers must outlive the receiver and remain exclusively owned by it.
    NULL pointers, overlapping buffers, or capacity below two bytes disable
    reception: feed() returns BAD_FORMAT and no buffer is written. Valid
    buffers are initialized to empty strings. No memory is allocated.
*/
/**************************************************************************/
Adafruit_NMEA::Adafruit_NMEA(volatile char *firstBuffer,
                             volatile char *secondBuffer, size_t capacity)
    : _buffer(firstBuffer), _lastBuffer(secondBuffer), _capacity(capacity) {
  if (!firstBuffer || !secondBuffer || capacity < 2) {
    _capacity = 0;
  } else {
    // Subtract addresses instead of adding capacity to avoid end overflow.
    uintptr_t firstAddress = (uintptr_t)firstBuffer;
    uintptr_t secondAddress = (uintptr_t)secondBuffer;
    if (firstAddress <= secondAddress) {
      if (secondAddress - firstAddress < capacity)
        _capacity = 0;
    } else if (firstAddress - secondAddress < capacity) {
      _capacity = 0;
    }
  }
  reset();
}

/**************************************************************************/
/*!
    @brief Discard partial and completed lines and clear all timestamps.

    Invalidates every view returned by lastSentence(). Valid receive buffers
    become empty strings. Invalid storage remains disabled and untouched.
*/
/**************************************************************************/
void Adafruit_NMEA::reset() {
  _length = _lastLength = 0;
  _startedAt = _lastStarted = _lastReceived = 0;
  if (_capacity) {
    _buffer[0] = '\0';
    _lastBuffer[0] = '\0';
  }
}

/**************************************************************************/
/*!
    @brief Feed one byte into this receiver's bounded sentence buffer.
    @param byte Received byte, including any CR/LF characters.
    @param receivedAtMs Receive time in milliseconds, such as millis().
    @return INCOMPLETE until LF completes a line, then its validation status.
    OVERFLOW discards a line that cannot fit including LF and NUL. BAD_FORMAT
    also indicates invalid storage supplied to the constructor.

    Ignore bytes before '$' or '!'. Either start marker restarts assembly,
    including after overflow. Overflow is reported once; subsequent bytes
    are ignored until another start marker. Partial and overflowing lines
    preserve the previous complete line and its timestamps.

    Every complete, non-overflowed line replaces lastSentence(), even if its
    format or checksum is invalid. Its raw text includes LF and is followed
    by a NUL terminator. The caller must consume each line promptly; there is
    no queue. No transport access, allocation, or sentence copying occurs.
*/
/**************************************************************************/
nmea_frame_status_t Adafruit_NMEA::feed(uint8_t byte, uint32_t receivedAtMs) {
  if (!_capacity)
    return NMEA_FRAME_BAD_FORMAT;

  if (byte == '$' || byte == '!') {
    _length = 0;
    _startedAt = receivedAtMs;
  } else if (!_length) {
    return NMEA_FRAME_INCOMPLETE;
  }

  // Reserve one byte for NUL, including when the incoming byte is LF.
  if (_length == _capacity - 1) {
    _length = 0;
    _buffer[0] = '\0';
    return NMEA_FRAME_OVERFLOW;
  }
  _buffer[_length++] = (char)byte;
  _buffer[_length] = '\0';
  if (byte != '\n')
    return NMEA_FRAME_INCOMPLETE;

  // Publish by swapping buffers, leaving the new completed line undisturbed
  // while the next line is assembled.
  volatile char *previous = _lastBuffer;
  _lastBuffer = _buffer;
  _buffer = previous;
  _lastLength = _length;
  _lastStarted = _startedAt;
  _lastReceived = receivedAtMs;
  _length = 0;
  _buffer[0] = '\0';
  return validate((const char *)_lastBuffer, _lastLength).status;
}

/**************************************************************************/
/*!
    @brief Get the latest complete raw line without validating it again.
    @return Borrowed text and its length, or an absent span before completion.

    Includes invalid lines and their LF, with a NUL after the stored length.
    The view expires on the next completed line or reset(). As with
    lastSentence(), callers must synchronize access with feed().
*/
/**************************************************************************/
nmea_span_t Adafruit_NMEA::lastText() const {
  nmea_span_t text = {NULL, 0};
  if (_lastLength) {
    text.data = (const char *)_lastBuffer;
    text.length = _lastLength;
  }
  return text;
}

/**************************************************************************/
/*!
    @brief Get views of the latest complete line, including invalid raw text.
    @return Validated sentence, or INCOMPLETE with absent spans before a line
    has completed or after reset(). Overflow does not replace this result.

    Views borrow the receive buffer and expire on the next complete line,
    reset(), or receiver destruction. Address and fields are only available
    for VALID lines. Validation uses the stored length, not the NUL terminator.
*/
/**************************************************************************/
nmea_sentence_t Adafruit_NMEA::lastSentence() const {
  if (_lastLength)
    return validate((const char *)_lastBuffer, _lastLength);
  nmea_sentence_t result = {
      NMEA_FRAME_INCOMPLETE, {NULL, 0}, {NULL, 0}, {NULL, 0}};
  return result;
}

/**************************************************************************/
/*!
    @brief Get the start-marker timestamp of the latest complete line.
    @return Supplied receive time in milliseconds, or zero before a line exists.
    Zero is also a valid timestamp; use lastSentence().status to distinguish it.
*/
/**************************************************************************/
uint32_t Adafruit_NMEA::sentenceStartedAt() const { return _lastStarted; }

/**************************************************************************/
/*!
    @brief Get the LF timestamp of the latest complete line.
    @return Supplied receive time in milliseconds, or zero before a line exists.
    Timestamps retain the caller's uint32_t wraparound behavior.
*/
/**************************************************************************/
uint32_t Adafruit_NMEA::sentenceReceivedAt() const { return _lastReceived; }

/**************************************************************************/
/*!
    @brief Validate a complete, length-bounded NMEA sentence without copying it.
    @param data Readable input buffer; no NUL terminator is required.
    @param length Number of bytes to inspect, excluding any NUL terminator.
    @return Status and borrowed spans. Address and fields are absent on failure.
    Raw text remains available unless data is NULL, which gives absent spans.

    Accepts '$' or '!', an ASCII alphanumeric address, optional comma-separated
    fields, '*', two hex checksum digits, optional CR, and optional LF, in that
    order. A valid but unknown address is accepted. Embedded start markers,
    control characters, non-ASCII bytes, and extra trailer bytes are rejected.
    Missing or invalid hex digits are BAD_FORMAT; a mismatch is BAD_CHECKSUM.

    A zero-field message such as "$PQTMVERNO*58" has absent fields. A comma
    immediately followed by '*' denotes one empty field, with non-NULL data
    and zero length. Returned views require the input to remain unchanged.
    This function allocates no memory and has no shared or per-instance state.
*/
/**************************************************************************/
nmea_sentence_t Adafruit_NMEA::validate(const char *data, size_t length) {
  nmea_sentence_t result = {
      NMEA_FRAME_BAD_FORMAT, {NULL, 0}, {NULL, 0}, {NULL, 0}};
  if (!data)
    return result;
  result.text.data = data;
  result.text.length = length;

  // The shortest sentence has a start, one address character, '*', and two hex
  // digits. Check the length before reading even the start character.
  if (length < 5 || (data[0] != '$' && data[0] != '!'))
    return result;

  size_t addressEnd = 0;
  size_t checksumOffset = 0;
  uint8_t checksum = 0;
  for (size_t i = 1; i < length; i++) {
    uint8_t c = (uint8_t)data[i];
    if (c == '*') {
      checksumOffset = i;
      break;
    }
    if (c < ' ' || c > '~' || c == '$' || c == '!')
      return result;

    if (!addressEnd) {
      if (c == ',') {
        addressEnd = i;
      } else if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9'))) {
        return result;
      }
    }
    checksum ^= c;
  }

  if (!checksumOffset)
    return result;
  if (!addressEnd)
    addressEnd = checksumOffset;
  if (addressEnd == 1 || length - checksumOffset < 3)
    return result;

  // Decode both checksum digits only after proving that both are in bounds.
  uint8_t expected = 0;
  for (size_t i = checksumOffset + 1; i < checksumOffset + 3; i++) {
    char c = data[i];
    uint8_t digit;
    if (c >= '0' && c <= '9')
      digit = c - '0';
    else if (c >= 'A' && c <= 'F')
      digit = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
      digit = c - 'a' + 10;
    else
      return result;
    expected = expected * 16 + digit;
  }

  size_t end = checksumOffset + 3;
  if (end < length && data[end] == '\r')
    end++;
  if (end < length && data[end] == '\n')
    end++;
  if (end != length)
    return result;
  if (checksum != expected) {
    result.status = NMEA_FRAME_BAD_CHECKSUM;
    return result;
  }

  result.status = NMEA_FRAME_VALID;
  result.address.data = data + 1;
  result.address.length = addressEnd - 1;
  if (addressEnd < checksumOffset) {
    result.fields.data = data + addressEnd + 1;
    result.fields.length = checksumOffset - addressEnd - 1;
  }
  return result;
}

/**************************************************************************/
/*!
    @brief Return the next field and advance a bounded field cursor.
    @param remaining Start with a copy of a VALID sentence's fields span.
    Updated in place to refer to the fields after the next comma, or to an
    absent span after the last field. Its input storage must remain readable.
    @return Borrowed field text, excluding the comma. Non-NULL data with zero
    length means an empty field; NULL data means there are no fields left.

    Leading, consecutive, and trailing commas preserve empty fields. An absent
    cursor stays absent on subsequent calls. No text is copied or modified,
    and no NUL terminator is required. Only the supplied length is inspected.
    Iterate each cursor in order for one forward scan of its fields.

    To mark a cursor exhausted, set remaining.data to NULL. Setting only
    remaining.length to zero still yields one empty field if data is non-NULL.
*/
/**************************************************************************/
nmea_span_t Adafruit_NMEA::nextField(nmea_span_t &remaining) {
  nmea_span_t field = {NULL, 0};
  if (!remaining.data) {
    remaining.length = 0;
    return field;
  }

  field.data = remaining.data;
  while (field.length < remaining.length &&
         remaining.data[field.length] != ',') {
    field.length++;
  }

  if (field.length < remaining.length) {
    // Keep a non-NULL cursor after a trailing comma: one empty field remains.
    remaining.data += field.length + 1;
    remaining.length -= field.length + 1;
  } else {
    remaining.data = NULL;
    remaining.length = 0;
  }
  return field;
}

static nmea_number_status_t scanDecimal(nmea_span_t field, uint8_t &places,
                                        bool &negative) {
  if (!field.data)
    return NMEA_NUMBER_MISSING;
  if (!field.length)
    return NMEA_NUMBER_EMPTY;
  size_t start = 0;
  negative = field.data[0] == '-';
  if (negative || field.data[0] == '+')
    start = 1;
  bool decimalPoint = false, hasDigit = false, overflow = false;
  uint8_t significant = 0;
  int8_t comparison = 0;
  places = 0;
  // Compare significant digits with INT64_MAX's magnitude. This also accepts
  // INT64_MIN's extra final digit without requiring any 64-bit arithmetic.
  const char *limit = "9223372036854775807";
  for (size_t i = start; i < field.length; i++) {
    char c = field.data[i];
    if (c == '.' && !decimalPoint) {
      decimalPoint = true;
      continue;
    }
    if (c < '0' || c > '9')
      return NMEA_NUMBER_BAD_FORMAT;
    hasDigit = true;
    if (decimalPoint) {
      if (places == UINT8_MAX)
        overflow = true;
      else
        places++;
    }
    if (significant || c != '0') {
      if (significant < 20)
        significant++;
      if (significant <= 19 && !comparison) {
        char bound =
            significant == 19 && negative ? '8' : limit[significant - 1];
        if (c != bound)
          comparison = c < bound ? -1 : 1;
      }
    }
  }
  if (!hasDigit)
    return NMEA_NUMBER_BAD_FORMAT;
  if (overflow || significant > 19 || (significant == 19 && comparison > 0))
    return NMEA_NUMBER_OUT_OF_RANGE;
  negative = negative && significant; // Negative zero remains nonnegative.
  return NMEA_NUMBER_VALID;
}

/**************************************************************************/
/*!
    @brief Convert a complete field to an exact signed decimal value.
    @param field Borrowed readable text, with no NUL terminator required.
    @return Status, integer coefficient, and decimal-place count. Both numeric
    members are zero on failure. NULL data is MISSING; zero length is EMPTY.

    Accepts an optional sign, digits, and at most one decimal point. At least
    one digit is required. Whitespace, exponents, and non-digit suffixes are
    rejected. The supplied fractional digit count is retained, including zeros.
    Coefficients must fit int64_t and decimal-place counts must fit uint8_t.
    Malformed syntax takes precedence over overflow. No rounding, floating-point
    conversion, allocation, or input modification occurs.
*/
/**************************************************************************/
nmea_decimal_t Adafruit_NMEA::parseDecimal(nmea_span_t field) {
  nmea_decimal_t result = {NMEA_NUMBER_BAD_FORMAT, 0, 0};
  uint8_t places = 0;
  bool negative = false;
  result.status = scanDecimal(field, places, negative);
  if (result.status != NMEA_NUMBER_VALID)
    return result;
  // The complete field is already checked, so accumulation cannot overflow.
  // Accumulate negatively because INT64_MIN has no positive counterpart.
  int64_t coefficient = 0;
  for (size_t i = 0; i < field.length; i++) {
    char c = field.data[i];
    if (c >= '0' && c <= '9')
      coefficient = coefficient * 10 - (c - '0');
  }
  result.coefficient = negative ? coefficient : -coefficient;
  result.decimalPlaces = places;
  return result;
}

/**************************************************************************/
/*!
    @brief Check an exact decimal without constructing its numeric value.
    @param field Borrowed readable text, with no NUL terminator required.
    @param allowNegative Whether to accept negative nonzero values.
    @return The same syntax and range status as parseDecimal(). Negative
    nonzero values give OUT_OF_RANGE when allowNegative is false.

    Retains parseDecimal's signed 64-bit coefficient and fractional digit
    limits, including malformed-syntax precedence. Uses no 64-bit arithmetic
    or allocation. Negative zero is accepted in either mode.
*/
/**************************************************************************/
nmea_number_status_t Adafruit_NMEA::validateDecimal(nmea_span_t field,
                                                    bool allowNegative) {
  uint8_t places = 0;
  bool negative = false;
  nmea_number_status_t status = scanDecimal(field, places, negative);
  if (status == NMEA_NUMBER_VALID && negative && !allowNegative)
    return NMEA_NUMBER_OUT_OF_RANGE;
  return status;
}

/**************************************************************************/
/*!
    @brief Build a checksummed command in caller-provided storage.
    @param output Writable buffer for the complete command and NUL terminator.
    @param capacity Size of output in bytes, including space for NUL.
    @param body Readable address and optional comma-separated fields, without
    '$', '!', '*', control characters, or non-ASCII bytes. No NUL is required.
    @param bodyLength Number of bytes in body, excluding any NUL terminator.
    @return Bytes written excluding NUL, or zero on invalid input, overlapping
    buffers, or insufficient capacity. On failure, output[0] is cleared if
   output is non-NULL and capacity is nonzero; no other output bytes are
   changed.

    Produces "$<body>*HH\r\n" followed by NUL, using uppercase checksum digits.
    Requires bodyLength + NMEA_COMMAND_OVERHEAD bytes of storage. The address
    must be nonempty and ASCII alphanumeric. Zero-field commands are allowed.
    Arguments, capacity, and body syntax are checked before constructing output.
    No heap allocation or transport I/O occurs.
*/
/**************************************************************************/
size_t Adafruit_NMEA::buildCommand(char *output, size_t capacity,
                                   const char *body, size_t bodyLength) {
  bool valid = output && body && bodyLength &&
               capacity >= NMEA_COMMAND_OVERHEAD &&
               bodyLength <= capacity - NMEA_COMMAND_OVERHEAD;
  if (valid) {
    // Compare address differences so checking overlap cannot overflow an end
    // address. Reject overlap with the caller's entire writable output span.
    uintptr_t outputAddress = (uintptr_t)output;
    uintptr_t bodyAddress = (uintptr_t)body;
    if (outputAddress <= bodyAddress)
      valid = bodyAddress - outputAddress >= capacity;
    else
      valid = outputAddress - bodyAddress >= bodyLength;
  }

  uint8_t checksum = 0;
  bool inAddress = true;
  if (valid) {
    for (size_t i = 0; i < bodyLength; i++) {
      uint8_t c = (uint8_t)body[i];
      if (c < ' ' || c > '~' || c == '$' || c == '!' || c == '*') {
        valid = false;
        break;
      }
      if (inAddress) {
        if (c == ',' && i > 0) {
          inAddress = false;
        } else if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9'))) {
          valid = false;
          break;
        }
      }
      checksum ^= c;
    }
  }
  if (!valid) {
    if (output && capacity)
      output[0] = '\0';
    return 0;
  }

  output[0] = '$';
  memcpy(output + 1, body, bodyLength);
  size_t end = bodyLength + 1;
  output[end++] = '*';
  uint8_t digits[] = {(uint8_t)(checksum / 16), (uint8_t)(checksum % 16)};
  for (uint8_t i = 0; i < 2; i++) {
    if (digits[i] < 10)
      output[end++] = '0' + digits[i];
    else
      output[end++] = 'A' + digits[i] - 10;
  }
  output[end++] = '\r';
  output[end++] = '\n';
  output[end] = '\0';
  return end;
}

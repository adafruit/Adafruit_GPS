/**************************************************************************/
/*!
  @file Adafruit_NMEA.cpp
  @brief Hardware-independent NMEA sentence validation.

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#include "Adafruit_NMEA.h"

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

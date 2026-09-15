/**************************************************************************/
/*!
  @file Adafruit_NMEA.h
  @brief Proposed hardware-independent NMEA API for review.

  Declarations only: this proposal is not implemented or installed from src/.
  Private storage and class layout will be defined with the implementation.
  Existing Adafruit_GPS code does not use this header.

  Intended inheritance:
    Adafruit_NMEA -> Adafruit_GNSS -> Adafruit_GPS (existing MTK API)
                                 -> Adafruit_LC29H (Quectel API)

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#ifndef ADAFRUIT_NMEA_H
#define ADAFRUIT_NMEA_H

#include <stddef.h>
#include <stdint.h>

/** Result of framing or validating a sentence. Recognition belongs to GNSS
 *  or receiver-specific decoders: an unknown address can still be VALID. */
typedef enum : uint8_t {
  NMEA_FRAME_INCOMPLETE,   ///< More bytes are needed; no new sentence is ready.
  NMEA_FRAME_VALID,        ///< Complete format and checksum are valid.
  NMEA_FRAME_BAD_FORMAT,   ///< Invalid argument, address, framing, or trailer.
  NMEA_FRAME_BAD_CHECKSUM, ///< Two hex checksum digits do not match the
                           ///< payload.
  NMEA_FRAME_OVERFLOW ///< Receive buffer has no room for the whole sentence.
} nmea_frame_status_t;

/** Borrowed text with an explicit length; not necessarily NUL-terminated.
 *  NULL data means absent. Non-NULL data with length zero means present/empty.
 *  The owner must keep the underlying storage alive and unchanged while used.
 */
typedef struct {
  const char *data; ///< First character, or NULL for an absent field.
  size_t length;    ///< Number of characters, excluding any NUL terminator.
} nmea_span_t;

/** Sentence views. Only text is available when a complete sentence is invalid;
 *  address and fields require VALID status. No text is copied or whitelisted.
 */
typedef struct {
  nmea_frame_status_t status; ///< Result of validation.
  nmea_span_t text;    ///< Entire supplied sentence, including its trailer.
  nmea_span_t address; ///< Full address, e.g. GNGGA, PAIR001, or PQTMVERNO.
  nmea_span_t fields;  ///< Text after the first comma and before '*'.
} nmea_sentence_t;

/// Result of converting one complete field to a decimal number.
typedef enum : uint8_t {
  NMEA_NUMBER_VALID,       ///< The complete field is a representable decimal.
  NMEA_NUMBER_MISSING,     ///< Field is absent.
  NMEA_NUMBER_EMPTY,       ///< Field is present but empty.
  NMEA_NUMBER_BAD_FORMAT,  ///< Invalid character or decimal syntax.
  NMEA_NUMBER_OUT_OF_RANGE ///< Coefficient or decimal-place count overflowed.
} nmea_number_status_t;

/** Exact decimal value: coefficient / 10^decimalPlaces, with no float step.
 *  Coordinate interpretation and conversion to degrees belong to Adafruit_GNSS.
 *  On any conversion failure, coefficient and decimalPlaces are both zero. */
typedef struct {
  nmea_number_status_t status; ///< Result of conversion.
  int64_t coefficient;   ///< Signed integer containing the decimal digits.
  uint8_t decimalPlaces; ///< Number of digits following the decimal point.
} nmea_decimal_t;

/** Proposed framing and field utilities shared by receiver classes.
 *
 *  No transport, GPS fix data, command waits, heap allocation, or field arrays.
 *  Framing state and timestamps belong to each instance. Supplied-buffer
 *  validation, field iteration, conversion, and command building are stateless.
 *  One caller feeds each instance; concurrent access requires synchronization.
 */
class Adafruit_NMEA {
public:
  // Borrow two non-overlapping buffers, each capacity bytes including NUL.
  // They must outlive this object. This permits reuse of GPS's existing pair.
  // Invalid storage makes feed() return BAD_FORMAT without writing to it.
  Adafruit_NMEA(char *firstBuffer, char *secondBuffer, size_t capacity);

  // Copies must not share writable receive buffers with independent state.
  Adafruit_NMEA(const Adafruit_NMEA &) = delete;
  Adafruit_NMEA &operator=(const Adafruit_NMEA &) = delete;

  // Drop receive state and invalidate all views returned by lastSentence().
  void reset();

  // Supply one byte and its receive time in milliseconds (e.g. millis()).
  // Ignore bytes before '$' or '!'; a new start character restarts assembly.
  // LF completes the sentence; its format and checksum determine the result.
  // Overflow discards that sentence and resumes at the next start character.
  // Each complete, non-overflowed line replaces lastSentence(), even if
  // invalid. Retaining invalid raw text permits the existing lastNMEA()/logger
  // behavior.
  nmea_frame_status_t feed(uint8_t byte, uint32_t receivedAtMs);

  // Return the latest complete line, or INCOMPLETE/absent spans before one.
  // Views expire on the next complete line, reset(), or destruction.
  // Process each line promptly: there is no queue of older sentences.
  nmea_sentence_t lastSentence() const;

  // Time of the '$' or '!' of the latest complete sentence; zero before one
  // exists. Zero is also a valid timestamp; use lastSentence().status to tell
  // them apart.
  uint32_t sentenceStartedAt() const;

  // Time of the LF of that same sentence; unsigned times wrap like millis().
  uint32_t sentenceReceivedAt() const;

  // Validate exactly length bytes, excluding NUL; never modify or overread
  // data. Accept '$' or '!', a nonempty ASCII alphanumeric address, optional
  // fields,
  // '*', two hex checksum digits, then optional CR followed by optional LF.
  // Reject embedded NUL/control characters and extra trailer characters.
  // Missing/invalid hex digits are BAD_FORMAT; a mismatch is BAD_CHECKSUM.
  // Zero fields means absent fields, e.g. "$PQTMVERNO*58".
  // One empty field means non-NULL fields.data with zero length.
  // NULL data yields BAD_FORMAT with all spans absent.
  // Returned spans borrow data; receive state and timestamps are unchanged.
  static nmea_sentence_t validate(const char *data, size_t length);

  // Start with a copy of a VALID sentence's fields span. Return the next field
  // and advance remaining in place. Preserve empty and trailing empty fields.
  // After the last field, remaining and subsequent results are absent.
  // This supports one forward scan without a stored array of field offsets.
  static nmea_span_t nextField(nmea_span_t &remaining);

  // Convert a whole field: optional '+'/'-', then digits and at most one '.'.
  // At least one digit is required; whitespace, exponents, and NaN are invalid.
  // Retain the supplied fractional digit count; never round or silently clamp.
  static nmea_decimal_t parseDecimal(nmea_span_t field);

  // Build "$<body>*HH\r\n" plus NUL into caller-provided storage.
  // body contains an address and optional fields, without '$', '!', '*', NUL,
  // or control characters. Source/output must not overlap.
  // capacity includes NUL; return bytes excluding NUL, or zero on error.
  // On error, clear output[0] if output is non-NULL and capacity is nonzero.
  // Check arguments and capacity before writing; no heap allocation or I/O.
  static size_t buildCommand(char *output, size_t capacity, const char *body,
                             size_t bodyLength);
};

#endif // ADAFRUIT_NMEA_H

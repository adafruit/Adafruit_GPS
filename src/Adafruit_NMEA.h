/**************************************************************************/
/*!
  @file Adafruit_NMEA.h
  @brief Hardware-independent NMEA sentence and field utilities.

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#ifndef ADAFRUIT_NMEA_H
#define ADAFRUIT_NMEA_H

#include <stddef.h>
#include <stdint.h>

#define NMEA_COMMAND_OVERHEAD 7 ///< Bytes for '$', '*HH', CR/LF, and NUL.

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

/** Sentence views. Only text is available when a sentence is invalid;
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

/** Hardware-independent NMEA framing and utilities shared by receiver classes.
 *  Receive storage is borrowed, with no heap allocation. Each instance has its
 *  own receive state and timestamps; callers must synchronize concurrent use.
 */
class Adafruit_NMEA {
public:
  Adafruit_NMEA(volatile char *firstBuffer, volatile char *secondBuffer,
                size_t capacity);
  /// @brief Copying is disabled to prevent sharing writable receive buffers.
  /// @param other Receiver that cannot be copied.
  Adafruit_NMEA(const Adafruit_NMEA &other) = delete;
  /// @brief Assignment is disabled to prevent sharing writable receive buffers.
  /// @param other Receiver that cannot be assigned.
  /// @return No value; this deleted operation cannot be called.
  Adafruit_NMEA &operator=(const Adafruit_NMEA &other) = delete;
  void reset();
  nmea_frame_status_t feed(uint8_t byte, uint32_t receivedAtMs);
  nmea_span_t lastText() const;
  nmea_sentence_t lastSentence() const;
  uint32_t sentenceStartedAt() const;
  uint32_t sentenceReceivedAt() const;

  static nmea_sentence_t validate(const char *data, size_t length);
  static nmea_span_t nextField(nmea_span_t &remaining);
  static nmea_decimal_t parseDecimal(nmea_span_t field);
  static nmea_number_status_t validateDecimal(nmea_span_t field,
                                              bool allowNegative = true);
  static size_t buildCommand(char *output, size_t capacity, const char *body,
                             size_t bodyLength);

private:
  volatile char *_buffer;     ///< Buffer currently receiving bytes.
  volatile char *_lastBuffer; ///< Buffer holding the latest complete line.
  size_t _capacity;       ///< Bytes per buffer, or zero for invalid storage.
  size_t _length;         ///< Current line length; zero while awaiting a start.
  size_t _lastLength;     ///< Latest complete line length, excluding NUL.
  uint32_t _startedAt;    ///< Start time of the line being assembled.
  uint32_t _lastStarted;  ///< Start time of the latest complete line.
  uint32_t _lastReceived; ///< LF time of the latest complete line.
};

#endif // ADAFRUIT_NMEA_H

/**************************************************************************/
/*!
  @file Adafruit_NMEA.h
  @brief Hardware-independent NMEA sentence validation.

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

/** Sentence views. Only text is available when a sentence is invalid;
 *  address and fields require VALID status. No text is copied or whitelisted.
 */
typedef struct {
  nmea_frame_status_t status; ///< Result of validation.
  nmea_span_t text;    ///< Entire supplied sentence, including its trailer.
  nmea_span_t address; ///< Full address, e.g. GNGGA, PAIR001, or PQTMVERNO.
  nmea_span_t fields;  ///< Text after the first comma and before '*'.
} nmea_sentence_t;

/// Hardware-independent NMEA utilities shared by receiver classes.
class Adafruit_NMEA {
public:
  static nmea_sentence_t validate(const char *data, size_t length);
};

#endif // ADAFRUIT_NMEA_H

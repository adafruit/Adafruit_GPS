/**************************************************************************/
/*!
  @file Adafruit_LC29H.h
  @brief Proposed LC29H receive and reply API for review.

  Declarations only; not implemented or installed from src/. See LC29H.md
  for the protocol references, fixture, and subsequent transport milestone.
  Private layout will be defined with the implementation.

  Written for Adafruit Industries. BSD license; see license.txt.
*/
/**************************************************************************/

#ifndef ADAFRUIT_LC29H_H
#define ADAFRUIT_LC29H_H

// Use the implemented GNSS/NMEA types, not the older proposal beside this file.
#include "../../src/Adafruit_GNSS.h"

/// Decoding outcome, independent of whether a command was accepted.
typedef enum : uint8_t {
  LC29H_REPLY_INVALID_FRAME,  ///< Incomplete or invalid NMEA frame.
  LC29H_REPLY_UNSUPPORTED,    ///< Valid frame with a different address/marker.
  LC29H_REPLY_INVALID_FIELDS, ///< Recognized address with malformed payload.
  LC29H_REPLY_VALID,          ///< Supported response payload decoded.
  LC29H_REPLY_RECEIVER_ERROR  ///< Well-formed PQTM error response decoded.
} lc29h_reply_status_t;

/// PAIR001 result codes; UNKNOWN is a library sentinel, not a wire value.
typedef enum : uint8_t {
  LC29H_PAIR_ACCEPTED = 0,    ///< Positive acknowledgment; not a fix/readback.
  LC29H_PAIR_PROCESSING = 1,  ///< Wait for a subsequent result.
  LC29H_PAIR_FAILED = 2,      ///< Command failed.
  LC29H_PAIR_UNSUPPORTED = 3, ///< Command ID is unsupported.
  LC29H_PAIR_PARAMETER_ERROR = 4, ///< Receiver rejected command parameters.
  LC29H_PAIR_BUSY = 5,            ///< Receiver is busy.
  LC29H_PAIR_UNKNOWN = 255        ///< No recognized result was decoded.
} lc29h_pair_result_t;

/** Owned acknowledgment; no pointers into the receive buffer.
 *  Check status before using commandID or result. VALID includes rejection
 *  and PROCESSING responses; it does not mean the requested operation finished.
 *  Any decoding failure leaves commandID zero and result UNKNOWN. */
typedef struct {
  lc29h_reply_status_t status; ///< Frame/address/field decoding outcome.
  uint16_t commandID;          ///< Acknowledged PAIR ID, 0 through 999.
  lc29h_pair_result_t result;  ///< Decoded receiver result.
} lc29h_pair_ack_t;

/** Borrowed firmware text, without fixed string limits or numeric conversion.
 *  Success populates three nonempty spans and leaves errorCode zero. A decoded
 *  receiver error populates only errorCode. Other failures clear every value.
 *  Text is not NUL-terminated; copy it before its source sentence expires. */
typedef struct {
  lc29h_reply_status_t status; ///< VALID, RECEIVER_ERROR, or decoding failure.
  nmea_span_t version;   ///< Firmware identity text; not a capability map.
  nmea_span_t buildDate; ///< Opaque firmware build date text.
  nmea_span_t buildTime; ///< Opaque firmware build time text.
  uint8_t errorCode;     ///< PQTM error, 1 through 255; zero otherwise.
} lc29h_version_t;

/** Proposed LC29H-specific decoder above the shared precision-safe GNSS core.
 *  Inherits feed(), lastSentence(), timestamps, reset(), and lastPosition().
 *  No extra receive buffers, fix cache, heap allocation, or transport reader.
 *  reset() clears parser state only; it does not restart the physical module.
 */
class Adafruit_LC29H : public Adafruit_GNSS {
public:
  // Borrow two non-overlapping buffers, each capacity bytes including NUL.
  // Buffers must outlive this object. Invalid storage disables feed(), as in
  // Adafruit_NMEA. Copying remains disabled by the base class.
  Adafruit_LC29H(volatile char *firstBuffer, volatile char *secondBuffer,
                 size_t capacity);

  // Decode only the latest complete line, without caching an earlier reply.
  // Before any complete line: INVALID_FRAME. After navigation or another reply
  // type: UNSUPPORTED. Invalid completed lines replace earlier replies too.
  lc29h_pair_ack_t lastPairAck() const;
  lc29h_version_t lastVersion() const;

  // Accept a view produced by validate()/lastSentence(), kept unchanged while
  // decoding. As in parsePosition(), trust the frame status; do not rechecksum.
  // Require '$' and the exact address PAIR001, not a prefix. Require exactly
  // two nonempty unsigned integer fields: ID 0..999 and result 0..5. Leading
  // zeros are allowed; signs, decimals, unknown results, and extra fields are
  // INVALID_FIELDS. Bad fields never yield a partial acknowledgment.
  static lc29h_pair_ack_t parsePairAck(const nmea_sentence_t &sentence);

  // Require '$' and the exact address PQTMVERNO. Accept exactly three nonempty
  // text fields for success, or exactly ERROR,<code> with unsigned code 1..255.
  // Reserve ERROR as the first field; malformed error payloads cannot become
  // version strings. Preserve unknown nonzero error codes for diagnostics.
  // Date/time remain text, without calendar or timezone interpretation.
  // The zero-field query/echo is INVALID_FIELDS, never a successful response.
  // Spans borrow sentence storage. For lastVersion(), they expire on the next
  // complete line, reset(), or destruction. Synchronize concurrent feed/read.
  static lc29h_version_t parseVersion(const nmea_sentence_t &sentence);
};

#endif // ADAFRUIT_LC29H_H

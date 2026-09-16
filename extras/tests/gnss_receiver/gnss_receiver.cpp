#include "Adafruit_GNSS.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <type_traits>

class Receiver : public Adafruit_GNSS {
public:
  using Adafruit_GNSS::Adafruit_GNSS;
};

static size_t build(char *line, size_t capacity, const char *body) {
  size_t length =
      Adafruit_NMEA::buildCommand(line, capacity, body, strlen(body));
  assert(length);
  return length;
}

static nmea_frame_status_t feed(Adafruit_GNSS &receiver, const char *line,
                                size_t length, uint32_t start) {
  nmea_frame_status_t status = NMEA_FRAME_INCOMPLETE;
  for (size_t i = 0; i < length; i++)
    status = receiver.feed(line[i], start + (uint32_t)i);
  return status;
}

static void noPosition(const gnss_position_t &position,
                       gnss_sentence_status_t status) {
  assert(position.validation.status == status);
  assert(position.latitude.status == NMEA_NUMBER_MISSING &&
         position.longitude.status == NMEA_NUMBER_MISSING &&
         position.time.status == NMEA_NUMBER_MISSING &&
         position.date.status == NMEA_NUMBER_MISSING &&
         position.fixStatus == NMEA_NUMBER_MISSING &&
         position.fixQualityStatus == NMEA_NUMBER_MISSING);
  assert(!position.latitude.fractionalMinutes &&
         !position.longitude.degreesE7 && !position.fix &&
         !position.fixQuality);
}

int main() {
  static_assert(std::is_base_of<Adafruit_NMEA, Adafruit_GNSS>::value,
                "GNSS shares the NMEA receiver");
  static_assert(sizeof(Adafruit_GNSS) == sizeof(Adafruit_NMEA),
                "GNSS must not add persistent receive or fix storage");
  static_assert(!std::is_copy_constructible<Adafruit_GNSS>::value &&
                    !std::is_copy_assignable<Adafruit_GNSS>::value,
                "Receivers must not share writable buffers through copies");
  char first[128], second[128], third[128], fourth[128];
  Receiver one(first, second, sizeof(first)), two(third, fourth, sizeof(third));
  noPosition(one.lastPosition(), GNSS_SENTENCE_INVALID_FRAME);
  Adafruit_GNSS disabled;
  assert(disabled.feed('$', 1) == NMEA_FRAME_BAD_FORMAT);
  noPosition(disabled.lastPosition(), GNSS_SENTENCE_INVALID_FRAME);

  char rmc[128], gga[128];
  size_t rmcLength =
      build(rmc, sizeof(rmc),
            "GNRMC,123519.123,A,8959.123456789,S,17959.123456789,W,1,2,290224");
  size_t ggaLength =
      build(gga, sizeof(gga),
            "GPGGA,010203,4807.038,N,01131.000,E,4,08,0.9,545.4,M,46.9");
  uint32_t start = UINT32_MAX - 10;
  for (size_t i = 0; i < rmcLength || i < ggaLength; i++) {
    if (i < rmcLength)
      one.feed(rmc[i], start + (uint32_t)i);
    if (i < ggaLength)
      two.feed(gga[i], 500 + (uint32_t)i);
  }
  gnss_position_t saved = one.lastPosition();
  assert(saved.validation.status == GNSS_SENTENCE_VALID && saved.fix &&
         saved.latitude.fractionalMinutes == 123456789 && saved.date.day == 29);
  gnss_position_t other = two.lastPosition();
  assert(other.validation.status == GNSS_SENTENCE_VALID &&
         other.fixQuality == 4 && other.latitude.degreesE7 == 481173000 &&
         other.time.hour == 1);
  assert(one.sentenceStartedAt() == start &&
         one.sentenceReceivedAt() == start + (uint32_t)rmcLength - 1 &&
         two.sentenceStartedAt() == 500 &&
         two.sentenceReceivedAt() == 500 + ggaLength - 1);
  assert(one.lastPosition().longitude.fractionalMinutes == 123456789);
  assert(!memcmp(one.lastText().data, rmc, rmcLength));
  char coordinate[GNSS_COORDINATE_TEXT_SIZE];
  assert(Adafruit_GNSS::formatCoordinate(coordinate, sizeof(coordinate),
                                         saved.longitude));
  assert(!strcmp(coordinate, "-179.98539094648"));
  puts("PASS: subclass reception, interleaved receivers, exact output, and "
       "wraparound times");

  assert(one.feed('$', 1000) == NMEA_FRAME_INCOMPLETE);
  assert(one.feed('G', 1001) == NMEA_FRAME_INCOMPLETE);
  assert(one.lastPosition().latitude.fractionalMinutes == 123456789);
  unsigned overflows = 0;
  for (size_t i = 0; i < sizeof(first) + 5; i++)
    overflows += one.feed('A', 1002 + i) == NMEA_FRAME_OVERFLOW;
  assert(overflows == 1 && one.sentenceStartedAt() == start);
  assert(one.lastPosition().longitude.fractionalMinutes == 123456789);
  assert(feed(one, gga, ggaLength, 2000) == NMEA_FRAME_VALID);
  assert(one.lastPosition().fixQuality == 4);
  assert(saved.longitude.fractionalMinutes == 123456789 &&
         saved.date.day == 29);
  puts("PASS: partial/oversized input preserves the last line and recovery "
       "replaces it");

  char line[128];
  const char *unsupported[] = {"PQTMVERNO",    "PAIR001,062,0", "PGGLL,,,,,,A",
                               "GPGSV,1,1,00", "GPRMCX,1",      "GLL,1",
                               "gPGLL,,,,,,A", "1PGLL,,,,,,A"};
  for (const char *body : unsupported) {
    size_t length = build(line, sizeof(line), body);
    assert(feed(one, line, length, 3000) == NMEA_FRAME_VALID);
    noPosition(one.lastPosition(), GNSS_SENTENCE_UNSUPPORTED);
    assert(one.lastSentence().status == NMEA_FRAME_VALID &&
           !memcmp(one.lastText().data, line, length));
  }
  const char *talkers[] = {"GP", "GN", "GL", "GA", "GB", "GQ", "GI", "II"};
  for (const char *talker : talkers) {
    char body[64];
    snprintf(body, sizeof(body), "%sGLL,4807.038,N,01131.000,E,123519,A",
             talker);
    size_t length = build(line, sizeof(line), body);
    nmea_sentence_t sentence = Adafruit_NMEA::validate(line, length);
    assert(Adafruit_GNSS::parsePosition(sentence).validation.status ==
           GNSS_SENTENCE_VALID);
    line[0] = '!'; // The start marker is outside the checksum.
    sentence = Adafruit_NMEA::validate(line, length);
    assert(sentence.status == NMEA_FRAME_VALID);
    noPosition(Adafruit_GNSS::parsePosition(sentence),
               GNSS_SENTENCE_UNSUPPORTED);
  }
  puts("PASS: standard talkers route to positions; replies remain available to "
       "subclasses");

  size_t length =
      build(line, sizeof(line), "GPGLL,4807.038,N,01131.000,E,123519,Q");
  assert(feed(one, line, length, 4000) == NMEA_FRAME_VALID);
  noPosition(one.lastPosition(), GNSS_SENTENCE_INVALID_FIELD);
  assert(one.lastPosition().validation.field == 6);
  line[length - 4] = line[length - 4] == '0' ? '1' : '0';
  assert(feed(one, line, length, 5000) == NMEA_FRAME_BAD_CHECKSUM);
  noPosition(one.lastPosition(), GNSS_SENTENCE_INVALID_FRAME);
  assert(one.lastSentence().status == NMEA_FRAME_BAD_CHECKSUM);
  one.reset();
  noPosition(one.lastPosition(), GNSS_SENTENCE_INVALID_FRAME);
  assert(one.sentenceStartedAt() == 0 && one.sentenceReceivedAt() == 0);
  assert(two.lastPosition().fixQuality == 4 && two.sentenceStartedAt() == 500);
  assert(saved.latitude.fractionalMinutes == 123456789);
  puts("PASS: malformed navigation, checksum failure, reset, and owned result "
       "lifetime");
  return 0;
}

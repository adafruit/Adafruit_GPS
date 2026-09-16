#include "Adafruit_GNSS.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static nmea_span_t text(const char *value) {
  nmea_span_t span = {value, value ? strlen(value) : 0};
  return span;
}

static gnss_position_t decode(const char *type, const char *fields) {
  return Adafruit_GNSS::parsePosition(text(type), text(fields));
}

static void rejected(const char *type, const char *fields,
                     gnss_sentence_status_t status, uint8_t field) {
  gnss_position_t value = decode(type, fields);
  assert(value.validation.status == status && value.validation.field == field);
  assert(value.latitude.status == NMEA_NUMBER_MISSING &&
         value.longitude.status == NMEA_NUMBER_MISSING &&
         value.time.status == NMEA_NUMBER_MISSING &&
         value.date.status == NMEA_NUMBER_MISSING &&
         value.fixStatus == NMEA_NUMBER_MISSING &&
         value.fixQualityStatus == NMEA_NUMBER_MISSING);
  assert(!value.latitude.degreesE7 && !value.latitude.degrees &&
         !value.latitude.minutes && !value.latitude.fractionalMinutes &&
         !value.latitude.hemisphere && !value.longitude.degreesE7 &&
         !value.longitude.degrees && !value.longitude.minutes &&
         !value.longitude.fractionalMinutes && !value.longitude.hemisphere);
  assert(!value.fix && !value.fixQuality && !value.time.hour &&
         !value.time.minute && !value.time.second && !value.time.millisecond &&
         !value.date.day && !value.date.month && !value.date.year);
}

int main() {
  gnss_position_t rmc = decode(
      "RMC",
      "123519.123,A,8959.123456789,S,17959.123456789,W,0.01,0.02,290224");
  assert(rmc.validation.status == GNSS_SENTENCE_VALID && !rmc.validation.field);
  assert(rmc.fixStatus == NMEA_NUMBER_VALID && rmc.fix);
  assert(rmc.fixQualityStatus == NMEA_NUMBER_MISSING && !rmc.fixQuality);
  assert(rmc.latitude.status == NMEA_NUMBER_VALID &&
         rmc.longitude.status == NMEA_NUMBER_VALID);
  assert(rmc.latitude.degrees == 89 && rmc.latitude.minutes == 59 &&
         rmc.latitude.fractionalMinutes == 123456789 &&
         rmc.longitude.degrees == 179 && rmc.longitude.minutes == 59 &&
         rmc.longitude.fractionalMinutes == 123456789);
  assert(rmc.time.status == NMEA_NUMBER_VALID && rmc.time.hour == 12 &&
         rmc.time.minute == 35 && rmc.time.second == 19 &&
         rmc.time.millisecond == 123);
  assert(rmc.date.status == NMEA_NUMBER_VALID && rmc.date.day == 29 &&
         rmc.date.month == 2 && rmc.date.year == 24);
  char output[GNSS_COORDINATE_TEXT_SIZE];
  assert(Adafruit_GNSS::formatCoordinate(output, sizeof(output), rmc.latitude));
  assert(!strcmp(output, "-89.98539094648"));
  assert(
      Adafruit_GNSS::formatCoordinate(output, sizeof(output), rmc.longitude));
  assert(!strcmp(output, "-179.98539094648"));
  puts("PASS: exact RMC coordinates, UTC/date, and GGA-independent fix");

  gnss_position_t gga =
      decode("GGA", "123520,4807.038,N,01131.000,E,4,08,0.9,545.4,M,-46.9");
  assert(gga.validation.status == GNSS_SENTENCE_VALID && gga.fix);
  assert(gga.fixStatus == NMEA_NUMBER_VALID &&
         gga.fixQualityStatus == NMEA_NUMBER_VALID && gga.fixQuality == 4);
  assert(gga.latitude.degreesE7 == 481173000 &&
         gga.longitude.degreesE7 == 115166666 &&
         gga.date.status == NMEA_NUMBER_MISSING);
  gnss_position_t gll = decode("GLL", "4807.038,N,01131.000,E,123521,A");
  assert(gll.validation.status == GNSS_SENTENCE_VALID && gll.fix &&
         gll.fixStatus == NMEA_NUMBER_VALID && gll.time.second == 21 &&
         gll.date.status == NMEA_NUMBER_MISSING &&
         gll.fixQualityStatus == NMEA_NUMBER_MISSING);
  gll = decode("GLL", "4807.038,N,01131.000,E,123521,V");
  assert(gll.validation.status == GNSS_SENTENCE_VALID && !gll.fix &&
         gll.fixStatus == NMEA_NUMBER_VALID &&
         gll.latitude.status == NMEA_NUMBER_VALID);
  assert(rmc.latitude.fractionalMinutes == 123456789 && rmc.time.second == 19);
  puts("PASS: GGA RTK quality, GLL validity, and independent sentence results");

  gnss_position_t empty = decode("RMC", ",V,,,,,,,");
  assert(empty.validation.status == GNSS_SENTENCE_VALID && !empty.fix);
  assert(empty.latitude.status == NMEA_NUMBER_EMPTY &&
         empty.longitude.status == NMEA_NUMBER_EMPTY &&
         empty.time.status == NMEA_NUMBER_EMPTY &&
         empty.date.status == NMEA_NUMBER_EMPTY &&
         empty.fixStatus == NMEA_NUMBER_VALID &&
         empty.fixQualityStatus == NMEA_NUMBER_MISSING);
  empty = decode("GGA", ",,,,,0,00,99.99,,M,");
  assert(empty.validation.status == GNSS_SENTENCE_VALID && !empty.fix &&
         empty.fixStatus == NMEA_NUMBER_VALID &&
         empty.fixQualityStatus == NMEA_NUMBER_VALID && !empty.fixQuality &&
         empty.latitude.status == NMEA_NUMBER_EMPTY);
  empty = decode("GLL", ",,,,,");
  assert(empty.validation.status == GNSS_SENTENCE_VALID &&
         empty.fixStatus == NMEA_NUMBER_EMPTY && !empty.fix);
  empty = decode("GGA", ",,,,,,,,,,");
  assert(empty.validation.status == GNSS_SENTENCE_VALID &&
         empty.fixStatus == NMEA_NUMBER_EMPTY &&
         empty.fixQualityStatus == NMEA_NUMBER_EMPTY && !empty.fix);
  puts("PASS: empty fields, missing sentence-specific fields, and no-fix data");

  rejected("RMC", "123519,A,4807.038,N,01131.000,E,1,2,310426",
           GNSS_SENTENCE_INVALID_FIELD, 9);
  rejected("GGA", "123519,4807.038,N,01131.000,E,1,08,0.9,NaN,M,1",
           GNSS_SENTENCE_INVALID_FIELD, 9);
  rejected("GLL", "4807.038,N,01131.000,N,123519,A",
           GNSS_SENTENCE_INVALID_FIELD, 3);
  rejected("GGA", "123519,4807.038", GNSS_SENTENCE_MISSING_FIELDS, 3);
  rejected("RMC", NULL, GNSS_SENTENCE_MISSING_FIELDS, 1);
  rejected("GSA", "", GNSS_SENTENCE_UNSUPPORTED, 0);
  rejected("VTG", NULL, GNSS_SENTENCE_UNSUPPORTED, 0);
  rejected("GLLX", "", GNSS_SENTENCE_UNSUPPORTED, 0);
  rejected(NULL, NULL, GNSS_SENTENCE_UNSUPPORTED, 0);
  puts("PASS: early and late failures return no partial position data");

  char sentence[120];
  const char *body =
      "GNRMC,123519,A,8959.123456889,S,17959.123456889,W,1,2,290224";
  size_t length = Adafruit_NMEA::buildCommand(sentence, sizeof(sentence), body,
                                              strlen(body));
  nmea_sentence_t frame = Adafruit_NMEA::validate(sentence, length);
  assert(frame.status == NMEA_FRAME_VALID && frame.address.length == 5);
  gnss_position_t copied =
      Adafruit_GNSS::parsePosition({frame.address.data + 2, 3}, frame.fields);
  assert(copied.validation.status == GNSS_SENTENCE_VALID);
  memset(sentence, 'x', sizeof(sentence));
  assert(copied.latitude.degreesE7 == rmc.latitude.degreesE7 &&
         copied.latitude.fractionalMinutes == 123456889);
  assert(
      Adafruit_GNSS::formatCoordinate(output, sizeof(output), copied.latitude));
  assert(!strcmp(output, "-89.98539094815"));
  const char bounded[] = "4807.038,N,01131.000,E,123519,Aignored";
  const char kind[] = {'G', 'L', 'L', 'X'};
  copied =
      Adafruit_GNSS::parsePosition({kind, 3}, {bounded, sizeof(bounded) - 8});
  assert(copied.validation.status == GNSS_SENTENCE_VALID && copied.fix);
  puts("PASS: framed input, owned results, bounded spans, and submillimeter "
       "text");
  return 0;
}

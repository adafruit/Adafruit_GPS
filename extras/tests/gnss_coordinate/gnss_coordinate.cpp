#include "Adafruit_GNSS.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static gnss_coordinate_t decode(const char *text, const char *direction) {
  nmea_span_t coordinate = {text, text ? strlen(text) : 0};
  nmea_span_t hemisphere = {direction, direction ? strlen(direction) : 0};
  return Adafruit_GNSS::parseCoordinate(coordinate, hemisphere);
}

static void invalid(const char *text, const char *direction,
                    nmea_number_status_t status) {
  gnss_coordinate_t value = decode(text, direction);
  assert(value.status == status);
  assert(value.degreesE7 == 0 && value.degrees == 0 && value.minutes == 0 &&
         value.fractionalMinutes == 0 && value.hemisphere == 0);
}

int main() {
  gnss_coordinate_t value = decode("4807.038", "N");
  assert(value.status == NMEA_NUMBER_VALID && value.degreesE7 == 481173000);
  assert(value.degrees == 48 && value.minutes == 7 &&
         value.fractionalMinutes == 38000000 && value.hemisphere == 'N');
  assert(decode("4807.038", "S").degreesE7 == -481173000);
  assert(decode("01131.000", "E").degreesE7 == 115166666);
  assert(decode("01131.000", "W").degreesE7 == -115166666);
  assert(decode("0000", "N").status == NMEA_NUMBER_VALID);
  assert(decode("9000.000", "S").degreesE7 == -900000000);
  assert(decode("18000", "E").degreesE7 == 1800000000);
  assert(decode("18000.0000000000000000", "W").degreesE7 == -1800000000);
  puts("PASS: standard coordinates, hemispheres, zero, poles, and date line");

  invalid(NULL, "N", NMEA_NUMBER_MISSING);
  invalid("4807.038", NULL, NMEA_NUMBER_MISSING);
  invalid("", NULL, NMEA_NUMBER_MISSING);
  invalid("", "N", NMEA_NUMBER_EMPTY);
  invalid("4807.038", "", NMEA_NUMBER_EMPTY);
  const char *bad[] = {"4807.",
                       "4807.1x",
                       "4807.1.2",
                       "4807e1",
                       "+4807.1",
                       "-4807.1",
                       " 4807.1",
                       "4807.1 ",
                       "480.1",
                       "04807.1",
                       "480a.1",
                       "4807,1",
                       "4807.0000000000000x"};
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    invalid(bad[i], "N", NMEA_NUMBER_BAD_FORMAT);
  invalid("4807.038", "North", NMEA_NUMBER_BAD_FORMAT);
  invalid("4807.038", "n", NMEA_NUMBER_BAD_FORMAT);
  invalid("4807.038", "X", NMEA_NUMBER_BAD_FORMAT);
  invalid("4807.038", "E", NMEA_NUMBER_BAD_FORMAT);
  invalid("01131.000", "N", NMEA_NUMBER_BAD_FORMAT);
  invalid("4860.0", "N", NMEA_NUMBER_OUT_OF_RANGE);
  invalid("9100.0", "S", NMEA_NUMBER_OUT_OF_RANGE);
  invalid("18100.0", "W", NMEA_NUMBER_OUT_OF_RANGE);
  invalid("9000.0000000000001", "N", NMEA_NUMBER_OUT_OF_RANGE);
  invalid("18000.0000000000001", "E", NMEA_NUMBER_OUT_OF_RANGE);
  invalid("65536.0", "E", NMEA_NUMBER_OUT_OF_RANGE);
  invalid("99999.0", "W", NMEA_NUMBER_OUT_OF_RANGE);
  puts("PASS: missing, empty, syntax, overflow, and range failures clear "
       "outputs");

  // The buffers need not be NUL-terminated; bytes outside each span are
  // ignored.
  const char coordinate[] = {'4', '8', '0', '7', '.', '0', '3', '8', 'x'};
  const char hemisphere[] = {'N', 'x'};
  value = Adafruit_GNSS::parseCoordinate({coordinate, 8}, {hemisphere, 1});
  assert(value.status == NMEA_NUMBER_VALID && value.degreesE7 == 481173000);
  assert(decode("1234.123456789987654321", "N").fractionalMinutes == 123456789);
  assert(decode("0000.000005999999999", "N").degreesE7 == 0);
  assert(decode("0000.000006", "N").degreesE7 == 1);
  assert(decode("8959.999999999999", "N").degreesE7 == 899999999);
  puts("PASS: bounded spans, retained precision, and truncation boundaries");

  // These positions differ by less than a millimeter but share an E7 value.
  gnss_coordinate_t first = decode("8959.123456789", "S");
  gnss_coordinate_t second = decode("8959.123456889", "S");
  assert(first.status == NMEA_NUMBER_VALID &&
         second.status == NMEA_NUMBER_VALID);
  assert(first.degreesE7 == second.degreesE7);
  assert(first.fractionalMinutes == 123456789 &&
         second.fractionalMinutes == 123456889);
  assert(first.degrees == 89 && first.minutes == 59 && first.hemisphere == 'S');
  puts("PASS: exact components retain position changes below E7 resolution");

  const uint32_t fractions[] = {0, 1, 599, 600, 123456789, 999999999};
  for (unsigned degrees = 0; degrees < 180; degrees += 7) {
    for (unsigned minutes = 0; minutes < 60; minutes++) {
      for (size_t i = 0; i < sizeof(fractions) / sizeof(fractions[0]); i++) {
        char text[24];
        snprintf(text, sizeof(text), "%03u%02u.%09lu", degrees, minutes,
                 (unsigned long)fractions[i]);
        int32_t expected =
            degrees * 10000000L +
            ((uint64_t)minutes * 1000000000 + fractions[i]) / 6000;
        value = decode(text, "E");
        assert(value.status == NMEA_NUMBER_VALID &&
               value.degreesE7 == expected);
        assert(decode(text, "W").degreesE7 == -expected);
      }
    }
  }
  puts("PASS: fixed-point output matches an independent 64-bit calculation");
  return 0;
}

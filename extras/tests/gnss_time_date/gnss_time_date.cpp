#include "Adafruit_GNSS.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static nmea_span_t field(const char *text) {
  nmea_span_t result = {text, text ? strlen(text) : 0};
  return result;
}

static void badTime(const char *text, nmea_number_status_t status) {
  gnss_time_t value = Adafruit_GNSS::parseTime(field(text));
  assert(value.status == status);
  assert(value.hour == 0 && value.minute == 0 && value.second == 0 &&
         value.millisecond == 0);
}

static void badDate(const char *text, nmea_number_status_t status) {
  gnss_date_t value = Adafruit_GNSS::parseDate(field(text));
  assert(value.status == status);
  assert(value.day == 0 && value.month == 0 && value.year == 0);
}

int main() {
  gnss_time_t time = Adafruit_GNSS::parseTime(field("123456.789123456789"));
  assert(time.status == NMEA_NUMBER_VALID && time.hour == 12 &&
         time.minute == 34 && time.second == 56 && time.millisecond == 789);
  time = Adafruit_GNSS::parseTime(field("000000"));
  assert(time.status == NMEA_NUMBER_VALID && time.hour == 0 &&
         time.minute == 0 && time.second == 0 && time.millisecond == 0);
  time = Adafruit_GNSS::parseTime(field("235960.1"));
  assert(time.status == NMEA_NUMBER_VALID && time.hour == 23 &&
         time.minute == 59 && time.second == 60 && time.millisecond == 100);
  assert(Adafruit_GNSS::parseTime(field("010203.01")).millisecond == 10);
  assert(Adafruit_GNSS::parseTime(field("010203.001")).millisecond == 1);
  assert(Adafruit_GNSS::parseTime(field("010203.0009999")).millisecond == 0);
  assert(Adafruit_GNSS::parseTime(field("010203.9999999")).millisecond == 999);
  puts("PASS: time components, fractional truncation, and leap-second range");

  badTime(NULL, NMEA_NUMBER_MISSING);
  badTime("", NMEA_NUMBER_EMPTY);
  const char *badTimes[] = {"12345",      "1234567",
                            "123456.",    "123456.0x",
                            "+123456",    "123456 ",
                            "123456.1.2", "123456e2",
                            "12a456",     "123456.0000000000000000x"};
  for (size_t i = 0; i < sizeof(badTimes) / sizeof(badTimes[0]); i++)
    badTime(badTimes[i], NMEA_NUMBER_BAD_FORMAT);
  badTime("240000", NMEA_NUMBER_OUT_OF_RANGE);
  badTime("126000", NMEA_NUMBER_OUT_OF_RANGE);
  badTime("125961", NMEA_NUMBER_OUT_OF_RANGE);
  badTime("999999", NMEA_NUMBER_OUT_OF_RANGE);
  puts("PASS: invalid times clear every component");

  gnss_date_t date = Adafruit_GNSS::parseDate(field("150926"));
  assert(date.status == NMEA_NUMBER_VALID && date.day == 15 &&
         date.month == 9 && date.year == 26);
  assert(Adafruit_GNSS::parseDate(field("290224")).status == NMEA_NUMBER_VALID);
  assert(Adafruit_GNSS::parseDate(field("290200")).status == NMEA_NUMBER_VALID);
  assert(Adafruit_GNSS::parseDate(field("311299")).year == 99);
  assert(Adafruit_GNSS::parseDate(field("010180")).year == 80);
  badDate(NULL, NMEA_NUMBER_MISSING);
  badDate("", NMEA_NUMBER_EMPTY);
  badDate("15092", NMEA_NUMBER_BAD_FORMAT);
  badDate("1509260", NMEA_NUMBER_BAD_FORMAT);
  badDate("150926.0", NMEA_NUMBER_BAD_FORMAT);
  badDate("15a926", NMEA_NUMBER_BAD_FORMAT);
  badDate("000926", NMEA_NUMBER_OUT_OF_RANGE);
  badDate("150026", NMEA_NUMBER_OUT_OF_RANGE);
  badDate("151326", NMEA_NUMBER_OUT_OF_RANGE);
  badDate("310426", NMEA_NUMBER_OUT_OF_RANGE);
  badDate("290226", NMEA_NUMBER_OUT_OF_RANGE);
  badDate("300224", NMEA_NUMBER_OUT_OF_RANGE);
  puts("PASS: dates, month lengths, leap years, and two-digit-year "
       "preservation");

  const char boundedTime[] = {'1', '2', '3', '4', '5', '6', '.', '7', 'x'};
  time = Adafruit_GNSS::parseTime({boundedTime, 8});
  assert(time.status == NMEA_NUMBER_VALID && time.millisecond == 700);
  const char boundedDate[] = {'2', '9', '0', '2', '2', '4', 'x'};
  date = Adafruit_GNSS::parseDate({boundedDate, 6});
  assert(date.status == NMEA_NUMBER_VALID && date.day == 29);
  puts("PASS: bounded fields do not require NUL terminators");
  return 0;
}

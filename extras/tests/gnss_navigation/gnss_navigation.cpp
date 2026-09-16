#include "Adafruit_GNSS.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>

static nmea_span_t text(const char *value) {
  nmea_span_t span = {value, value ? strlen(value) : 0};
  return span;
}

static void expect(const char *type, const char *fields,
                   gnss_sentence_status_t status, uint8_t field = 0) {
  gnss_validation_t result =
      Adafruit_GNSS::validateNavigation(text(type), text(fields));
  if (result.status != status || result.field != field) {
    fprintf(stderr, "%s expected %u/%u, got %u/%u: %s\n", type, status, field,
            result.status, result.field, fields ? fields : "NULL");
    assert(false);
  }
}

static std::string replace(const char *fields, uint8_t index,
                           const char *value) {
  std::string result;
  nmea_span_t remaining = text(fields);
  for (uint8_t i = 1; remaining.data; i++) {
    nmea_span_t field = Adafruit_NMEA::nextField(remaining);
    if (i > 1)
      result += ',';
    if (i == index)
      result += value;
    else
      result.append(field.data, field.length);
  }
  return result;
}

int main() {
  const char *gga = "123519.123,4807.038,N,01131.000,E,1,08,0.9,545.4,M,-46.9";
  const char *rmc = "123519,A,4807.038,N,01131.000,E,22.4,84.4,150926";
  const char *gll = "4807.038,N,01131.000,E,123519,A";
  const char *gsa = "A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.5,0.9,1.2";
  expect("GGA", gga, GNSS_SENTENCE_VALID);
  expect("RMC", rmc, GNSS_SENTENCE_VALID);
  expect("GLL", gll, GNSS_SENTENCE_VALID);
  expect("GSA", gsa, GNSS_SENTENCE_VALID);
  expect("RMC", ",V,,,,,,,", GNSS_SENTENCE_VALID);
  expect("GGA", ",,,,,0,00,99.99,,M,", GNSS_SENTENCE_VALID);
  expect("GLL", ",,,,,V", GNSS_SENTENCE_VALID);
  expect("VTG", "anything", GNSS_SENTENCE_UNSUPPORTED);
  expect("GGAX", gga, GNSS_SENTENCE_UNSUPPORTED);
  expect(NULL, NULL, GNSS_SENTENCE_UNSUPPORTED);
  puts("PASS: navigation types, empty no-fix fields, and unsupported types");

  struct Invalid {
    uint8_t index;
    const char *value;
  };
  const Invalid badGga[] = {
      {1, "246000"}, {2, "4860.0"}, {2, "4807x"}, {3, "E"},   {3, ""},
      {4, "01131x"}, {5, "N"},      {6, "256"},   {6, "1.0"}, {7, "99999"},
      {8, "-0.1"},   {8, "0.9x"},   {9, "NaN"},   {10, "F"},  {11, "12e2"}};
  for (size_t i = 0; i < sizeof(badGga) / sizeof(badGga[0]); i++) {
    uint8_t field = badGga[i].index;
    if (field == 3 || field == 5)
      field--;
    expect("GGA", replace(gga, badGga[i].index, badGga[i].value).c_str(),
           GNSS_SENTENCE_INVALID_FIELD, field);
  }
  expect("RMC", replace(rmc, 2, "Active").c_str(), GNSS_SENTENCE_INVALID_FIELD,
         2);
  expect("RMC", replace(rmc, 7, "-1").c_str(), GNSS_SENTENCE_INVALID_FIELD, 7);
  expect("RMC", replace(rmc, 8, "1.2x").c_str(), GNSS_SENTENCE_INVALID_FIELD,
         8);
  expect("RMC", replace(rmc, 9, "310426").c_str(), GNSS_SENTENCE_INVALID_FIELD,
         9);
  expect("GLL", replace(gll, 5, "120000.1x").c_str(),
         GNSS_SENTENCE_INVALID_FIELD, 5);
  expect("GLL", replace(gll, 6, "Q").c_str(), GNSS_SENTENCE_INVALID_FIELD, 6);
  expect("GSA", replace(gsa, 2, "256").c_str(), GNSS_SENTENCE_INVALID_FIELD, 2);
  expect("GSA", replace(gsa, 15, "NaN").c_str(), GNSS_SENTENCE_INVALID_FIELD,
         15);
  expect("GSA", replace(gsa, 16, "-1").c_str(), GNSS_SENTENCE_INVALID_FIELD,
         16);
  expect("GSA", replace(gsa, 17, "1.2x").c_str(), GNSS_SENTENCE_INVALID_FIELD,
         17);
  puts("PASS: field identity, syntax, range, and complete coordinate pairs");

  expect("GGA", NULL, GNSS_SENTENCE_MISSING_FIELDS, 1);
  expect("GGA", "123519,4807.038", GNSS_SENTENCE_MISSING_FIELDS, 3);
  expect("RMC", "123519,A,4807.038,N,01131.000,E,22.4,84.4",
         GNSS_SENTENCE_MISSING_FIELDS, 9);
  expect("GLL", "4807.038,N,01131.000,E,123519", GNSS_SENTENCE_MISSING_FIELDS,
         6);
  // Optional fields beyond the supported decoder's last field remain untouched.
  expect("RMC", (std::string(rmc) + ",,,A,V").c_str(), GNSS_SENTENCE_VALID);
  puts("PASS: missing field positions and supported optional tails");
  return 0;
}

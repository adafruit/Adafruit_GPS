#include <Adafruit_GNSS.h>
#include <Arduino.h>
#include <string.h>

bool testsPassed = false;
bool checkText(const gnss_coordinate_t &coordinate, const char *expected);
bool checkRejected(const gnss_coordinate_t &coordinate);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Adafruit GNSS exact coordinate text regression"));
  const struct {
    const char *field;
    const char *hemisphere;
    const char *expected;
  } cases[] = {
    {"4807.038123456", "N", "48.11730205760"},
    {"4807.038123456", "S", "-48.11730205760"},
    {"01131.000", "E", "11.51666666666"},
    {"17959.123456789", "W", "-179.98539094648"},
    {"8959.123456789", "S", "-89.98539094648"},
    {"8959.123456889", "S", "-89.98539094815"},
    {"8959.999999999", "N", "89.99999999998"},
    {"0000.000000001", "N", "0.00000000001"},
    {"0000.000000002", "N", "0.00000000003"},
    {"0000", "N", "0.00000000000"},
    {"0000", "S", "-0.00000000000"},
    {"00000", "W", "-0.00000000000"},
    {"9000", "S", "-90.00000000000"},
    {"18000", "E", "180.00000000000"},
    {"18000", "W", "-180.00000000000"}
  };
  for (uint8_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    gnss_coordinate_t coordinate = Adafruit_GNSS::parseCoordinate(
        {cases[i].field, strlen(cases[i].field)}, {cases[i].hemisphere, 1});
    if (coordinate.status != NMEA_NUMBER_VALID ||
        !checkText(coordinate, cases[i].expected)) {
      Serial.println(F("FAIL: exact text, truncation, or output buffer bounds"));
      return;
    }
  }
  Serial.println(F("PASS: hemispheres, boundaries, and all output capacities"));
  Serial.println(F("PASS: submillimeter and adjacent fractional-minute values"));

  gnss_coordinate_t valid = Adafruit_GNSS::parseCoordinate({"4807.038", 8}, {"N", 1});
  for (uint8_t i = 0; i < 7; i++) {
    gnss_coordinate_t invalid = valid;
    if (i == 0) {
      invalid.status = NMEA_NUMBER_EMPTY;
    }
    if (i == 1) {
      invalid.hemisphere = 'X';
    }
    if (i == 2) {
      invalid.minutes = 60;
    }
    if (i == 3) {
      invalid.fractionalMinutes = 1000000000UL;
    }
    if (i == 4) {
      invalid.degrees = 91;
    }
    if (i == 5) {
      invalid.degrees = 90; // Nonzero minutes at the pole.
    }
    if (i == 6) {
      invalid.hemisphere = 'W';
      invalid.degrees = 180; // Nonzero minutes at the date line.
    }
    if (!checkRejected(invalid)) {
      Serial.println(F("FAIL: invalid coordinate components were formatted"));
      return;
    }
  }
  gnss_coordinate_t empty = {};
  if (!checkRejected(empty) ||
      Adafruit_GNSS::formatCoordinate(NULL, GNSS_COORDINATE_TEXT_SIZE, valid)) {
    Serial.println(F("FAIL: empty coordinate or NULL output accepted"));
    return;
  }
  // The E7 convenience field must never be the source of precise text.
  valid.degreesE7 = 0;
  if (!checkText(valid, "48.11730000000")) {
    Serial.println(F("FAIL: output depended on the lower-resolution E7 value"));
    return;
  }
  Serial.println(F("PASS: invalid components rejected; E7 is not used"));
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: exact coordinate text regression"));
  } else {
    Serial.println(F("FAIL: exact coordinate text regression"));
  }
}

bool checkText(const gnss_coordinate_t &coordinate, const char *expected) {
  size_t length = strlen(expected);
  for (size_t capacity = 0; capacity <= GNSS_COORDINATE_TEXT_SIZE; capacity++) {
    char storage[GNSS_COORDINATE_TEXT_SIZE + 2];
    memset(storage, '?', sizeof(storage));
    char *output = storage + 1;
    size_t written = Adafruit_GNSS::formatCoordinate(output, capacity, coordinate);
    size_t changed = capacity ? 1 : 0;
    if (capacity > length) {
      if (written != length || strcmp(output, expected)) {
        return false;
      }
      changed = length + 1;
    } else if (written || (capacity && output[0])) {
      return false;
    }
    if (storage[0] != '?') {
      return false;
    }
    for (size_t i = changed + 1; i < sizeof(storage); i++) {
      if (storage[i] != '?') {
        return false;
      }
    }
  }
  return true;
}

bool checkRejected(const gnss_coordinate_t &coordinate) {
  char output[GNSS_COORDINATE_TEXT_SIZE];
  memset(output, '?', sizeof(output));
  if (Adafruit_GNSS::formatCoordinate(output, sizeof(output), coordinate) || output[0]) {
    return false;
  }
  for (size_t i = 1; i < sizeof(output); i++) {
    if (output[i] != '?') {
      return false;
    }
  }
  return true;
}

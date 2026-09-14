// Regression test for bounded copying of unknown NMEA sentence identifiers.
// Runs without a GPS receiver; tests both comma and checksum delimiters.

#include <Adafruit_GPS.h>

bool testsPassed = true;

void setup() {
  Serial.begin(115200);
  // Wait briefly for the Serial Monitor on native USB boards.
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  delay(250);
  Serial.println("Adafruit GPS text field bounds regression");

  const uint8_t lengths[] = {0, 1, NMEA_MAX_SENTENCE_ID - 2,
                            NMEA_MAX_SENTENCE_ID - 1,
                            NMEA_MAX_SENTENCE_ID, NMEA_MAX_SENTENCE_ID + 5};
  for (uint8_t length : lengths) {
    if (!testIdentifier(length, false)) {
      testsPassed = false;
    }
    if (!testIdentifier(length, true)) {
      testsPassed = false;
    }
  }
}

void loop() {
  if (testsPassed) {
    Serial.println("PASS: all 12 text field bounds cases");
  } else {
    Serial.println("FAIL: text field bounds regression");
  }
  delay(2000);
}

bool testIdentifier(uint8_t length, bool comma) {
  Adafruit_GPS gps;
  char sentence[64] = "$GP";
  size_t used = 3;
  for (uint8_t i = 0; i < length; i++) {
    sentence[used++] = 'X';
  }
  if (comma) {
    sentence[used++] = ',';
  }
  uint8_t checksum = 0;
  for (size_t i = 1; i < used; i++) {
    checksum ^= sentence[i];
  }
  snprintf(sentence + used, sizeof(sentence) - used, "*%02X",
           (unsigned int)checksum);

  // Prefill the destination and its neighboring public fields. This catches
  // missing termination as well as writes beyond the destination buffer.
  memset(gps.thisSentence, '?', sizeof(gps.thisSentence));
  memset(gps.lastSource, 'S', sizeof(gps.lastSource));
  memset(gps.lastSentence, 'T', sizeof(gps.lastSentence));

  // The checksum is valid, but the unknown identifier must not be recognized.
  bool passed = !gps.check(sentence);
  if (strcmp(gps.thisSource, "GP") != 0) {
    passed = false;
  }
  size_t copied = min((size_t)length, sizeof(gps.thisSentence) - 1);
  for (size_t i = 0; i < copied; i++) {
    if (gps.thisSentence[i] != 'X') {
      passed = false;
    }
  }
  if (gps.thisSentence[copied] != '\0') {
    passed = false;
  }
  for (size_t i = copied + 1; i < sizeof(gps.thisSentence); i++) {
    if (gps.thisSentence[i] != '?') {
      passed = false;
    }
  }
  for (size_t i = 0; i < sizeof(gps.lastSource); i++) {
    if (gps.lastSource[i] != 'S') {
      passed = false;
    }
  }
  for (size_t i = 0; i < sizeof(gps.lastSentence); i++) {
    if (gps.lastSentence[i] != 'T') {
      passed = false;
    }
  }
  if (passed) {
    Serial.print("PASS: ");
  } else {
    Serial.print("FAIL: ");
  }
  Serial.print("identifier length ");
  Serial.print(length);
  if (comma) {
    Serial.println(", comma delimiter");
  } else {
    Serial.println(", checksum delimiter");
  }
  return passed;
}

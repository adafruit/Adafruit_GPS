#include <Adafruit_GPS.h>

Adafruit_GPS GPS;
bool testsPassed = false;

void setup() {
  Serial.begin(115200);
  // Wait briefly for the Serial Monitor on native USB boards.
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  delay(250);
  Serial.println(F("Adafruit GPS antenna status regression"));

  char valid[][20] = {
    "$PGTOP,11,1*6D",
    "$PGTOP,11,2*6E",
    "$PGTOP,11,3*6F",
    "$PCD,11,1*66",
    "$PCD,11,2*65",
    "$PCD,11,3*64",
  };
  // PGTOP: problem, internal, external. PCD: internal, external, problem.
  const uint8_t expected[] = {1, 2, 3, 2, 3, 1};
  for (uint8_t i = 0; i < 6; i++) {
    GPS.antenna = 0;
    if (!GPS.parse(valid[i]) || GPS.antenna != expected[i]) {
      Serial.print(F("FAIL: antenna status for "));
      Serial.println(valid[i]);
      return;
    }
    if (i < 3) {
      if (strcmp(GPS.lastSource, "PG") || strcmp(GPS.lastSentence, "TOP")) {
        Serial.println(F("FAIL: legacy sentence identity"));
        return;
      }
    } else if (strcmp(GPS.lastSource, "P") || strcmp(GPS.lastSentence, "CD")) {
      Serial.println(F("FAIL: new sentence identity"));
      return;
    }
    Serial.print(F("PASS: "));
    Serial.println(valid[i]);
  }

  char invalid[][22] = {
    "$PCD,11*7B",
    "$PCD,11,*57",
    "$PCD,11,4*63",
    "$PCD,9,2*5C",
    "$PCDXYZ,11,1*3D",
    "$PGTOP,11*70",
    "$PCD,11,10*56",
    "$PGTOP,11,10*5D",
  };
  for (uint8_t i = 0; i < 8; i++) {
    GPS.antenna = 2;
    if (GPS.parse(invalid[i]) || GPS.antenna != 2) {
      Serial.print(F("FAIL: invalid sentence changed antenna status: "));
      Serial.println(invalid[i]);
      return;
    }
  }
  Serial.println(F("PASS: incomplete and unrelated sentences preserve antenna status"));
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: antenna status regression"));
  } else {
    Serial.println(F("FAIL: antenna status regression"));
  }
  delay(2000);
}

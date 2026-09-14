// Two-sentence regression test. Runs without a GPS receiver.

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
  Serial.println("Adafruit GPS text field bounds regression");

  char valid[] =
      "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
  Serial.println(valid);
  if (!GPS.parse(valid)) {
    Serial.println("FAIL: valid GGA sentence did not parse");
    return;
  }
  Serial.println("PASS: valid GGA sentence parsed");

  // Valid checksum, but the unknown identifier exceeds the 19-character
  // destination. Before the fix, its terminator overwrites lastSource[0].
  char invalid[] = "$GPXXXXXXXXXXXXXXXXXXXX*17";
  Serial.println(invalid);
  if (GPS.parse(invalid)) {
    Serial.println("FAIL: unknown sentence was accepted");
    return;
  }
  Serial.println("PASS: unknown sentence rejected");

  // Rejection alone passed before the fix too. The last valid sentence's
  // identity must also survive, proving that the rejected input stayed in bounds.
  if (strcmp(GPS.lastSource, "GP") != 0 ||
      strcmp(GPS.lastSentence, "GGA") != 0) {
    Serial.println("FAIL: rejected sentence corrupted the previous result");
    return;
  }
  Serial.println("PASS: previous result preserved");
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println("PASS: two-sentence regression");
  } else {
    Serial.println("FAIL: two-sentence regression");
  }
  delay(2000);
}

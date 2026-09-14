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
  Serial.println("Adafruit GPS GNSS source regression");

  char glonass[] = "$GLGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*5B";
  if (!GPS.parse(glonass) || strcmp(GPS.lastSource, "GL")) {
    Serial.println("FAIL: GLONASS sentence was not parsed with source GL");
    return;
  }
  Serial.println("PASS: GLONASS sentence was parsed with source GL");

  char galileo[] = "$GAGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*56";
  if (!GPS.parse(galileo) || strcmp(GPS.lastSource, "GA")) {
    Serial.println("FAIL: Galileo sentence was not parsed with source GA");
    return;
  }
  Serial.println("PASS: Galileo sentence was parsed with source GA");
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println("PASS: GNSS source regression");
  } else {
    Serial.println("FAIL: GNSS source regression");
  }
  delay(2000);
}

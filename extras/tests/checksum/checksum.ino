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
  Serial.println("Adafruit GPS checksum regression");

  char good[] = "$GPGGA,120009,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*40";
  if (!GPS.parse(good)) {
    Serial.println("FAIL: valid checksum was rejected");
    return;
  }
  Serial.println("PASS: valid checksum was accepted");

  char bad[] = "$GPGGA,120009,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*4Z";
  if (GPS.parse(bad)) {
    Serial.println("FAIL: invalid checksum character was accepted");
    return;
  }
  Serial.println("PASS: invalid checksum character was rejected");
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println("PASS: checksum regression");
  } else {
    Serial.println("FAIL: checksum regression");
  }
  delay(2000);
}

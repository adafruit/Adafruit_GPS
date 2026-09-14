// Test the private field helper without changing the library's public API.
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#define private public
#include <Adafruit_GPS.h>
#undef private

Adafruit_GPS GPS;
bool testsPassed = false;

void setup() {
  Serial.begin(115200);
  // Wait briefly for the Serial Monitor on native USB boards.
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  delay(250);
  Serial.println("Adafruit GPS NULL field regression");

  char field[] = "123,";
  if (GPS.isEmpty(field)) {
    Serial.println("FAIL: a populated field was reported empty");
    return;
  }
  Serial.println("PASS: populated field is not empty");

  if (!GPS.isEmpty(NULL)) {
    Serial.println("FAIL: a NULL field was not reported empty");
    return;
  }
  Serial.println("PASS: NULL field is empty");
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println("PASS: NULL field regression");
  } else {
    Serial.println("FAIL: NULL field regression");
  }
  delay(2000);
}

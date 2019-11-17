// Test code for Adafruit GPS That Support Using SPI
//
// This code shows how to test a passthru between USB and SPI
//
// Pick one up today at the Adafruit electronics shop
// and help support open source hardware & software! -ada

#include <Adafruit_GPS.h>

// Connect to the GPS on the hardware SPI port
// with CS on pin #10
Adafruit_GPS GPS(&SPI, 10);
#define RESET_PIN 9

void setup() {
  // wait for hardware serial to appear
  while (!Serial);

  // make this baud rate fast enough to we aren't waiting on it
  Serial.begin(115200);

  Serial.println("Adafruit GPS library basic SPI test!");
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(100);
  GPS.begin(100000);  // use 100kHz for SPI data rate
}


void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    GPS.write(c);
  }
  if (GPS.available()) {
    char c = GPS.read();
    Serial.write(c);
  }
}
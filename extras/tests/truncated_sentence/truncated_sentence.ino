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
  Serial.println("Adafruit GPS truncated sentence regression");

  char good[] = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
  if (!GPS.parse(good)) {
    Serial.println("FAIL: valid RMC sentence was rejected");
    return;
  }
  Serial.println("PASS: valid RMC sentence was accepted");
  nmea_float_t latitude = GPS.latitude;
  nmea_float_t longitude = GPS.longitude;

  // This checksum is valid, but the sentence from issue #128 is incomplete.
  char bad[] = "$GPRMC,181536.000,A,5936.79K,D*3A";
  if (GPS.parse(bad)) {
    Serial.println("FAIL: truncated RMC sentence was accepted");
    return;
  }
  Serial.println("PASS: truncated RMC sentence was rejected");
  if (GPS.hour != 12 || GPS.minute != 35 || GPS.seconds != 19 ||
      GPS.day != 23 || GPS.month != 3 || GPS.year != 94 ||
      GPS.latitude != latitude || GPS.longitude != longitude) {
    Serial.println("FAIL: truncated sentence changed the last GPS data");
    return;
  }
  Serial.println("PASS: time, date, and position were preserved");
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println("PASS: truncated sentence regression");
  } else {
    Serial.println("FAIL: truncated sentence regression");
  }
  delay(2000);
}

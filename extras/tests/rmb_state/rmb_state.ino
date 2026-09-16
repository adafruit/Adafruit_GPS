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
  Serial.println("Adafruit GPS RMB state regression");

  char good[] = "$GPRMB,A,0.10,R,DEST,START,4807.038,N,01131.000,E,1.0,84.4,2.5,A*44";
#ifndef NMEA_EXTENSIONS
  if (GPS.parse(good)) {
    Serial.println("FAIL: basic GPS build accepted an unsupported RMB sentence");
    return;
  }
  Serial.println("PASS: basic GPS build rejects unsupported RMB sentences");
#else
  if (!GPS.parse(good)) {
    Serial.println("FAIL: valid RMB sentence was rejected");
    return;
  }
  Serial.println("PASS: valid RMB sentence was accepted");
  nmea_float_t crossTrackError = GPS.get(NMEA_XTE);
  nmea_float_t latitude = GPS.get(NMEA_LATWP);
  nmea_float_t longitude = GPS.get(NMEA_LONWP);

  // The checksum is valid, but Q is not a longitude direction.
  char bad[] = "$GPRMB,A,0.20,L,OTHER,NEW,4907.038,N,01231.000,Q,2.0,90.0,3.0,A*17";
  if (GPS.parse(bad)) {
    Serial.println("FAIL: invalid RMB sentence was accepted");
    return;
  }
  Serial.println("PASS: invalid RMB sentence was rejected");
  if (GPS.get(NMEA_XTE) != crossTrackError ||
      GPS.get(NMEA_LATWP) != latitude || GPS.get(NMEA_LONWP) != longitude ||
      strcmp(GPS.toID, "DEST") || strcmp(GPS.fromID, "START")) {
    Serial.println("FAIL: rejected sentence changed the waypoint data");
    return;
  }
  Serial.println("PASS: previous waypoint data was preserved");
#endif
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println("PASS: RMB state regression");
  } else {
    Serial.println("FAIL: RMB state regression");
  }
  delay(2000);
}

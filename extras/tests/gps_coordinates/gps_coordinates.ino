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
  Serial.println(F("Adafruit GPS shared coordinate regression"));
  char sentence[120] =
      "$GPRMC,123519,A,4807.038123456,N,01131.123456789,E,1.2,3.4,150926,,,A";
  GPS.addChecksum(sentence);
  if (!GPS.parse(sentence) || GPS.latitude_fixed != 481173020 ||
      GPS.longitude_fixed != 115187242 || GPS.lat != 'N' || GPS.lon != 'E') {
    Serial.println(F("FAIL: exact coordinates through the RMC decoder"));
    return;
  }
  double tolerance = sizeof(nmea_float_t) > 4 ? 1e-11 : 1e-5;
  if (abs(GPS.latitudeDegrees - (48.0 + 7.038123456 / 60)) > tolerance ||
      abs(GPS.longitudeDegrees - (11.0 + 31.123456789 / 60)) > tolerance) {
    Serial.println(F("FAIL: decimal degrees lost precision through E7"));
    return;
  }
  Serial.println(F("PASS: exact fixed point and full convenience-float precision"));

  const char *bad[] = {"4860.0,N", "4807.1x,N", "4807.0,North", "9000.00001,N",
                       "18000.1,E", "4807.0,E", "-4807.0,S", "4807.0,"};
  for (uint8_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    char text[32];
    strcpy(text, bad[i]);
    nmea_float_t degrees = 12, angle = 34;
    int32_t fixed = 56;
    char direction = 'X';
    if (GPS.parseCoord(text, &degrees, &angle, &fixed, &direction) ||
        degrees != 12 || angle != 34 || fixed != 56 || direction != 'X') {
      Serial.println(F("FAIL: bad coordinate changed outputs"));
      return;
    }
  }
  if (GPS.parseCoord(NULL)) {
    Serial.println(F("FAIL: NULL coordinate accepted"));
    return;
  }
  Serial.println(F("PASS: invalid fields leave every requested output intact"));

  char west[] = "18000,W";
  int32_t fixed = 0;
  if (!GPS.parseCoord(west, NULL, NULL, &fixed) || fixed != -1800000000) {
    Serial.println(F("FAIL: integral minutes or optional outputs"));
    return;
  }
  char south[] = "8959.999999999999,S";
  if (!GPS.parseCoord(south, NULL, NULL, &fixed) || fixed != -899999999) {
    Serial.println(F("FAIL: high precision near a pole"));
    return;
  }
  Serial.println(F("PASS: hemispheres, boundary values, and optional outputs"));

  if (sizeof(nmea_float_t) > 4) {
    char first[] =
        "$GPRMC,123519,A,8959.123456789,S,17959.123456789,W,1.2,3.4,150926,,,A";
    char second[] =
        "$GPRMC,123519,A,8959.123456889,S,17959.123456889,W,1.2,3.4,150926,,,A";
    // Leave room for addChecksum() to append *HH and the terminator.
    char precise[120];
    strcpy(precise, first);
    GPS.addChecksum(precise);
    if (!GPS.parse(precise)) {
      Serial.println(F("FAIL: first high-precision position"));
      return;
    }
    nmea_float_t latitude = GPS.latitudeDegrees;
    nmea_float_t longitude = GPS.longitudeDegrees;
    int32_t latitudeE7 = GPS.latitude_fixed, longitudeE7 = GPS.longitude_fixed;
    strcpy(precise, second);
    GPS.addChecksum(precise);
    // A 0.0000001-minute change is about 0.185 mm in latitude. Both
    // positions occupy the same E7 cell, so E7 cannot reconstruct the change.
    double change = -0.0000001 / 60;
    if (!GPS.parse(precise) || GPS.latitude_fixed != latitudeE7 ||
        GPS.longitude_fixed != longitudeE7 ||
        abs((GPS.latitudeDegrees - latitude) - change) > 1e-12 ||
        abs((GPS.longitudeDegrees - longitude) - change) > 1e-12) {
      Serial.println(F("FAIL: submillimeter detail lost in double coordinates"));
      return;
    }
    Serial.println(F("PASS: double coordinates retain changes smaller than E7"));
  }
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: GPS shared coordinate regression"));
  } else {
    Serial.println(F("FAIL: GPS shared coordinate regression"));
  }
}

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#define private public
#include <Adafruit_GPS.h>
#undef private

Adafruit_GPS GPS;
bool testsPassed = false;

struct NavigationState {
  uint8_t hour, minute, second, day, month, year, quality, quality3d, satellites;
  uint16_t milliseconds;
  int32_t latitude, longitude;
  nmea_float_t altitude, geoid, speed, angle, hdop, pdop, vdop;
  uint32_t updateTime, fixTime, timeTime, dateTime;
  bool fix;
  char source[NMEA_MAX_SOURCE_ID], sentence[NMEA_MAX_SENTENCE_ID];
};

bool parseBody(const char *body);
NavigationState saveState();
bool sameState(const NavigationState &before);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Adafruit GPS coherent navigation regression"));
  GPS.resetSentTime();
  if (!parseBody("$GPRMC,123519.123,A,4807.038,N,01131.000,E,22.4,84.4,150926,,,A") ||
      !parseBody("$GPGGA,123519.123,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9") ||
      !parseBody("$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.5,0.9,1.2")) {
    Serial.println(F("FAIL: valid navigation seed"));
    return;
  }
  NavigationState before = saveState();
  const char *bad[] = {
      "$GPRMC,101112,Q,5000.0,N,02000.0,E,1,2,160926",
      "$GPRMC,101112,A,5060.0,N,02000.0,E,1,2,160926",
      "$GPRMC,101112,A,5000.0,N,02000.0,E,1x,2,160926",
      "$GPRMC,101112,A,5000.0,N,02000.0,E,1,2,310426",
      "$GPGGA,101112,5000.0,N,02000.0,E,256,09,1,100,M,40",
      "$GPGGA,101112,5000.0,N,02000.0,E,2,09,-1,100,M,40",
      "$GPGGA,101112,5000.0,N,02000.0,E,2,09,1,100,M,40x",
      "$GPGGA,101112x,5000.0,N,02000.0,E,2,09,1,100,M,40",
      "$GPGLL,5000.0,N,02000.0,E,101112,Q",
      "$GPGLL,05000.0,E,02000.0,E,101112,A",
      "$GPGSA,A,2,01,02,03,04,05,06,07,08,09,10,11,12,2.5,1.9,2x"};
  for (uint8_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    delay(2);
    GPS.resetSentTime();
    if (parseBody(bad[i]) || !sameState(before)) {
      Serial.print(F("FAIL: rejected sentence changed navigation, case "));
      Serial.println(i);
      return;
    }
  }
  Serial.println(F("PASS: bad early and late fields preserve navigation and timestamps"));

  if (!parseBody("$GPRMC,235960.123999,A,5000.0,S,02000.0,W,1,2,290224") ||
      GPS.hour != 23 || GPS.minute != 59 || GPS.seconds != 60 ||
      GPS.milliseconds != 123 || GPS.day != 29 || GPS.month != 2 || GPS.year != 24) {
    Serial.println(F("FAIL: leap time, millisecond truncation, or date decoding"));
    return;
  }
  Serial.println(F("PASS: shared UTC and date decoding through the RMC parser"));

  int32_t latitude = GPS.latitude_fixed, longitude = GPS.longitude_fixed;
  uint32_t timeTime = GPS.lastTime;
  if (!parseBody("$GPGLL,,,,,,V") || GPS.fix ||
      GPS.latitude_fixed != latitude || GPS.longitude_fixed != longitude ||
      GPS.lastTime != timeTime) {
    Serial.println(F("FAIL: empty no-fix fields replaced stored values or timestamps"));
    return;
  }
  Serial.println(F("PASS: valid no-fix messages preserve absent measurements"));

  uint32_t dateTime = GPS.lastDate;
  GPS.sentTime = 1000;
  if (!parseBody("$GPGGA,123519,4807.038,N,01131.000,E,4,08,0.9,545.4,M,46.9") ||
      !GPS.fix || GPS.fixquality != 4 || GPS.lastFix != 1000 ||
      GPS.lastTime != 1000 || GPS.lastDate != dateTime) {
    Serial.println(F("FAIL: GGA position update or independent date timestamp"));
    return;
  }
  latitude = GPS.latitude_fixed;
  longitude = GPS.longitude_fixed;
  GPS.sentTime = 2000;
  if (!parseBody("$GPRMC,,V,,,,,,,") || GPS.fix || GPS.fixquality != 4 ||
      GPS.latitude_fixed != latitude || GPS.longitude_fixed != longitude ||
      GPS.lastFix != 1000 || GPS.lastTime != 1000 || GPS.lastDate != dateTime) {
    Serial.println(F("FAIL: RMC no-fix changed GGA quality or absent data"));
    return;
  }
  GPS.sentTime = 3000;
  if (!parseBody("$GPGLL,,,,,123520,A") || !GPS.fix || GPS.fixquality != 4 ||
      GPS.seconds != 20 || GPS.lastFix != 3000 || GPS.lastTime != 3000 ||
      GPS.lastDate != dateTime || GPS.latitude_fixed != latitude ||
      GPS.longitude_fixed != longitude) {
    Serial.println(F("FAIL: GLL fix/time update replaced absent position or date"));
    return;
  }
  GPS.sentTime = 4000;
  if (!parseBody("$GPGGA,,,,,,,,,,,") || !GPS.fix || GPS.fixquality != 4 ||
      GPS.lastFix != 3000 || GPS.lastTime != 3000 || GPS.lastDate != dateTime) {
    Serial.println(F("FAIL: empty GGA changed stored fix or timestamps"));
    return;
  }
  if (!parseBody("$GPGGA,,,,,,0,00,99.99,,M,") || GPS.fix || GPS.fixquality ||
      GPS.lastFix != 3000 || GPS.lastTime != 3000) {
    Serial.println(F("FAIL: GGA no-fix refreshed last fix or time"));
    return;
  }
  Serial.println(F("PASS: shared position results preserve legacy merging and timestamps"));
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: coherent navigation regression"));
  } else {
    Serial.println(F("FAIL: coherent navigation regression"));
  }
}

bool parseBody(const char *body) {
  char sentence[150];
  strcpy(sentence, body);
  GPS.addChecksum(sentence);
  return GPS.parse(sentence);
}

NavigationState saveState() {
  NavigationState state = {
      GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year,
      GPS.fixquality, GPS.fixquality_3d, GPS.satellites, GPS.milliseconds,
      GPS.latitude_fixed, GPS.longitude_fixed, GPS.altitude, GPS.geoidheight,
      GPS.speed, GPS.angle, GPS.HDOP, GPS.PDOP, GPS.VDOP,
      GPS.lastUpdate, GPS.lastFix, GPS.lastTime, GPS.lastDate, GPS.fix, {}, {}};
  strcpy(state.source, GPS.lastSource);
  strcpy(state.sentence, GPS.lastSentence);
  return state;
}

bool sameState(const NavigationState &before) {
  NavigationState after = saveState();
  return before.hour == after.hour && before.minute == after.minute &&
      before.second == after.second && before.milliseconds == after.milliseconds &&
      before.day == after.day && before.month == after.month && before.year == after.year &&
      before.quality == after.quality && before.quality3d == after.quality3d &&
      before.satellites == after.satellites && before.latitude == after.latitude &&
      before.longitude == after.longitude && before.altitude == after.altitude &&
      before.geoid == after.geoid && before.speed == after.speed && before.angle == after.angle &&
      before.hdop == after.hdop && before.pdop == after.pdop && before.vdop == after.vdop &&
      before.updateTime == after.updateTime && before.fixTime == after.fixTime &&
      before.timeTime == after.timeTime && before.dateTime == after.dateTime &&
      before.fix == after.fix && !strcmp(before.source, after.source) &&
      !strcmp(before.sentence, after.sentence);
}

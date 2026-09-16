#include <Adafruit_GPS.h>

class TestStream : public Stream {
public:
  const char *input = "";
  int available() { return *input != 0; }
  int read() { return *input ? *input++ : -1; }
  int peek() { return *input ? *input : -1; }
  void flush() {}
  size_t write(uint8_t) { return 1; }
};

TestStream firstStream, secondStream;
Adafruit_GPS firstGPS((Stream *)&firstStream), secondGPS((Stream *)&secondStream);
bool testsPassed = false;
bool receiveBody(TestStream &stream, Adafruit_GPS &gps, const char *body);
bool noPosition(const gnss_position_t &position, gnss_sentence_status_t status);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Adafruit GPS exact received position regression"));
  const Adafruit_GPS &view = firstGPS;
  if (!noPosition(view.lastPosition(), GNSS_SENTENCE_INVALID_FRAME) ||
      firstGPS.newNMEAreceived()) {
    Serial.println(F("FAIL: getter before reception"));
    return;
  }
  firstGPS.fix = false;
  firstGPS.latitude_fixed = 42;
  firstGPS.year = 99;
  if (!receiveBody(firstStream, firstGPS,
      "GNRMC,123519.123,A,8959.123456789,S,17959.123456789,W,1,2,290224")) {
    Serial.println(F("FAIL: receive first exact position"));
    return;
  }
  gnss_position_t saved = view.lastPosition();
  char coordinate[GNSS_COORDINATE_TEXT_SIZE];
  if (saved.validation.status != GNSS_SENTENCE_VALID || !saved.fix ||
      saved.latitude.fractionalMinutes != 123456789 || saved.date.day != 29 ||
      saved.time.millisecond != 123 || !firstGPS.newNMEAreceived() ||
      firstGPS.fix || firstGPS.latitude_fixed != 42 || firstGPS.year != 99 ||
      !Adafruit_GNSS::formatCoordinate(coordinate, sizeof(coordinate), saved.longitude) ||
      strcmp(coordinate, "-179.98539094648")) {
    Serial.println(F("FAIL: exact result, acknowledgement flag, or legacy state changed"));
    return;
  }
  if (!firstGPS.parse(firstGPS.lastNMEA()) || firstGPS.newNMEAreceived() ||
      !firstGPS.fix || firstGPS.latitude_fixed != saved.latitude.degreesE7 ||
      view.lastPosition().longitude.fractionalMinutes != 123456789) {
    Serial.println(F("FAIL: legacy parse and exact getter coexistence"));
    return;
  }
  Serial.println(F("PASS: exact output on float builds without changing receive or legacy state"));

  firstStream.input = "$unfinished";
  while (firstStream.available()) firstGPS.read();
  if (firstGPS.newNMEAreceived() ||
      view.lastPosition().latitude.fractionalMinutes != 123456789 ||
      !receiveBody(firstStream, firstGPS,
          "GNRMC,123520,A,8959.123456889,S,17959.123456889,W,1,2,290224")) {
    Serial.println(F("FAIL: partial sentence or recovery"));
    return;
  }
  gnss_position_t next = view.lastPosition();
  if (next.latitude.degreesE7 != saved.latitude.degreesE7 ||
      next.latitude.fractionalMinutes != 123456889 ||
      saved.latitude.fractionalMinutes != 123456789 ||
      !Adafruit_GNSS::formatCoordinate(coordinate, sizeof(coordinate), next.longitude) ||
      strcmp(coordinate, "-179.98539094815")) {
    Serial.println(F("FAIL: submillimeter change or owned result"));
    return;
  }
  Serial.println(F("PASS: new sentences retain changes smaller than E7 and preserve copied results"));

  if (!receiveBody(secondStream, secondGPS,
          "GBGGA,010203,4807.038,N,01131.000,E,4,08,0.9,545.4,M,46.9") ||
      secondGPS.lastPosition().fixQuality != 4 ||
      secondGPS.lastPosition().latitude.degreesE7 != 481173000 ||
      view.lastPosition().latitude.fractionalMinutes != 123456889) {
    Serial.println(F("FAIL: independent receivers or shared talker routing"));
    return;
  }
  char separate[120] = "$GPGLL,4807.038,N,01131.000,E,123519,A";
  firstGPS.addChecksum(separate);
  if (!firstGPS.parse(separate) ||
      view.lastPosition().latitude.fractionalMinutes != 123456889 ||
      !firstGPS.newNMEAreceived()) {
    Serial.println(F("FAIL: parsing a supplied string changed the received snapshot"));
    return;
  }
  Serial.println(F("PASS: supplied-buffer parsing and separate receivers remain independent"));

  firstGPS.lastNMEA();
  if (!receiveBody(firstStream, firstGPS, "GNRMC,,V,,,,,,,") ||
      view.lastPosition().validation.status != GNSS_SENTENCE_VALID ||
      view.lastPosition().latitude.status != NMEA_NUMBER_EMPTY ||
      view.lastPosition().fix || !firstGPS.fix) {
    Serial.println(F("FAIL: empty/no-fix line reused a cached position"));
    return;
  }
  firstGPS.lastNMEA();
  if (!receiveBody(firstStream, firstGPS, "PAIR001,062,0") ||
      !noPosition(view.lastPosition(), GNSS_SENTENCE_UNSUPPORTED) ||
      !firstGPS.newNMEAreceived() || strncmp(firstGPS.lastNMEA(), "$PAIR001,", 9)) {
    Serial.println(F("FAIL: command reply replaced or acknowledged by getter"));
    return;
  }
  firstStream.input = "$GPGLL,4807.038,N,01131.000,E,123519,A*00\n";
  while (firstStream.available()) firstGPS.read();
  if (!firstGPS.newNMEAreceived() ||
      !noPosition(view.lastPosition(), GNSS_SENTENCE_INVALID_FRAME)) {
    Serial.println(F("FAIL: bad checksum accepted or acknowledged"));
    return;
  }
  firstGPS.common_init();
  if (!noPosition(view.lastPosition(), GNSS_SENTENCE_INVALID_FRAME) ||
      firstGPS.newNMEAreceived() || secondGPS.lastPosition().fixQuality != 4 ||
      saved.latitude.fractionalMinutes != 123456789) {
    Serial.println(F("FAIL: reset, receiver isolation, or result lifetime"));
    return;
  }
  Serial.println(F("PASS: no-fix, replies, bad checksum, and reset never reuse stale positions"));
  testsPassed = true;
}

void loop() {
  if (testsPassed) Serial.println(F("PASS: exact received position regression"));
  else Serial.println(F("FAIL: exact received position regression"));
}

bool receiveBody(TestStream &stream, Adafruit_GPS &gps, const char *body) {
  char line[120];
  if (!Adafruit_NMEA::buildCommand(line, sizeof(line), body, strlen(body))) return false;
  stream.input = line;
  while (stream.available()) gps.read();
  stream.input = "";
  return gps.newNMEAreceived();
}

bool noPosition(const gnss_position_t &position, gnss_sentence_status_t status) {
  return position.validation.status == status &&
      position.latitude.status == NMEA_NUMBER_MISSING &&
      position.longitude.status == NMEA_NUMBER_MISSING &&
      position.time.status == NMEA_NUMBER_MISSING &&
      position.date.status == NMEA_NUMBER_MISSING &&
      position.fixStatus == NMEA_NUMBER_MISSING &&
      position.fixQualityStatus == NMEA_NUMBER_MISSING && !position.fix;
}

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
// common_init() detaches the transport; reconnect the test stream afterwards.
#define private public
#include <Adafruit_GPS.h>
#undef private

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
Adafruit_GPS firstGPS((Stream *)&firstStream);
Adafruit_GPS secondGPS((Stream *)&secondStream);
bool testsPassed = false;
const char navigation[] =
    "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n";

bool receive(TestStream &stream, Adafruit_GPS &gps, const char *text);
bool checkReinitialization();

void setup() {
  Serial.begin(115200);
  Serial.println(F("Adafruit GPS receive framing regression"));
  if (firstGPS.newNMEAreceived() || firstGPS.lastNMEA()[0] ||
      firstGPS.available() || firstGPS.read()) {
    Serial.println(F("FAIL: initial receive state"));
    return;
  }
  if (receive(firstStream, firstGPS, "noise\r\n") ||
      firstGPS.newNMEAreceived()) {
    Serial.println(F("FAIL: noise was published as a sentence"));
    return;
  }
  if (!receive(firstStream, firstGPS, navigation) ||
      !firstGPS.newNMEAreceived() ||
      strcmp(firstGPS.lastNMEA(), navigation) || firstGPS.newNMEAreceived() ||
      !firstGPS.parse(firstGPS.lastNMEA())) {
    Serial.println(F("FAIL: reception, acknowledgement, or parsing"));
    return;
  }
  Serial.println(F("PASS: initial state, noise, byte reads, and normal parsing"));

  if (receive(firstStream, firstGPS, "$interrupted,") ||
      strcmp(firstGPS.lastNMEA(), navigation) ||
      !receive(firstStream, firstGPS, navigation) ||
      strcmp(firstGPS.lastNMEA(), navigation)) {
    Serial.println(F("FAIL: interrupted line recovery"));
    return;
  }
  if (!receive(firstStream, firstGPS, "$A*00\n") ||
      strcmp(firstGPS.lastNMEA(), "$A*00\n") ||
      !receive(firstStream, firstGPS, "!A,1*5C\n") ||
      strcmp(firstGPS.lastNMEA(), "!A,1*5C\n")) {
    Serial.println(F("FAIL: invalid raw lines or exclamation start marker"));
    return;
  }
  firstStream.input = "$PQTMVERNO*58\n";
  if (!firstGPS.waitForSentence("$PQTMVERNO", 1, false, 100) ||
      strcmp(firstGPS.lastNMEA(), "$PQTMVERNO*58\n")) {
    Serial.println(F("FAIL: unknown zero-field command response"));
    return;
  }
  Serial.println(F("PASS: restart markers, raw invalid lines, and command waits"));

  char body[MAXLINELENGTH - NMEA_COMMAND_OVERHEAD + 1];
  memset(body, 'x', sizeof(body) - 1);
  body[0] = 'A';
  body[1] = ',';
  body[sizeof(body) - 1] = 0;
  char maximum[MAXLINELENGTH];
  if (Adafruit_NMEA::buildCommand(maximum, sizeof(maximum), body,
                                   strlen(body)) != MAXLINELENGTH - 1 ||
      !receive(firstStream, firstGPS, maximum) ||
      strcmp(firstGPS.lastNMEA(), maximum)) {
    Serial.println(F("FAIL: maximum-size line"));
    return;
  }
  // Keep the previous complete line while a longer sentence is discarded.
  char oversized[MAXLINELENGTH + 4];
  memset(oversized, 'x', sizeof(oversized));
  oversized[0] = '$';
  oversized[sizeof(oversized) - 2] = '\n';
  oversized[sizeof(oversized) - 1] = 0;
  if (receive(firstStream, firstGPS, oversized) ||
      firstGPS.newNMEAreceived() || strcmp(firstGPS.lastNMEA(), maximum) ||
      !receive(firstStream, firstGPS, navigation) ||
      strcmp(firstGPS.lastNMEA(), navigation)) {
    Serial.println(F("FAIL: overflow preservation or recovery"));
    return;
  }
  Serial.println(F("PASS: exact capacity, overflow discard, and recovery"));

  firstStream.input = navigation;
  firstGPS.read();
  const char *pausedAt = firstStream.input;
  firstGPS.pause(true);
  if (firstGPS.available() || firstGPS.read() ||
      firstStream.input != pausedAt || firstGPS.newNMEAreceived()) {
    Serial.println(F("FAIL: pause consumed input"));
    return;
  }
  firstGPS.pause(false);
  while (firstStream.available()) {
    firstGPS.read();
  }
  if (!firstGPS.newNMEAreceived() || strcmp(firstGPS.lastNMEA(), navigation)) {
    Serial.println(F("FAIL: resume lost partial sentence"));
    return;
  }
  Serial.println(F("PASS: pause and resume preserve partial input"));

  firstStream.input = navigation;
  secondStream.input = navigation;
  uint32_t firstStarted = millis();
  firstGPS.read();
  delay(60);
  uint32_t secondStarted = millis();
  secondGPS.read();
  delay(60);
  while (firstStream.available()) {
    firstGPS.read();
  }
  while (secondStream.available()) {
    secondGPS.read();
  }
  if (!firstGPS.parse(firstGPS.lastNMEA()) ||
      !secondGPS.parse(secondGPS.lastNMEA()) ||
      abs(firstGPS.secondsSinceTime() - (millis() - firstStarted) / 1000.0) >
          0.02 ||
      abs(secondGPS.secondsSinceTime() - (millis() - secondStarted) / 1000.0) >
          0.02) {
    Serial.println(F("FAIL: receivers shared a sentence timestamp"));
    return;
  }
  Serial.println(F("PASS: two receivers keep independent sentence timestamps"));
  if (!checkReinitialization()) {
    Serial.println(F("FAIL: reinitialization retained input or mixed buffer roles"));
    return;
  }
  Serial.println(F("PASS: reinitialization clears input and preserves buffer roles"));
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: GPS receive framing regression"));
  } else {
    Serial.println(F("FAIL: GPS receive framing regression"));
  }
}

bool receive(TestStream &stream, Adafruit_GPS &gps, const char *text) {
  stream.input = text;
  bool completed = false;
  while (*stream.input) {
    char expected = *stream.input;
    if (gps.read() != expected) {
      Serial.println(F("FAIL: read did not return the incoming byte"));
      testsPassed = false;
      return false;
    }
    completed = completed || gps.newNMEAreceived();
  }
  return completed;
}

bool checkReinitialization() {
  // Cover the initial buffer orientation and both roles after completed lines.
  for (uint8_t completed = 0; completed < 3; completed++) {
    TestStream stream;
    Adafruit_GPS gps((Stream *)&stream);
    for (uint8_t i = 0; i < completed; i++) {
      if (!receive(stream, gps, navigation)) {
        return false;
      }
    }
    // Leave any completed line unacknowledged while starting another line.
    receive(stream, gps, "$stale,");
    gps.common_init();
    if (gps.newNMEAreceived() || gps.lastNMEA()[0] ||
        gps.receiver.lastText().data || gps.receiver.sentenceStartedAt() ||
        gps.receiver.sentenceReceivedAt()) {
      return false;
    }
    gps.gpsStream = &stream;
    if (receive(stream, gps, "tail\n") || gps.lastNMEA()[0] ||
        receive(stream, gps, "$fresh,") || gps.lastNMEA()[0] ||
        !receive(stream, gps, navigation) || strcmp(gps.lastNMEA(), navigation)) {
      return false;
    }
    if (receive(stream, gps, "$next,") || strcmp(gps.lastNMEA(), navigation)) {
      return false;
    }
    gps.common_init();
    if (gps.newNMEAreceived() || gps.lastNMEA()[0]) {
      return false;
    }
  }
  return true;
}

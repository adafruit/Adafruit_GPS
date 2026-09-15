#include <Adafruit_GPS.h>

// Supply test sentences without changing the configuration of a real GPS.
class TestStream : public Stream {
public:
  const char *input = "";

  int available() { return *input != 0; }
  int read() {
    if (*input) {
      return *input++;
    }
    return -1;
  }
  int peek() { return *input ? *input : -1; }
  void flush() {}
  size_t write(uint8_t) { return 1; }
};

TestStream stream;
Adafruit_GPS GPS((Stream *)&stream);
bool testsPassed = false;

void setup() {
  Serial.begin(115200);
  // Wait briefly for the Serial Monitor on native USB boards.
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  delay(250);
  Serial.println(F("Adafruit GPS response timeout regression"));

  uint32_t start = millis();
  if (GPS.waitForSentence(PMTK_AWAKE, 10, false, 100) ||
      millis() - start < 100 || millis() - start > 500) {
    Serial.println(F("FAIL: silent stream timeout"));
    return;
  }
  Serial.println(F("PASS: silent stream times out"));

  start = millis();
  if (GPS.waitForSentence(PMTK_AWAKE, 10, true, 100) ||
      millis() - start < 100 || millis() - start > 500) {
    Serial.println(F("FAIL: interrupt-driven timeout"));
    return;
  }
  Serial.println(F("PASS: interrupt-driven wait times out"));

  stream.input = "$PMTK010";
  if (GPS.waitForSentence(PMTK_AWAKE, 10, false, 100)) {
    Serial.println(F("FAIL: incomplete sentence was accepted"));
    return;
  }
  Serial.println(F("PASS: incomplete sentence times out"));
  // Finish and discard the partial line before testing complete sentences.
  stream.input = "\r\n";
  while (stream.available()) {
    GPS.read();
  }
  GPS.lastNMEA();

  stream.input = "$OTHER\r\n" PMTK_AWAKE "\r\n";
  if (GPS.waitForSentence(PMTK_AWAKE, 1, false, 100)) {
    Serial.println(F("FAIL: sentence limit was ignored"));
    return;
  }
  if (!GPS.waitForSentence(PMTK_AWAKE, 1, false, 100)) {
    Serial.println(F("FAIL: matching response was not accepted"));
    return;
  }
  Serial.println(F("PASS: sentence limit and matching response"));

  stream.input = PMTK_AWAKE "\r\n";
  if (GPS.waitForSentence(PMTK_AWAKE, 0, false, 100) ||
      GPS.waitForSentence(PMTK_AWAKE, 1, false, 0) ||
      !GPS.waitForSentence(PMTK_AWAKE, 1, false, 100)) {
    Serial.println(F("FAIL: zero limits consumed a response"));
    return;
  }
  Serial.println(F("PASS: zero limits return without consuming data"));

  GPS.standby();
  start = millis();
  Serial.println(F("Testing failed wakeup (10 seconds)..."));
  if (GPS.wakeup() || millis() - start < 10000 ||
      millis() - start > 10500) {
    Serial.println(F("FAIL: wakeup did not time out"));
    return;
  }
  Serial.println(F("PASS: failed wakeup returns after 10 seconds"));
  stream.input = PMTK_AWAKE "\r\n";
  if (!GPS.wakeup() || GPS.wakeup()) {
    Serial.println(F("FAIL: wakeup retry or standby state"));
    return;
  }
  Serial.println(F("PASS: failed wakeup can be retried successfully"));
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: response timeout regression"));
  } else {
    Serial.println(F("FAIL: response timeout regression"));
  }
  delay(2000);
}

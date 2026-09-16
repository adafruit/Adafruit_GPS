#include <Adafruit_GPS.h>

// Supply test sentences without changing the configuration of a real GPS.
class TestStream : public Stream {
public:
  const char *input = "";
  const char *end = NULL; // Optional explicit end for embedded-NUL input.

  int available() {
    if (end) {
      return input < end;
    }
    return *input != 0;
  }
  int read() {
    if (available()) {
      return *input++;
    }
    return -1;
  }
  int peek() { return available() ? *input : -1; }
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

  // A matching prefix alone must not turn a corrupt reply into success.
  const char *invalid[] = {
      "$PMTK010,002*00\r\n", // Wrong checksum.
      "$PMTK010,002*2Z\r\n", // Non-hex checksum.
      "$PMTK010,002\r\n",    // Missing checksum.
      PMTK_AWAKE "junk\r\n", // Extra trailer after a valid checksum.
  };
  for (uint8_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
    stream.input = invalid[i];
    if (GPS.waitForSentence("$PMTK010", 1, false, 100) ||
        strcmp(GPS.lastNMEA(), invalid[i])) {
      Serial.println(F("FAIL: corrupt reply accepted or raw response lost"));
      return;
    }
  }
  char embeddedNul[] = PMTK_AWAKE "\0junk\r\n";
  stream.input = embeddedNul;
  stream.end = embeddedNul + sizeof(embeddedNul) - 1;
  bool accepted = GPS.waitForSentence(PMTK_AWAKE, 1, false, 100);
  stream.end = NULL;
  if (accepted) {
    Serial.println(F("FAIL: embedded NUL hid corrupt bytes after a valid prefix"));
    return;
  }
  Serial.println(F("PASS: matching corrupt replies and embedded NUL rejected"));

  stream.input = PMTK_AWAKE "junk\r\n" PMTK_AWAKE "\r\n";
  if (GPS.waitForSentence(PMTK_AWAKE, 1, false, 100) ||
      !GPS.waitForSentence(PMTK_AWAKE, 1, false, 100)) {
    Serial.println(F("FAIL: invalid reply did not consume exactly one sentence"));
    return;
  }
  stream.input = "$PMTK010,002*00\r\n" PMTK_AWAKE "\r\n";
  if (!GPS.waitForSentence("$PMTK010", 2, false, 100) ||
      strcmp(GPS.lastNMEA(), PMTK_AWAKE "\r\n")) {
    Serial.println(F("FAIL: valid reply after a corrupt match was lost"));
    return;
  }
  Serial.println(F("PASS: invalid replies count toward the limit and waits can continue"));

  char replies[160];
  const char *navigation = "GNGLL,4807.038,N,01131.000,E,123519,A";
  size_t length = Adafruit_NMEA::buildCommand(replies, sizeof(replies),
                                              navigation, strlen(navigation));
  const char *reply = "PAIR001,062,0";
  if (!length || !Adafruit_NMEA::buildCommand(replies + length,
      sizeof(replies) - length, reply, strlen(reply))) {
    Serial.println(F("FAIL: constructing mixed reply fixture"));
    return;
  }
  stream.input = replies;
  if (!GPS.waitForSentence("$PAIR001,062,", 2, false, 100) ||
      strncmp(GPS.lastNMEA(), "$PAIR001,062,0*", 14)) {
    Serial.println(F("FAIL: proprietary reply after navigation was not accepted"));
    return;
  }
  stream.input = "$PQTMVERNO*58\n";
  if (!GPS.waitForSentence("$PQTMVERNO", 1, false, 100)) {
    Serial.println(F("FAIL: valid unknown zero-field response was rejected"));
    return;
  }
  Serial.println(F("PASS: proprietary replies and zero-field messages need no GPS whitelist"));

  // Simulate a complete line already published by an interrupt reader.
  stream.input = PMTK_AWAKE "\r\n";
  while (stream.available()) {
    GPS.read();
  }
  if (!GPS.waitForSentence(PMTK_AWAKE, 1, true, 100)) {
    Serial.println(F("FAIL: valid interrupt-published response was rejected"));
    return;
  }
  stream.input = PMTK_AWAKE "junk\r\n";
  while (stream.available()) {
    GPS.read();
  }
  if (GPS.waitForSentence(PMTK_AWAKE, 1, true, 100)) {
    Serial.println(F("FAIL: invalid interrupt-published response was accepted"));
    return;
  }
  stream.input = PMTK_AWAKE "\r\n";
  if (GPS.waitForSentence(NULL, 1, false, 100) ||
      !GPS.waitForSentence(PMTK_AWAKE, 1, false, 100)) {
    Serial.println(F("FAIL: NULL prefix consumed input or was accepted"));
    return;
  }
  Serial.println(F("PASS: interrupt-published replies and NULL prefix handling"));

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

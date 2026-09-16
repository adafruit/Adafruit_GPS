#include <Adafruit_GPS.h>

Adafruit_GPS GPS;
bool testsPassed = false;
bool checkSentence(const char *body, bool accepted, int checks,
                   const char *source, const char *sentence);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Adafruit GPS shared validation regression"));
  int framed = NMEA_HAS_DOLLAR + NMEA_HAS_CHECKSUM;
  int sourced = framed + NMEA_HAS_SOURCE;
  int parsed = sourced + NMEA_HAS_SENTENCE + NMEA_HAS_SENTENCE_P;
  if (!checkSentence("$GPGGA,", true, parsed, "GP", "GGA") ||
      !checkSentence("!GNGLL,", true, parsed, "GN", "GLL") ||
      !checkSentence("$PGTOP,11,2", true, parsed, "PG", "TOP") ||
      !checkSentence("$PCD,11,1", true, parsed, "P", "CD") ||
      !checkSentence("$GPZZZ,", false, sourced, "GP", "ZZZ") ||
      !checkSentence("$XXGGA,", false, framed, "", "")) {
    return;
  }
#ifdef NMEA_EXTENSIONS
  if (!checkSentence("$GPGSV,", false, sourced + NMEA_HAS_SENTENCE, "GP", "GSV")) {
    return;
  }
#else
  if (!checkSentence("$GPDBT,", false, sourced + NMEA_HAS_SENTENCE, "GP", "DBT")) {
    return;
  }
#endif
  Serial.println(F("PASS: recognized, unknown, proprietary, and known-only IDs"));

  // A recognized prefix must not select the wrong decoder.
  if (!checkSentence("$GPGGAX,", false, sourced, "GP", "GGAX") ||
      !checkSentence("$GPGSVX,", false, sourced, "GP", "GSVX") ||
      !checkSentence("$PCDX,11,1", false, sourced, "P", "CDX")) {
    return;
  }
  char extended[100] =
      "$GPGGAX,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,";
  GPS.addChecksum(extended);
  GPS.hour = 7;
  GPS.altitude = 12;
  if (GPS.parse(extended) || GPS.hour != 7 || GPS.altitude != 12) {
    Serial.println(F("FAIL: an extended address changed navigation data"));
    return;
  }
  Serial.println(F("PASS: sentence names require an exact match"));

  // These retain a matching XOR checksum but have invalid sentence syntax.
  const char *malformed[] = {"$GPGGA!,1", "$GPGGA,1\t2", "$GPGGA,1$2",
                             "$GPGGA,1!2", "$GPGGA,1*2", "$GPGGA,1\n2"};
  for (uint8_t i = 0; i < sizeof(malformed) / sizeof(malformed[0]); i++) {
    if (!checkSentence(malformed[i], false, NMEA_HAS_DOLLAR, "", "")) {
      return;
    }
  }
  Serial.println(F("PASS: malformed payloads are rejected despite valid XOR"));

  const char *shortLines[] = {"", "$", "!", "$GPGGA*", "$GPGGA*0", "$GPGGA*GG"};
  if (GPS.check(NULL) || GPS.parse(NULL) || GPS.thisCheck || GPS.thisSource[0] ||
      GPS.thisSentence[0]) {
    Serial.println(F("FAIL: NULL input or stale diagnostic fields"));
    return;
  }
  for (uint8_t i = 0; i < sizeof(shortLines) / sizeof(shortLines[0]); i++) {
    char line[20];
    strcpy(line, shortLines[i]);
    if (GPS.check(line) || GPS.thisSource[0] || GPS.thisSentence[0]) {
      Serial.println(F("FAIL: incomplete input was accepted"));
      return;
    }
  }
  Serial.println(F("PASS: NULL, empty, and incomplete input"));

  // Zero data fields remain a framing success, independent of GPS decoding.
  if (!checkSentence("$GPGGA", true, parsed, "GP", "GGA")) {
    return;
  }
  char knownZeroFields[] = "$GPGGA*56";
  if (GPS.parse(knownZeroFields)) {
    Serial.println(F("FAIL: GGA was decoded without its required fields"));
    return;
  }
  char zeroFields[] = "$PQTMVERNO*58";
  if (Adafruit_NMEA::validate(zeroFields, strlen(zeroFields)).status !=
          NMEA_FRAME_VALID || GPS.check(zeroFields) || GPS.thisCheck != sourced) {
    Serial.println(F("FAIL: zero-field proprietary response classification"));
    return;
  }
  const char *endings[] = {"", "\r", "\n", "\r\n"};
  for (uint8_t i = 0; i < sizeof(endings) / sizeof(endings[0]); i++) {
    char line[24] = "$GPGGA,*7a";
    strcat(line, endings[i]);
    if (!GPS.check(line) || GPS.thisCheck != parsed) {
      Serial.println(F("FAIL: lowercase checksum or supported line ending"));
      return;
    }
  }
  char badChecksum[] = "$GPGGA,*00";
  char badTrailer[] = "$GPGGA,*7Aextra";
  if (GPS.check(badChecksum) || GPS.thisCheck != NMEA_HAS_DOLLAR ||
      GPS.check(badTrailer) || GPS.thisCheck != NMEA_HAS_DOLLAR) {
    Serial.println(F("FAIL: bad checksum or trailer"));
    return;
  }
  Serial.println(F("PASS: zero fields, checksum cases, and line endings"));
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: GPS shared validation regression"));
  } else {
    Serial.println(F("FAIL: GPS shared validation regression"));
  }
}

bool checkSentence(const char *body, bool accepted, int checks,
                   const char *source, const char *sentence) {
  char line[100];
  strcpy(line, body);
  GPS.addChecksum(line);
  if (GPS.check(line) != accepted || GPS.thisCheck != checks ||
      strcmp(GPS.thisSource, source) || strcmp(GPS.thisSentence, sentence)) {
    Serial.print(F("FAIL: validation or diagnostics for "));
    Serial.println(body);
    return false;
  }
  return true;
}

#include <Adafruit_GPS.h>

bool parseSentence(char *sentence, const char *source, const char *type);

Adafruit_GPS GPS;
bool testsPassed = false;

void setup() {
  Serial.begin(115200);
  // Wait briefly for the Serial Monitor on native USB boards.
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  delay(250);
  Serial.println(F("Adafruit GPS marine source regression"));

  // Supported sentence types are independent of the source identifier.
  // Exercise the new identifiers and every existing two-letter source.
  const char sources[][3] = {
    "HC", "TI", "SD", "AI", "II", "WI", "GP", "PG", "GL", "GA", "GN"
  };
  for (uint8_t i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
    char sentence[80] = "$GPGLL,4916.45,N,12311.12,W,225444,A";
    sentence[1] = sources[i][0];
    sentence[2] = sources[i][1];
    if (!parseSentence(sentence, sources[i], "GLL")) {
      return;
    }
  }

  // PG must still be recognized before the shorter proprietary P prefix.
  char legacy[24] = "$PGTOP,11,2";
  if (!parseSentence(legacy, "PG", "TOP") || GPS.antenna != 2) {
    return;
  }
  char proprietary[24] = "$PCD,11,1";
  if (!parseSentence(proprietary, "P", "CD") || GPS.antenna != 2) {
    return;
  }

  char unknown[80] = "$XXGLL,4916.45,N,12311.12,W,225444,A";
  GPS.addChecksum(unknown);
  if (GPS.parse(unknown)) {
    Serial.println(F("FAIL: unknown source was accepted"));
    return;
  }
  Serial.println(F("PASS: unknown source is rejected"));

#ifdef NMEA_EXTENSIONS
  // Typical magnetic heading, water temperature, and sounder sentences.
  char compass[32] = "$HCHDM,238.5,M";
  if (!parseSentence(compass, "HC", "HDM")) {
    return;
  }
  char temperature[32] = "$TIMTW,18.5,C";
  if (!parseSentence(temperature, "TI", "MTW")) {
    return;
  }
  char depth[48] = "$SDDBT,32.8,f,10.0,M,5.5,F";
  if (!parseSentence(depth, "SD", "DBT")) {
    return;
  }
  Serial.println(F("PASS: marine extension sentences"));
#endif
  testsPassed = true;
}

void loop() {
  if (testsPassed) {
    Serial.println(F("PASS: marine source regression"));
  } else {
    Serial.println(F("FAIL: marine source regression"));
  }
  delay(2000);
}

bool parseSentence(char *sentence, const char *source, const char *type) {
  GPS.addChecksum(sentence);
  if (!GPS.parse(sentence) || strcmp(GPS.lastSource, source) ||
      strcmp(GPS.lastSentence, type)) {
    Serial.print(F("FAIL: source or sentence identity for "));
    Serial.println(sentence);
    return false;
  }
  Serial.print(F("PASS: "));
  Serial.println(sentence);
  return true;
}

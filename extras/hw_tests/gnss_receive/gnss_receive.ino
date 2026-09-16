// Engineering reception test. No commands are sent to any GNSS module.
// Classic Nano: default SoftwareSerial RX D8, TX D7 left disconnected.
// Define GNSS_TEST_I2C for PA1010D at 0x10 on A4/A5 instead.
// HILBERT ESP32-S3: LC29H(EA) GPIO8/460800 and ATGM336H GPIO38/115200.
// Keep receiver power wiring unchanged. A position fix is reported, not required.
#include <Adafruit_GPS.h>

struct ReceiveCounts {
  uint32_t bytes = 0;
  uint32_t valid = 0;
  uint32_t checksumErrors = 0;
  uint32_t formatErrors = 0;
  uint32_t parsed = 0;
};

#if defined(ARDUINO_ARCH_ESP32)
HardwareSerial firstPort(1), secondPort(2);
Adafruit_GPS firstGPS(&firstPort), secondGPS(&secondPort);
ReceiveCounts firstCounts, secondCounts;
#elif defined(GNSS_TEST_I2C)
Adafruit_GPS firstGPS(&Wire);
ReceiveCounts firstCounts;
#else
SoftwareSerial firstPort(8, 7);
Adafruit_GPS firstGPS(&firstPort);
ReceiveCounts firstCounts;
#endif

uint32_t lastReport = 0;
bool receiverReady = false;
void receiveGPS(Adafruit_GPS &gps, ReceiveCounts &counts,
                const __FlashStringHelper *name);
void reportGPS(Adafruit_GPS &gps, ReceiveCounts &counts,
               const __FlashStringHelper *name);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }
  delay(250);
  Serial.println(F("Adafruit GNSS receive regression"));
#if defined(ARDUINO_ARCH_ESP32)
  firstPort.setRxBufferSize(4096);
  secondPort.setRxBufferSize(4096);
  if (!firstPort.setPins(8, -1) || !secondPort.setPins(38, -1) ||
      !firstGPS.begin(460800) || !secondGPS.begin(115200)) {
    Serial.println(F("FAIL: HILBERT receive UART initialization"));
    return;
  }
#elif defined(GNSS_TEST_I2C)
  if (!firstGPS.begin(0x10)) {
    Serial.println(F("FAIL: PA1010D did not acknowledge I2C address 0x10"));
    return;
  }
#else
  firstGPS.begin(9600);
  // The module drives this line at 3.3 V; disable the Nano's 5 V pull-up.
  pinMode(8, INPUT);
#endif
  receiverReady = true;
  Serial.println(F("Receive interface initialized; waiting for NMEA"));
}

void loop() {
  if (!receiverReady) {
    delay(1000);
    return;
  }
#if defined(ARDUINO_ARCH_ESP32)
  receiveGPS(firstGPS, firstCounts, F("LC29H"));
  receiveGPS(secondGPS, secondCounts, F("ATGM"));
#else
  receiveGPS(firstGPS, firstCounts, F("NANO"));
#endif
  if (millis() - lastReport >= 2000) {
    lastReport = millis();
#if defined(ARDUINO_ARCH_ESP32)
    reportGPS(firstGPS, firstCounts, F("LC29H"));
    reportGPS(secondGPS, secondCounts, F("ATGM"));
#else
    reportGPS(firstGPS, firstCounts, F("NANO"));
#endif
  }
}

void receiveGPS(Adafruit_GPS &gps, ReceiveCounts &counts,
                const __FlashStringHelper *name) {
  // Limit each turn so two busy UARTs can both make progress.
  for (uint16_t i = 0; i < 256 && gps.available(); i++) {
    if (gps.read()) {
      counts.bytes++;
    }
    if (!gps.newNMEAreceived()) {
      continue;
    }
    char *line = gps.lastNMEA();
    Serial.print(name);
    Serial.print(' ');
    Serial.print(line);
    nmea_sentence_t sentence = Adafruit_NMEA::validate(line, strlen(line));
    if (sentence.status == NMEA_FRAME_VALID) {
      counts.valid++;
    } else if (sentence.status == NMEA_FRAME_BAD_CHECKSUM) {
      counts.checksumErrors++;
    } else {
      counts.formatErrors++;
    }
    if (gps.parse(line)) {
      counts.parsed++;
    }
  }
}

void reportGPS(Adafruit_GPS &gps, ReceiveCounts &counts,
               const __FlashStringHelper *name) {
  Serial.print(F("STATUS "));
  Serial.print(name);
  Serial.print(F(" bytes="));
  Serial.print(counts.bytes);
  Serial.print(F(" valid="));
  Serial.print(counts.valid);
  Serial.print(F(" checksumErrors="));
  Serial.print(counts.checksumErrors);
  Serial.print(F(" formatErrors="));
  Serial.print(counts.formatErrors);
  Serial.print(F(" parsed="));
  Serial.print(counts.parsed);
  bool freshFix = gps.fix && gps.secondsSinceFix() < 5;
  Serial.print(F(" fix="));
  Serial.print(freshFix);
  Serial.print(F(" ggaQuality="));
  Serial.print(gps.fixquality);
  if (freshFix) {
    Serial.print(F(" latitudeE7="));
    Serial.print(gps.latitude_fixed);
    Serial.print(F(" longitudeE7="));
    Serial.print(gps.longitude_fixed);
  }
  Serial.println();
}

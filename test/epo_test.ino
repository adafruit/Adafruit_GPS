#include "Adafruit_GPS.h"

Adafruit_GPS gps = Adafruit_GPS();

void setup() {
  Serial.begin(9600);
  Serial.println("Hello world");
  gps.begin(9600);
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(500);
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(500);
  gps.sendCommand(PGCMD_NOANTENNA);
  delay(500);
  gps.endEpoUpload();
}

bool result = false;

void loop() {
/*
  Serial.println("loop begin");
  char c = gps.read();
  if (!result) {
    result = gps.startEpoUpload(4000);
    if (result) {
      Serial.println("EPO Upload start succeeded");
    } else {
      Serial.println("EPO Upload start failed");
    }
  }
  delay(1);
  */
}

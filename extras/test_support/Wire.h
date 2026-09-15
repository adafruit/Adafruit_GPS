#ifndef GPS_TEST_WIRE_H
#define GPS_TEST_WIRE_H

#include "Arduino.h"

class TwoWire {
public:
  void begin() { unexpectedHardware(); }
  void beginTransmission(uint8_t) { unexpectedHardware(); }
  uint8_t endTransmission(bool = true) {
    unexpectedHardware();
    return 4;
  }
  size_t write(uint8_t) {
    unexpectedHardware();
    return 0;
  }
  uint8_t requestFrom(uint8_t, uint8_t, bool) {
    unexpectedHardware();
    return 0;
  }
  int read() {
    unexpectedHardware();
    return -1;
  }
};

#endif

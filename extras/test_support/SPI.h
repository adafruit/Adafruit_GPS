#ifndef GPS_TEST_SPI_H
#define GPS_TEST_SPI_H

#include "Arduino.h"

#define MSBFIRST 1
#define SPI_MODE0 0

class SPISettings {
public:
  SPISettings(uint32_t, uint8_t, uint8_t) {}
};

class SPIClass {
public:
  void begin() { unexpectedHardware(); }
  void beginTransaction(SPISettings) { unexpectedHardware(); }
  void endTransaction() { unexpectedHardware(); }
  uint8_t transfer(uint8_t) {
    unexpectedHardware();
    return 0;
  }
};

#endif

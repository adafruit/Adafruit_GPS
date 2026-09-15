// Minimal Arduino compatibility for host regressions, not a hardware emulator.
#ifndef GPS_TEST_ARDUINO_H
#define GPS_TEST_ARDUINO_H

#include <algorithm>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using std::max;
using std::min;
typedef uint8_t byte;

#define PROGMEM
#define PSTR(text) (text)
#define strcmp_P strcmp
#define strncmp_P strncmp
#define strcpy_P strcpy
#define strlen_P strlen
#define pgm_read_byte(address) (*(const uint8_t *)(address))
#define RAD_TO_DEG 57.295779513082320876798154814105
#define OUTPUT 1
#define HIGH 1
#define LOW 0

class __FlashStringHelper;
#define F(text) ((const __FlashStringHelper *)(text))

uint32_t millis();
void delay(uint32_t milliseconds);
void yield();
void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
void unexpectedHardware();

inline bool isDigit(int c) { return isdigit((unsigned char)c); }
inline bool isAlpha(int c) { return isalpha((unsigned char)c); }

class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t c) = 0;
  size_t print(const char *text) {
    size_t count = 0;
    while (*text) {
      count += write(*text++);
    }
    return count;
  }
  size_t print(const __FlashStringHelper *text) {
    return print((const char *)text);
  }
  size_t print(char value) { return write(value); }
  size_t print(int value) { return print((long)value); }
  size_t print(unsigned value) { return print((unsigned long)value); }
  size_t print(long value) {
    char text[32];
    snprintf(text, sizeof(text), "%ld", value);
    return print(text);
  }
  size_t print(unsigned long value) {
    char text[32];
    snprintf(text, sizeof(text), "%lu", value);
    return print(text);
  }
  size_t print(double value, int digits = 2) {
    char text[384];
    snprintf(text, sizeof(text), "%.*f", digits, value);
    return print(text);
  }
  size_t println() { return print("\r\n"); }
  template <typename T> size_t println(T value) {
    return print(value) + println();
  }
  size_t println(double value, int digits) {
    return print(value, digits) + println();
  }
  size_t println(const char *text) { return print(text) + print("\r\n"); }
  size_t println(const __FlashStringHelper *text) {
    return println((const char *)text);
  }
};

class Stream : public Print {
public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  virtual void flush() = 0;
};

class HardwareSerial : public Stream {
public:
  void begin(uint32_t) {}
  operator bool() const { return true; }
  int available() {
    unexpectedHardware();
    return 0;
  }
  int read() {
    unexpectedHardware();
    return -1;
  }
  int peek() {
    unexpectedHardware();
    return -1;
  }
  void flush() { fflush(stdout); }
  size_t write(uint8_t c) { return fputc(c, stdout) == EOF ? 0 : 1; }
};

extern HardwareSerial Serial;

#endif

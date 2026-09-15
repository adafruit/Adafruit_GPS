#include "Arduino.h"
#include <chrono>
#include <thread>

HardwareSerial Serial;

uint32_t millis() {
  static const auto start = std::chrono::steady_clock::now();
  return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start)
      .count();
}

void delay(uint32_t milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void yield() { std::this_thread::yield(); }

void unexpectedHardware() {
  fputs("FAIL: hardware access is not supported by host tests\n", stderr);
  abort();
}

void pinMode(int, int) { unexpectedHardware(); }
void digitalWrite(int, int) { unexpectedHardware(); }

/* Host regression for decimal parsing and command construction:
   g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc \
     src/Adafruit_NMEA.cpp extras/tests/nmea_utilities/nmea_utilities.cpp \
     -o /tmp/nmea_utilities
   /tmp/nmea_utilities
*/

#include "Adafruit_NMEA.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expectNumber(nmea_span_t field, nmea_number_status_t status,
                         int64_t coefficient = 0, uint8_t decimalPlaces = 0);
static void expectCommand(const char *body, const char *expected);
static void expectRejected(const char *body, size_t length);

int main() {
  struct {
    const char *text;
    int64_t coefficient;
    uint8_t decimalPlaces;
  } numbers[] = {{"0", 0, 0},
                 {"-0.000", 0, 3},
                 {"-123.4500", -1234500, 4},
                 {"+12", 12, 0},
                 {"-.5", -5, 1},
                 {"1.", 1, 0},
                 {"00012.0300", 120300, 4},
                 {"0009223372036854775807", INT64_MAX, 0},
                 {"-0009223372036854775808", INT64_MIN, 0},
                 {"1000000000000000000", INT64_C(1000000000000000000), 0},
                 {"4807.038123456", INT64_C(4807038123456), 9},
                 {"9223372036854775807", INT64_MAX, 0},
                 {"-9223372036854775808", INT64_MIN, 0},
                 {"922337203685477580.7", INT64_MAX, 1},
                 {"-922337203685477580.8", INT64_MIN, 1}};
  for (const auto &number : numbers)
    expectNumber({number.text, strlen(number.text)}, NMEA_NUMBER_VALID,
                 number.coefficient, number.decimalPlaces);
  puts("PASS: exact decimals, retained zeros, and signed 64-bit limits");

  const char *badNumbers[] = {
      ".",  "+",   "-",   "+.",  "--1", "1.2.3", " 1",
      "1 ", "1e2", "NaN", "inf", "1,2", "1*00",  "9223372036854775808x"};
  for (const char *text : badNumbers)
    expectNumber({text, strlen(text)}, NMEA_NUMBER_BAD_FORMAT);
  const char *overflow[] = {"9223372036854775808", "-9223372036854775809",
                            "99999999999999999999999999",
                            "10000000000000000000", "0009223372036854775808"};
  for (const char *text : overflow)
    expectNumber({text, strlen(text)}, NMEA_NUMBER_OUT_OF_RANGE);
  expectNumber({NULL, 100}, NMEA_NUMBER_MISSING);
  expectNumber({"1", 0}, NMEA_NUMBER_EMPTY);
  const char embeddedNul[] = {'1', 0, '2'};
  expectNumber({embeddedNul, sizeof(embeddedNul)}, NMEA_NUMBER_BAD_FORMAT);
  const char highByte[] = {'1', (char)0x80};
  expectNumber({highByte, sizeof(highByte)}, NMEA_NUMBER_BAD_FORMAT);
  puts("PASS: missing, empty, malformed, and overflowing numbers are distinct");

  const char bounded[] = {'-', '1', '.', '2'};
  expectNumber({bounded, sizeof(bounded)}, NMEA_NUMBER_VALID, -12, 1);
  expectNumber({"12x", 2}, NMEA_NUMBER_VALID, 12, 0);
  char fraction[258];
  memset(fraction, '0', sizeof(fraction));
  fraction[1] = '.';
  expectNumber({fraction, 257}, NMEA_NUMBER_VALID, 0, 255);
  expectNumber({fraction, sizeof(fraction)}, NMEA_NUMBER_OUT_OF_RANGE);
  char zeros[300];
  memset(zeros, '0', sizeof(zeros));
  expectNumber({zeros, sizeof(zeros)}, NMEA_NUMBER_VALID);
  puts("PASS: bounded numeric input and decimal-place limits");

  expectCommand("PQTMVERNO", "$PQTMVERNO*58\r\n");
  expectCommand("PQTMVERNO,", "$PQTMVERNO,*74\r\n");
  expectCommand("PAIR002", "$PAIR002*38\r\n");
  expectCommand("PAIR001,004,0", "$PAIR001,004,0*3F\r\n");
  expectCommand("PMTK220,1000", "$PMTK220,1000*1F\r\n");
  puts("PASS: known command checksums and every output capacity boundary");

  const char *badBodies[] = {"",     ",",   ",A",  "A B",  "$A",     "!A",
                             "A*41", "A\r", "A\n", "A,\t", "A,\x7f", "A,\x80"};
  for (const char *body : badBodies)
    expectRejected(body, strlen(body));
  expectRejected(NULL, 10);
  expectRejected("A", SIZE_MAX);
  expectRejected(embeddedNul, sizeof(embeddedNul));
  assert(Adafruit_NMEA::buildCommand(NULL, 100, "A", 1) == 0);
  puts("PASS: malformed commands and invalid arguments produce no partial "
       "output");

  char overlap[40];
  memset(overlap, 'A', sizeof(overlap));
  assert(Adafruit_NMEA::buildCommand(overlap, 20, overlap + 4, 7) == 0);
  assert(overlap[0] == 0);
  for (size_t i = 1; i < sizeof(overlap); i++)
    assert(overlap[i] == 'A');
  memset(overlap, 'A', sizeof(overlap));
  assert(Adafruit_NMEA::buildCommand(overlap + 4, 20, overlap, 7) == 0);
  for (size_t i = 0; i < sizeof(overlap); i++) {
    if (i == 4)
      assert(overlap[i] == 0);
    else
      assert(overlap[i] == 'A');
  }
  memset(overlap, 'A', sizeof(overlap));
  assert(Adafruit_NMEA::buildCommand(overlap, 20, overlap, 7) == 0);
  puts("PASS: overlapping source and destination ranges are rejected");

  char adjacent[13];
  memcpy(adjacent + 10, "A,1", 3);
  assert(Adafruit_NMEA::buildCommand(adjacent, 10, adjacent + 10, 3) == 9);
  assert(strcmp(adjacent, "$A,1*5C\r\n") == 0);
  assert(memcmp(adjacent + 10, "A,1", 3) == 0);
  memcpy(adjacent, "A,1", 3);
  assert(Adafruit_NMEA::buildCommand(adjacent + 3, 10, adjacent, 3) == 9);
  assert(strcmp(adjacent + 3, "$A,1*5C\r\n") == 0);
  assert(memcmp(adjacent, "A,1", 3) == 0);
  puts("PASS: adjacent buffers and nonterminated command bodies are supported");
  return 0;
}

static void expectNumber(nmea_span_t field, nmea_number_status_t status,
                         int64_t coefficient, uint8_t decimalPlaces) {
  nmea_decimal_t result = Adafruit_NMEA::parseDecimal(field);
  assert(result.status == status);
  assert(Adafruit_NMEA::validateDecimal(field) == status);
  nmea_number_status_t nonnegative = status;
  if (status == NMEA_NUMBER_VALID && coefficient < 0)
    nonnegative = NMEA_NUMBER_OUT_OF_RANGE;
  assert(Adafruit_NMEA::validateDecimal(field, false) == nonnegative);
  assert(result.coefficient == coefficient);
  assert(result.decimalPlaces == decimalPlaces);
}

static void expectCommand(const char *body, const char *expected) {
  size_t required = strlen(expected) + 1;
  for (size_t capacity = 0; capacity <= required + 1; capacity++) {
    size_t allocated = capacity;
    if (!allocated)
      allocated = 1;
    char *output = (char *)malloc(allocated);
    assert(output != NULL);
    memset(output, '?', allocated);
    size_t length =
        Adafruit_NMEA::buildCommand(output, capacity, body, strlen(body));
    if (capacity < required) {
      assert(length == 0);
      if (capacity)
        assert(output[0] == 0);
      else
        assert(output[0] == '?');
      for (size_t i = 1; i < allocated; i++)
        assert(output[i] == '?');
    } else {
      assert(length == required - 1);
      assert(strcmp(output, expected) == 0);
      assert(Adafruit_NMEA::validate(output, length).status ==
             NMEA_FRAME_VALID);
      if (capacity > required)
        assert(output[required] == '?');
    }
    free(output);
  }
}

static void expectRejected(const char *body, size_t length) {
  char output[64];
  memset(output, '?', sizeof(output));
  assert(Adafruit_NMEA::buildCommand(output, sizeof(output), body, length) ==
         0);
  assert(output[0] == 0);
  for (size_t i = 1; i < sizeof(output); i++)
    assert(output[i] == '?');
}

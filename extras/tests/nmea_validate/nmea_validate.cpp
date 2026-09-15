/* Host regression for the hardware-independent core. From the repository root:
   g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc \
     src/Adafruit_NMEA.cpp extras/tests/nmea_validate/nmea_validate.cpp \
     -o /tmp/nmea_validate
   /tmp/nmea_validate
*/

#include "Adafruit_NMEA.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect(const char *data, size_t length, nmea_frame_status_t status);
static void expectText(nmea_span_t span, const char *text);

int main() {
  const char *valid[] = {
      "$A*41", "$A*41\r",       "$A*41\n",           "$A*41\r\n",
      "!A*41", "$PQTMVERNO*58", "$PAIR001,004,0*3f", "$A, *4D"};
  for (const char *text : valid)
    expect(text, strlen(text), NMEA_FRAME_VALID);
  puts("PASS: checksums, line endings, unknown and proprietary addresses");

  nmea_sentence_t sentence =
      Adafruit_NMEA::validate("$PQTMVERNO*58", sizeof("$PQTMVERNO*58") - 1);
  expectText(sentence.address, "PQTMVERNO");
  assert(sentence.fields.data == NULL && sentence.fields.length == 0);
  sentence =
      Adafruit_NMEA::validate("$PQTMVERNO,*74", sizeof("$PQTMVERNO,*74") - 1);
  assert(sentence.status == NMEA_FRAME_VALID);
  assert(sentence.fields.data != NULL && sentence.fields.length == 0);
  sentence = Adafruit_NMEA::validate("$A,,*41", 7);
  assert(sentence.status == NMEA_FRAME_VALID);
  expectText(sentence.fields, ",");
  puts("PASS: absent, empty, and trailing empty fields stay distinct");

  const char *malformed[] = {
      "",          "$",       "$A",       "$A*",       "$A*4",      "$A*4Z",
      "$A*Z1",     "$A*+1",   "$A*411",   "$A*41junk", "$A*41\n\r", "$A*41\r\r",
      "$A*41\n\n", "$*00",    "$,*2C",    "$A B*23",   "$A,*",      "$A**41",
      "$A,$*49",   "$A,!*4C", "$A,\t*64", "A*41",      "$A*41$B*42"};
  for (const char *text : malformed)
    expect(text, strlen(text), NMEA_FRAME_BAD_FORMAT);
  expect("$PQTMVERNO*59", sizeof("$PQTMVERNO*59") - 1, NMEA_FRAME_BAD_CHECKSUM);
  expect(NULL, 0, NMEA_FRAME_BAD_FORMAT);
  expect(NULL, 100, NMEA_FRAME_BAD_FORMAT);
  const char embeddedNul[] = {'$', 'A', ',', 0, '*', '6', 'D'};
  expect(embeddedNul, sizeof(embeddedNul), NMEA_FRAME_BAD_FORMAT);
  const char nonAscii[] = {'$', 'A', ',', (char)0x80, '*', 'E', 'D'};
  expect(nonAscii, sizeof(nonAscii), NMEA_FRAME_BAD_FORMAT);
  puts("PASS: malformed framing, checksum digits, control bytes, and NULL "
       "input");

  const char fix[] =
      "$GPGGA,120009,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*40\r\n";
  const size_t length = sizeof(fix) - 1;
  // Each allocation ends at the declared length, so AddressSanitizer detects
  // any lookahead past it. The complete checksum needs no optional CR/LF.
  for (size_t i = 0; i <= length; i++) {
    char *bounded = (char *)malloc(i ? i : 1);
    assert(bounded != NULL);
    memcpy(bounded, fix, i);
    nmea_frame_status_t status = NMEA_FRAME_BAD_FORMAT;
    if (i >= length - 2)
      status = NMEA_FRAME_VALID;
    expect(bounded, i, status);
    assert(memcmp(bounded, fix, i) == 0);
    free(bounded);
  }
  puts("PASS: every truncated prefix and exact-length nonterminated input");

  sentence = Adafruit_NMEA::validate(fix, length);
  expectText(sentence.address, "GPGGA");
  expectText(sentence.fields,
             "120009,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
  nmea_sentence_t other =
      Adafruit_NMEA::validate("$PQTMVERNO*58", sizeof("$PQTMVERNO*58") - 1);
  expectText(other.address, "PQTMVERNO");
  expectText(sentence.address, "GPGGA");
  assert(sentence.text.data == fix && sentence.text.length == length);
  puts("PASS: views borrow input and independent results do not overwrite it");
  return 0;
}

static void expect(const char *data, size_t length,
                   nmea_frame_status_t status) {
  nmea_sentence_t result = Adafruit_NMEA::validate(data, length);
  if (result.status != status) {
    fprintf(stderr, "FAIL: length %zu, expected status %d, got %d\n", length,
            status, result.status);
    abort();
  }
  assert(result.text.data == data);
  assert(result.text.length == (data ? length : 0));
  if (status != NMEA_FRAME_VALID) {
    assert(result.address.data == NULL && result.address.length == 0);
    assert(result.fields.data == NULL && result.fields.length == 0);
  }
}

static void expectText(nmea_span_t span, const char *text) {
  assert(span.data != NULL && span.length == strlen(text));
  assert(memcmp(span.data, text, span.length) == 0);
}

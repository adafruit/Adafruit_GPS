/* Host regression for bounded field iteration. From the repository root:
   g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc \
     src/Adafruit_NMEA.cpp extras/tests/nmea_fields/nmea_fields.cpp \
     -o /tmp/nmea_fields
   /tmp/nmea_fields
*/

#include "Adafruit_NMEA.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void expectField(nmea_span_t &remaining, const char *expected);
static void expectEnd(nmea_span_t &remaining);

int main() {
  // No NUL terminator: AddressSanitizer catches reading beyond this array.
  char text[] = {'A', ',', ',', 'B', ','};
  nmea_span_t remaining = {text, sizeof(text)};
  expectField(remaining, "A");
  expectField(remaining, "");
  expectField(remaining, "B");
  expectField(remaining, "");
  expectEnd(remaining);
  expectEnd(remaining);
  assert(memcmp(text, "A,,B,", sizeof(text)) == 0);
  puts("PASS: nonterminated input, empty fields, and repeated end-of-fields");

  const char commas[] = {',', ','};
  remaining = {commas, sizeof(commas)};
  expectField(remaining, "");
  expectField(remaining, "");
  expectField(remaining, "");
  expectEnd(remaining);
  puts("PASS: leading, consecutive, and trailing empty fields");

  nmea_sentence_t sentence =
      Adafruit_NMEA::validate("$PQTMVERNO*58", sizeof("$PQTMVERNO*58") - 1);
  assert(sentence.status == NMEA_FRAME_VALID);
  remaining = sentence.fields;
  expectEnd(remaining);
  sentence =
      Adafruit_NMEA::validate("$PQTMVERNO,*74", sizeof("$PQTMVERNO,*74") - 1);
  assert(sentence.status == NMEA_FRAME_VALID);
  remaining = sentence.fields;
  expectField(remaining, "");
  expectEnd(remaining);
  puts("PASS: validated zero-field and one-empty-field messages differ");

  const char outside[] = "AB,CD";
  remaining = {outside, 2};
  expectField(remaining, "AB");
  expectEnd(remaining);
  remaining = {outside, 0};
  expectField(remaining, "");
  expectEnd(remaining);
  remaining = {NULL, 100};
  expectEnd(remaining);
  puts("PASS: declared boundaries and absent cursors are respected");

  char longText[300];
  memset(longText, 'A', sizeof(longText));
  remaining = {longText, sizeof(longText)};
  nmea_span_t field = Adafruit_NMEA::nextField(remaining);
  assert(field.data == longText && field.length == sizeof(longText));
  expectEnd(remaining);
  puts("PASS: fields longer than 255 bytes keep their full length");

  nmea_span_t first = {"A,B", 3};
  nmea_span_t second = {"1,,2", 4};
  nmea_span_t saved = Adafruit_NMEA::nextField(first);
  expectField(second, "1");
  expectField(first, "B");
  expectField(second, "");
  expectEnd(first);
  expectField(second, "2");
  expectEnd(second);
  assert(saved.length == 1 && saved.data[0] == 'A');
  puts("PASS: interleaved cursors and previously returned views stay "
       "independent");
  return 0;
}

static void expectField(nmea_span_t &remaining, const char *expected) {
  const char *start = remaining.data;
  nmea_span_t field = Adafruit_NMEA::nextField(remaining);
  assert(field.data != NULL && field.data == start);
  assert(field.length == strlen(expected));
  if (field.length)
    assert(memcmp(field.data, expected, field.length) == 0);
}

static void expectEnd(nmea_span_t &remaining) {
  nmea_span_t field = Adafruit_NMEA::nextField(remaining);
  assert(field.data == NULL && field.length == 0);
  assert(remaining.data == NULL && remaining.length == 0);
}

// Automatically run by extras/tests/run_tests.py, without Arduino or hardware.
#include "Adafruit_NMEA.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <type_traits>

static void expectText(nmea_span_t span, const char *text);
static nmea_frame_status_t feedLine(Adafruit_NMEA &receiver, const char *text,
                                    uint32_t start);
static void checkCapacity();
static void checkStorage();
static void checkIndependentReceivers();

int main() {
  static_assert(!std::is_copy_constructible<Adafruit_NMEA>::value,
                "Receivers must not share writable buffers through copies");
  static_assert(!std::is_copy_assignable<Adafruit_NMEA>::value,
                "Receivers must not copy receive state");
  char first[32], second[32];
  Adafruit_NMEA receiver(first, second, sizeof(first));
  assert(!receiver.lastText().data && !receiver.lastText().length);
  nmea_sentence_t sentence = receiver.lastSentence();
  assert(sentence.status == NMEA_FRAME_INCOMPLETE);
  assert(!sentence.text.data && !sentence.address.data &&
         !sentence.fields.data);
  assert(!sentence.text.length && !sentence.address.length &&
         !sentence.fields.length);
  assert(receiver.sentenceStartedAt() == 0 &&
         receiver.sentenceReceivedAt() == 0);
  assert(first[0] == 0 && second[0] == 0);
  for (char c : "noise\r\n")
    assert(receiver.feed(c, 10) == NMEA_FRAME_INCOMPLETE);
  assert(receiver.lastSentence().status == NMEA_FRAME_INCOMPLETE);

  assert(feedLine(receiver, "$A*41\r\n", 0) == NMEA_FRAME_VALID);
  sentence = receiver.lastSentence();
  expectText(sentence.text, "$A*41\r\n");
  expectText(receiver.lastText(), "$A*41\r\n");
  expectText(sentence.address, "A");
  assert(sentence.fields.data == NULL);
  assert(sentence.text.data == first && first[sentence.text.length] == 0);
  assert(receiver.sentenceStartedAt() == 0 &&
         receiver.sentenceReceivedAt() == 6);
  puts("PASS: noise, initial state, CR/LF, borrowed storage, and zero "
       "timestamp");

  const char *partial = "$unfinished";
  for (size_t i = 0; i < strlen(partial); i++)
    assert(receiver.feed(partial[i], 20 + i) == NMEA_FRAME_INCOMPLETE);
  expectText(sentence.text, "$A*41\r\n");
  assert(receiver.sentenceStartedAt() == 0 &&
         receiver.sentenceReceivedAt() == 6);
  assert(feedLine(receiver, "!A,1*5C\n", 100) == NMEA_FRAME_VALID);
  sentence = receiver.lastSentence();
  expectText(sentence.text, "!A,1*5C\n");
  expectText(sentence.fields, "1");
  assert(sentence.text.data == second);
  assert(receiver.sentenceStartedAt() == 100 &&
         receiver.sentenceReceivedAt() == 107);
  assert(feedLine(receiver, "$PQTMVERNO*58\n", 200) == NMEA_FRAME_VALID);
  expectText(receiver.lastSentence().address, "PQTMVERNO");
  assert(receiver.lastSentence().text.data == first);
  puts("PASS: restart markers, LF-only lines, proprietary commands, and buffer "
       "reuse");

  const char *invalid[] = {"$A*40\n", "$A*4Z\n", "$A\n", "$A*41\rX\n"};
  for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
    nmea_frame_status_t expected = NMEA_FRAME_BAD_FORMAT;
    if (i == 0)
      expected = NMEA_FRAME_BAD_CHECKSUM;
    assert(feedLine(receiver, invalid[i], 300) == expected);
    sentence = receiver.lastSentence();
    assert(sentence.status == expected);
    expectText(sentence.text, invalid[i]);
    assert(!sentence.address.data && !sentence.fields.data);
    assert(receiver.sentenceStartedAt() == 300);
    assert(receiver.sentenceReceivedAt() == 300 + strlen(invalid[i]) - 1);
  }
  const uint8_t nulLine[] = {'$', 'A', ',', 0, '*', '6', 'D', '\n'};
  for (size_t i = 0; i < sizeof(nulLine); i++) {
    nmea_frame_status_t expected = NMEA_FRAME_INCOMPLETE;
    if (i == sizeof(nulLine) - 1)
      expected = NMEA_FRAME_BAD_FORMAT;
    assert(receiver.feed(nulLine[i], 400 + i) == expected);
  }
  sentence = receiver.lastSentence();
  assert(sentence.text.length == sizeof(nulLine));
  assert(memcmp(sentence.text.data, nulLine, sizeof(nulLine)) == 0);
  puts("PASS: invalid complete lines retain raw text and update timestamps");

  receiver.feed('$', 500);
  receiver.reset();
  assert(receiver.lastSentence().status == NMEA_FRAME_INCOMPLETE);
  assert(receiver.sentenceStartedAt() == 0 &&
         receiver.sentenceReceivedAt() == 0);
  assert(first[0] == 0 && second[0] == 0);
  assert(receiver.feed('\n', 501) == NMEA_FRAME_INCOMPLETE);
  assert(feedLine(receiver, "$A*41\n", 600) == NMEA_FRAME_VALID);
  puts("PASS: reset discards completed and partial lines and permits reuse");

  checkCapacity();
  checkStorage();
  checkIndependentReceivers();
  return 0;
}

static void expectText(nmea_span_t span, const char *text) {
  assert(span.data && span.length == strlen(text));
  assert(memcmp(span.data, text, span.length) == 0);
  assert(span.data[span.length] == 0 || span.data[span.length] == '*' ||
         span.data[span.length] == ',');
}

static nmea_frame_status_t feedLine(Adafruit_NMEA &receiver, const char *text,
                                    uint32_t start) {
  nmea_frame_status_t result = NMEA_FRAME_INCOMPLETE;
  for (size_t i = 0; i < strlen(text); i++) {
    result = receiver.feed(text[i], start + (uint32_t)i);
    if (text[i] != '\n')
      assert(result == NMEA_FRAME_INCOMPLETE);
  }
  return result;
}

static void checkCapacity() {
  const char line[] = "$A*41\r\n";
  for (size_t capacity = 0; capacity <= sizeof(line) + 1; capacity++) {
    // Allocations end at capacity so ASan detects a misplaced NUL or LF.
    char *first = (char *)malloc(capacity ? capacity : 1);
    char *second = (char *)malloc(capacity ? capacity : 1);
    assert(first && second);
    memset(first, '?', capacity ? capacity : 1);
    memset(second, '?', capacity ? capacity : 1);
    {
      Adafruit_NMEA receiver(first, second, capacity);
      unsigned overflows = 0;
      for (size_t i = 0; i < sizeof(line) - 1; i++) {
        nmea_frame_status_t status = receiver.feed(line[i], i);
        if (capacity < 2) {
          assert(status == NMEA_FRAME_BAD_FORMAT);
          assert(first[0] == '?' && second[0] == '?');
        } else if (capacity < sizeof(line) && i == capacity - 1) {
          assert(status == NMEA_FRAME_OVERFLOW);
          overflows++;
        } else if (capacity >= sizeof(line) && i == sizeof(line) - 2) {
          assert(status == NMEA_FRAME_VALID);
        } else {
          assert(status == NMEA_FRAME_INCOMPLETE);
        }
      }
      if (capacity >= sizeof(line))
        expectText(receiver.lastSentence().text, line);
      else
        assert(receiver.lastSentence().status == NMEA_FRAME_INCOMPLETE);
      assert(overflows == (unsigned)(capacity >= 2 && capacity < sizeof(line)));
    }
    free(first);
    free(second);
  }

  char first[8], second[8];
  Adafruit_NMEA receiver(first, second, sizeof(first));
  assert(feedLine(receiver, "$A*41\n", 10) == NMEA_FRAME_VALID);
  nmea_span_t retained = receiver.lastSentence().text;
  for (char c : "$AAAAAA") {
    if (c)
      assert(receiver.feed(c, 30) == NMEA_FRAME_INCOMPLETE);
  }
  assert(receiver.feed('A', 31) == NMEA_FRAME_OVERFLOW);
  for (char c : "ignored*41\n")
    assert(receiver.feed(c, 32) == NMEA_FRAME_INCOMPLETE);
  expectText(retained, "$A*41\n");
  assert(receiver.sentenceStartedAt() == 10 &&
         receiver.sentenceReceivedAt() == 15);
  assert(feedLine(receiver, "!A*41\n", 40) == NMEA_FRAME_VALID);
  expectText(receiver.lastSentence().text, "!A*41\n");

  char largeFirst[300], largeSecond[300], body[270], command[300];
  memset(body, '0', sizeof(body));
  body[0] = 'A';
  body[1] = ',';
  assert(Adafruit_NMEA::buildCommand(command, sizeof(command), body,
                                     sizeof(body)));
  Adafruit_NMEA large(largeFirst, largeSecond, sizeof(largeFirst));
  assert(feedLine(large, command, 50) == NMEA_FRAME_VALID);
  expectText(large.lastSentence().text, command);
  puts("PASS: every capacity boundary, overflow recovery, and lines over 255 "
       "bytes");
}

static void checkStorage() {
  char storage[16];
  char original[16];
  memset(storage, '?', sizeof(storage));
  memcpy(original, storage, sizeof(storage));
  char *first[] = {NULL, storage, storage, storage, storage + 7};
  char *second[] = {storage, NULL, storage, storage + 7, storage};
  for (size_t i = 0; i < sizeof(first) / sizeof(first[0]); i++) {
    Adafruit_NMEA receiver(first[i], second[i], 8);
    assert(receiver.feed('$', 1) == NMEA_FRAME_BAD_FORMAT);
    receiver.reset();
    assert(receiver.lastSentence().status == NMEA_FRAME_INCOMPLETE);
    assert(memcmp(storage, original, sizeof(storage)) == 0);
  }
  for (unsigned reverse = 0; reverse < 2; reverse++) {
    char *a = storage;
    char *b = storage + 8;
    if (reverse) {
      a = storage + 8;
      b = storage;
    }
    Adafruit_NMEA adjacent(a, b, 8);
    assert(feedLine(adjacent, "$A*41\n", 10) == NMEA_FRAME_VALID);
    assert(feedLine(adjacent, "!A*41\n", 20) == NMEA_FRAME_VALID);
  }
  puts("PASS: NULL and overlapping storage stay untouched; adjacent buffers "
       "work");
}

static void checkIndependentReceivers() {
  char a[16], b[16], c[16], d[16];
  Adafruit_NMEA first(a, b, sizeof(a));
  Adafruit_NMEA second(c, d, sizeof(c));
  const char line[] = "$A*41\n";
  for (size_t i = 0; i < sizeof(line) - 1; i++) {
    nmea_frame_status_t expected = NMEA_FRAME_INCOMPLETE;
    if (i == sizeof(line) - 2)
      expected = NMEA_FRAME_VALID;
    assert(first.feed(line[i], UINT32_MAX - 2 + (uint32_t)i) == expected);
    assert(second.feed(line[i], 100 + i) == expected);
  }
  assert(first.sentenceStartedAt() == UINT32_MAX - 2);
  assert(first.sentenceReceivedAt() == 2);
  assert(second.sentenceStartedAt() == 100 &&
         second.sentenceReceivedAt() == 105);
  nmea_span_t retained = second.lastSentence().text;
  first.reset();
  expectText(retained, line);
  expectText(second.lastSentence().text, line);
  assert(second.sentenceReceivedAt() == 105);
  puts("PASS: interleaved receivers have independent buffers, reset, and "
       "wrapping timestamps");
}

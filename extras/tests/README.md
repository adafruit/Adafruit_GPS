# Regression tests

Run every test on Linux with Python 3 and g++:

```sh
python3 extras/tests/run_tests.py
```

The runner discovers every `.cpp` and `.ino` in the test subdirectories.
Each C++ test supplies `main()`. Each Arduino sketch supplies `setup()`,
`loop()`, and a `testsPassed` flag that remains false until all checks pass.
The runner calls `setup()` and `loop()` once and returns failure when that
flag is false. New tests are picked up automatically without editing CI.

All tests use AddressSanitizer and UndefinedBehaviorSanitizer. Compiler
warnings, failed checks, sanitizer findings, and a 30-second execution timeout
fail the job. Other tests still run after an individual failure.

The Arduino sketches use the real library sources in four configurations:
`NMEA_EXTRAS=0` for the basic GPS API and `NMEA_EXTRAS=1` for marine extensions,
each with `NMEA_FLOAT_T=float` and `NMEA_FLOAT_T=double`. Definitions apply
consistently to the sketch and library. The RMB test checks that basic builds
reject the unsupported sentence and extended builds decode it without partial
updates on error. Standalone C++ core tests run once.

`extras/test_support` provides serial output and a real monotonic clock; the
wakeup timeout test takes ten seconds per configuration. Command-wait tests
also reject matching replies with invalid framing/checksums, keep invalid lines
in the sentence budget, and accept proprietary replies mixed with navigation.
I2C, SPI, GPIO, and hardware serial input abort if used. This runs the parser
and mock-stream regressions, not physical GPS/SD hardware or MCU emulation.
The sketches remain usable on Arduino boards with the normal Arduino core.

Coordinate precision tests preserve all nine retained fractional-minute digits
in the GNSS core and distinguish positions about 0.185 mm apart through the GPS
double output, including southern latitude and longitude near 180 degrees west.
The coordinate-text regression also checks integer-only decimal-degree output
with 11 fractional digits, including adjacent retained fractional-minute values
and every output capacity. This output preserves detail on AVR without a global
float-type override. The submillimeter positions deliberately share the same
legacy E7 coordinate. The default float fields and E7 fields are not
full-precision RTK outputs. Future receiver APIs and logging examples must
preserve the core precision through storage, public results, and formatting
without requiring a global float-type override.

The shared position decoder regression checks GGA/RMC/GLL results, independent
fix and GGA-quality status, empty/missing fields, exact coordinates through text
output, and rejection of malformed sentences without partial measurements.
Returned values remain independent of the input buffer and other receivers.

The GNSS receiver regression feeds interleaved bytes into two receiver subclasses,
checks independent timestamps and exact results, and exercises partial lines,
overflow recovery, invalid checksums, reset, and mixed proprietary replies.
`Adafruit_GNSS(firstBuffer, secondBuffer, capacity)` inherits `feed()` and
`lastSentence()` from the NMEA core; call `lastPosition()` after a completed line.
It returns that line's position, without caching or merging a previous fix.

Physical reception tests live separately in `extras/hw_tests/gnss_receive`.
Build and run them explicitly on the documented Nano or HILBERT fixture; the
host test runner does not operate attached boards.

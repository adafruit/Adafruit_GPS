# Adafruit GPS Library [![Arduino Library CI](https://github.com/adafruit/Adafruit_GPS/actions/workflows/githubci.yml/badge.svg)](https://github.com/adafruit/Adafruit_GPS/actions/workflows/githubci.yml) [![Documentation](https://raw.githubusercontent.com/adafruit/ci-arduino/master/assets/doxygen_badge.svg)](https://adafruit.github.io/Adafruit_GPS/html/index.html)

Read time, date, location, speed, and altitude from a GPS module in an Arduino
sketch. The library reads NMEA sentences (the text messages sent by the GPS) and
makes supported fields available as variables in your program.

Use it with the [Ultimate GPS Breakout](https://www.adafruit.com/product/746),
[GPS FeatherWing](https://www.adafruit.com/product/3133), or
[Mini GPS PA1010D](https://learn.adafruit.com/adafruit-mini-gps-pa1010d-module).
The library supports UART serial, I2C, and SPI connections; choose an interface
your GPS module actually provides. Module-specific configuration commands are
not universal across GPS manufacturers.

## Getting started

1. In the Arduino IDE Library Manager, search for **Adafruit GPS** and install
   **Adafruit GPS Library**. For the `shield_sdlog` example, also install
   **SdFat - Adafruit Fork** by Bill Greiman from Library Manager.
2. Follow the wiring guide for your module:
   [Ultimate GPS](https://learn.adafruit.com/adafruit-ultimate-gps) or
   [PA1010D](https://learn.adafruit.com/adafruit-mini-gps-pa1010d-module).
   Check its power and signal voltage requirements before connecting it.
3. Open **File > Examples > Adafruit GPS Library** and select an example below.
   Choose your board and USB port, then upload the sketch.
4. Open the Serial Monitor at the speed used by `Serial.begin()` in the sketch
   (usually **115200 baud**). This is separate from the GPS serial speed, which
   is usually **9600 baud** in these examples.
5. Start with an **EchoTest** to check that GPS messages arrive. Then use a
   **Parsing** example to display time and position. Put the antenna where it
   has a clear view of the sky; receiving text does not mean a position fix has
   been acquired yet.

This README describes the source in this repository. Library Manager releases
may not yet include the latest merged fixes.

## Choosing a connection

| Connection | When to use it | Example setup |
| --- | --- | --- |
| Hardware serial | Your board has a spare serial port, such as `Serial1`. | GPS TX connects to that port's RX; GPS RX connects to its TX. Select the port with `GPSSerial` in the sketch. |
| Software serial | An Uno or Metro 328 uses its main serial port for USB, so you need another pair of pins for the GPS. | The supplied examples use GPS TX to D8 and GPS RX to D7. SoftwareSerial support and usable pins depend on the board. |
| I2C | Your GPS exposes SDA and SCL, as the PA1010D does. | Connect SDA, SCL, power, and ground. Examples use address `0x10`; on an Uno or Metro Mini, SDA is A4 and SCL is A5. |
| SPI | Your GPS specifically supports SPI. | Follow its wiring requirements; the echo example uses CS on D10 and reset on D9. |

## Exact received positions

`GPS.lastPosition()` returns the latest received GGA, RMC, or GLL sentence as
an independent `gnss_position_t`. Its coordinate components retain all nine
fractional-minute digits without a global float-type override. Use
`Adafruit_GNSS::formatCoordinate()` to print them as decimal degrees on any board,
including AVR. Preserving digits does not increase the receiver's accuracy.

For a sketch that polls `GPS.read()` in `loop()`, after initializing `GPS`:

```cpp
GPS.read();
if (GPS.newNMEAreceived()) {
  gnss_position_t position = GPS.lastPosition();
  GPS.lastNMEA(); // Acknowledge this line; lastPosition() leaves the flag alone.
  if (position.validation.status == GNSS_SENTENCE_VALID &&
      position.fixStatus == NMEA_NUMBER_VALID && position.fix &&
      position.latitude.status == NMEA_NUMBER_VALID &&
      position.longitude.status == NMEA_NUMBER_VALID) {
    char coordinate[GNSS_COORDINATE_TEXT_SIZE];
    Adafruit_GNSS::formatCoordinate(coordinate, sizeof(coordinate), position.latitude);
    Serial.print(coordinate);
    Serial.print(F(", "));
    Adafruit_GNSS::formatCoordinate(coordinate, sizeof(coordinate), position.longitude);
    Serial.println(coordinate);
  }
}
```

The result describes one sentence, without merging previous fixes. Empty fields
stay empty, and replies or other sentence types return `GNSS_SENTENCE_UNSUPPORTED`.
Before any complete line, or for a line with an invalid frame/checksum, the result
is `GNSS_SENTENCE_INVALID_FRAME`. Partial or oversized input leaves the previous
complete line available. If an interrupt calls `GPS.read()`, synchronize it with
`lastPosition()` so the receive buffer cannot change during decoding.

This getter does not update `GPS.fix`, `GPS.latitudeDegrees`, or other legacy
fields; continue using `GPS.parse(GPS.lastNMEA())` when those fields are needed.
Likewise, parsing a separate caller-supplied string does not change this getter's
received sentence. Decode such a string with `Adafruit_NMEA::validate()` followed
by `Adafruit_GNSS::parsePosition()` instead.

## Example guide

Every example is linked below. An **echo** sketch shows the GPS's raw text;
a **parsing** sketch turns that text into individual readings.

| Example | What it does |
| --- | --- |
| [GPS_HardwareSerial_EchoTest](examples/GPS_HardwareSerial_EchoTest/GPS_HardwareSerial_EchoTest.ino) | Passes text between the Serial Monitor and a GPS on `Serial1`. A good first wiring test on a board with a spare serial port. |
| [GPS_HardwareSerial_Parsing](examples/GPS_HardwareSerial_Parsing/GPS_HardwareSerial_Parsing.ino) | Displays time, fix status, position, speed, and altitude from a GPS on `Serial1`. |
| [GPS_HardwareSerial_Timing](examples/GPS_HardwareSerial_Timing/GPS_HardwareSerial_Timing.ino) | Shows how old the last time and position updates are, and how to keep time between GPS messages. |
| [GPS_SoftwareSerial_EchoTest](examples/GPS_SoftwareSerial_EchoTest/GPS_SoftwareSerial_EchoTest.ino) | Shows incoming GPS text using pins D8 and D7. Start here with an Uno or Metro 328 and a serial GPS. |
| [GPS_SoftwareSerial_Parsing](examples/GPS_SoftwareSerial_Parsing/GPS_SoftwareSerial_Parsing.ino) | Reads a serial GPS using D8 and D7 and displays its time, fix status, and location. |
| [GPS_I2C_EchoTest](examples/GPS_I2C_EchoTest/GPS_I2C_EchoTest.ino) | Passes text between the Serial Monitor and an I2C GPS at `0x10`. Start here with a PA1010D. |
| [GPS_I2C_Parsing](examples/GPS_I2C_Parsing/GPS_I2C_Parsing.ino) | Displays time, fix status, position, speed, and altitude from an I2C GPS. |
| [GPS_I2C_OLEDdebug](examples/GPS_I2C_OLEDdebug/GPS_I2C_OLEDdebug.ino) | Currently another Serial Monitor/I2C bridge, despite its name. It does not initialize or draw to an OLED. |
| [GPS_SPI_EchoTest](examples/GPS_SPI_EchoTest/GPS_SPI_EchoTest.ino) | Resets an SPI GPS and passes text between it and the Serial Monitor. Requires an SPI-capable GPS and the reset connection. |
| [shield_sdlog](examples/shield_sdlog/shield_sdlog.ino) | Saves GPS sentences to numbered text files on an SD card using an Uno/Metro 328 logger shield setup. See the logging notes below. |
| [GPS_HardwareSerial_LOCUS_Start](examples/GPS_HardwareSerial_LOCUS_Start/GPS_HardwareSerial_LOCUS_Start.ino) | Starts recording in the GPS module's own LOCUS memory, using `Serial1`. |
| [GPS_SoftwareSerial_LOCUS_Start](examples/GPS_SoftwareSerial_LOCUS_Start/GPS_SoftwareSerial_LOCUS_Start.ino) | Starts LOCUS recording using D8 and D7. |
| [GPS_HardwareSerial_LOCUS_Status](examples/GPS_HardwareSerial_LOCUS_Status/GPS_HardwareSerial_LOCUS_Status.ino) | Starts LOCUS recording, then reports settings, record count, and memory usage from the GPS on `Serial1`. |
| [GPS_SoftwareSerial_LOCUS_Status](examples/GPS_SoftwareSerial_LOCUS_Status/GPS_SoftwareSerial_LOCUS_Status.ino) | Starts LOCUS recording, then reports its settings and memory usage using D8 and D7. |
| [GPS_HardwareSerial_LOCUS_DumpBasic](examples/GPS_HardwareSerial_LOCUS_DumpBasic/GPS_HardwareSerial_LOCUS_DumpBasic.ino) | Requests saved LOCUS records over `Serial1` and prints the raw response. Turns off regular NMEA output first. |
| [GPS_SoftwareSerial_LOCUS_DumpBasic](examples/GPS_SoftwareSerial_LOCUS_DumpBasic/GPS_SoftwareSerial_LOCUS_DumpBasic.ino) | Requests saved LOCUS records using D8 and D7 and prints the raw response. Turns off regular NMEA output first. |
| [GPS_HardwareSerial_LOCUS_Erase](examples/GPS_HardwareSerial_LOCUS_Erase/GPS_HardwareSerial_LOCUS_Erase.ino) | **Erases saved LOCUS records** in the GPS connected to `Serial1`. |
| [GPS_SoftwareSerial_LOCUS_Erase](examples/GPS_SoftwareSerial_LOCUS_Erase/GPS_SoftwareSerial_LOCUS_Erase.ino) | **Erases saved LOCUS records** in the GPS connected to D8 and D7. |
| [NMEA_EXTENSIONS](examples/NMEA_EXTENSIONS/NMEA_EXTENSIONS.ino) | Generates and parses simulated navigation and instrument messages without GPS hardware. Use a board with more RAM than an Uno. |
| [blank](examples/blank/blank.ino) | Leaves the processor idle so a compatible board's USB-to-serial converter can talk directly to the GPS. Uses special wiring in the sketch; it is not the usual D8/D7 connection. |

## Logging

**SD logging** writes files to a separate SD card. The `shield_sdlog` example
uses GPS pins D8/D7, the hardware SPI pins, and SD chip select D10. It writes raw
NMEA text into files named `GPSLOG0000.TXT` through `GPSLOG9999.TXT`, using
SdFat's long-filename support. Each start creates a new file; if all names are
used, the logger stops instead of reopening an old log. With `LOG_FIXONLY` set
to `false`, it also saves sentences while the GPS has no position fix. Set it
to `true` if you only want records with a fix.

The Uno and Metro 328 have only 2 KB of RAM. SD logging leaves limited room for
additional buffers and variables; keep constant print messages in `F()` when
extending the example. The logger uses AVR-specific code and needs adaptation
for other processor families.

**LOCUS logging** uses memory inside a compatible GPS module and does not need
an SD card. Support depends on the module and its firmware. The Start and Status
examples enable recording; the Erase examples delete saved records. After a
DumpBasic example disables normal NMEA output, run a Parsing example to restore
the usual output configuration.

## Reading data and troubleshooting

- Call `GPS.read()` frequently so incoming data is not lost. When
  `GPS.newNMEAreceived()` is true, pass `GPS.lastNMEA()` to `GPS.parse()` and
  check its return value before using that sentence's results.
- Check `GPS.fix` before using a position. GPS text and time information can
  arrive before a valid location is available. For applications that need fresh
  data, also check `GPS.secondsSinceFix()`.
- If the Serial Monitor shows nothing, check its baud rate, the selected USB
  port, wiring, and whether the sketch waits for `Serial`. A `while (!Serial)`
  wait on a native USB board requires opening the Serial Monitor; remove that
  wait when the project must run without a computer.
- Some PA1010D firmware stops responding over I2C in standby and cannot be
  woken with an I2C command. `wakeup()` then returns `false` after its response
  timeout; recovery may require cycling the GPS module's power. See
  [issue #124](https://github.com/adafruit/Adafruit_GPS/issues/124).
- The extended marine/instrument features are normally enabled on non-AVR
  boards and disabled on AVR to save space. See
  [Adafruit_GPS.h](src/Adafruit_GPS.h) for the `NMEA_EXTRAS` build setting.

> **Parser limitations:** Feed the parser complete, supported NMEA sentences
> from your GPS. Checks reject bad checksums and many incomplete sentences, but
> the parser is not hardened for arbitrary input. Malformed or unsupported input
> may still cause memory corruption or faults.

See the [API documentation](https://adafruit.github.io/Adafruit_GPS/html/index.html)
for constructors, commands, and available data fields. To report a problem,
include your board, GPS model and connection, library version, example or small
reproduction sketch, and the relevant error output or raw NMEA sentences in a
[GitHub issue](https://github.com/adafruit/Adafruit_GPS/issues).

## License and support

Written by Limor Fried/Ladyada for Adafruit Industries.
Distributed under the BSD license; see [license.txt](license.txt).
The copyright and license notices must be included in any redistribution.

Adafruit invests time and resources providing this open source code;
please support Adafruit and open-source hardware by purchasing
products from [Adafruit](https://www.adafruit.com/).

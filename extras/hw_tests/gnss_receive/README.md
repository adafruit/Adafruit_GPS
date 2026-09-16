# GNSS reception test

This engineering sketch exercises the real library receive and parse paths.
It echoes complete raw sentences and reports byte, valid-frame, checksum-error,
format-error, and parsed-sentence counts every two seconds. Unsupported sentence
types count as valid frames but do not increase the parsed count.

No receiver commands are sent. A position fix is reported when available and
is not required to establish communication. `fix` is true only when the latest
fix status is valid and less than five seconds old; coordinates use that same
check, including for RMC/GLL-only output. `ggaQuality` reports the last GGA fix
quality separately and may be stale when GGA sentences stop arriving. Keep raw
captures private if they contain location information.

| Fixture | Build selection | Receive wiring |
| --- | --- | --- |
| Classic Nano V3 + PA6H | Default AVR build | GPS TX to D8; D7 disconnected; 9600 baud |
| Classic Nano V3 + SAM-M8Q | Default AVR build | GPS TX to D8; D7 disconnected; 9600 baud |
| Classic Nano V3 + PA1010D | Define `GNSS_TEST_I2C` | SDA A4, SCL A5, address 0x10 |
| HILBERT ESP32-S3 | ESP32 build | LC29H(EA) GPIO8 at 460800; ATGM336H GPIO38 at 115200 |

The Nano uses its own USB for upload and serial monitoring while seated in
Jumperless. Leave Jumperless UART_TX/D0 and UART_RX/D1 disconnected. SoftwareSerial
TX D7 stays physically disconnected, and the sketch disables the D8 input
pull-up so it does not pull a 3.3 V receiver output toward the Nano's 5 V rail.

In the established Jumperless fixture, route PA6H TX row 5 **or** SAM-M8Q TX row
17 to D8, never both. PA1010D SDA is row 24 to A4 and SCL is row 25 to A5. Retain
the existing physical power and ground jumpers; no power routing is changed by
this sketch.

Build from the library checkout with `--library .` and one of these selections:

- Nano: `--fqbn arduino:avr:nano:cpu=atmega328`.
- Nano I2C: the Nano target plus
  `--build-property compiler.cpp.extra_flags=-DGNSS_TEST_I2C`.
- HILBERT:
  `--fqbn esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,FlashMode=qio,PSRAM=enabled`.

Read the serial monitor at 115200 baud. Verify that valid and parsed counts
continue increasing. Record any startup errors separately and compare error
counts during steady reception; do not hide increasing errors. HILBERT reports
both receivers independently. A missing fix is different from missing UART
or I2C traffic.

These hardware tests are compiled and run explicitly. The host regression
runner exercises synthetic parser tests without connected boards.

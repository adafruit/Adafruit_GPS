# LC29H receiver API proposal

This is a reviewable declaration-only milestone. [Adafruit_LC29H.h](Adafruit_LC29H.h)
is outside `src/` and cannot yet be used as a working driver. It builds on the
implemented `Adafruit_GNSS` and `Adafruit_NMEA` types; the older NMEA proposal in
this directory is historical. No existing GPS API, compiled code, or receive
buffer size changes here.

## First supported path

Target the LC29H(EA) fitted to HILBERT first. The new class inherits
`Adafruit_GNSS`, independently of the MTK-specific `Adafruit_GPS` API. The legacy
GPS class currently contains a GNSS receiver; changing its inheritance is a
separate compatibility decision.

The first implementation will share byte framing and exact GGA/RMC/GLL position
decoding, and add PAIR acknowledgments plus firmware-version replies. Callers
provide the existing pair of receive buffers. There is no second reader, hidden
fix history, or heap allocation. Each complete line must be handled before
feeding the next one. Increasing a particular LC29H sketch's buffer capacity
will not increase the Uno logger's buffers.

`lastPosition()` keeps the shared exact coordinate components and GGA fix
quality. Applications must check sentence/field validity before using them.
`degreesE7` and legacy floats are convenience values; use the exact components
or `formatCoordinate()` when retaining the finer coordinate precision. Logging
the raw sentence retains the original text. A command reply replaces the latest
line, so an application wanting a previous position must copy that result.

## Protocol contract

Source: Quectel's [LC29H Series & LC79H(AL) GNSS Protocol Specification V1.4](https://www.quectel.com/content/uploads/2022/02/Quectel_LC29H_SeriesLC79HAL_GNSS_Protocol_Specification_V1.4.pdf),
sections 1.1, 2.3.1, 2.4.1, 2.4.50, and 3. This covers EA; it does not establish
support for every LC29H variant or firmware.

- `PAIR001` carries a command ID and result. Results 0/1/2/3/4/5 mean positive
  acknowledgment, processing, failure, unsupported command, parameter error,
  and busy. Processing is not completion; a positive acknowledgment is not a
  position fix or configuration readback. For example, `PAIR865` also returns
  a separate query result.
- `PQTMVERNO` takes no fields. Its success reply has version, build date, and
  build time; failure uses `ERROR,<code>`. PQTM codes 1/2/3 denote parameter,
  execution, and unsupported-command errors.
- RTCM is binary. EA supports correction input; this milestone defines neither
  RTCM decoding nor a correction transport.

The proposed parser deliberately requires exact addresses and field counts.
PAIR unknown result values fail decoding; version errors retain numeric codes
through 255, including future codes. These bounds and failure policies are
library choices. A zero-field firmware query echoed back is not identification.
Version text is borrowed and is not automatically converted into capabilities.

## Receive ownership and subsequent command work

One owner reads the connection and feeds bytes to this instance. Navigation and
command responses take that same path. The new static decoders also accept a
validated supplied sentence for host tests, without constructing a receiver.
Like `Adafruit_GNSS::parsePosition()`, they trust a view produced by the shared
validator and require its storage to remain unchanged during decoding.

A following transport milestone should initialize an explicitly supplied UART,
query firmware without changing receiver settings, and keep processing
navigation during commands. It must define elapsed-time deadlines, partial
writes, cancellation, and one outstanding command per connection. Match the
PAIR command ID or complete PQTM address, not merely a text prefix. Processing
replies must not extend the original deadline indefinitely. No automatic retry
of state-changing commands is proposed.

PAIR has no per-request sequence token. Draining old input and serializing
commands reduces stale-reply risk but cannot distinguish a delayed reply to an
earlier identical command. An eventual API must document this limit and use
readback where the command supplies it; it must not claim perfect correlation.

For now, `buildCommand()` already constructs a bounded firmware query from the
nine-byte body `PQTMVERNO`. Its output is a command to send, not proof of device
identity. UART startup, command waiting, reset, configuration setters, and RTCM
input will be implemented in separate reviewed increments. The inherited
`reset()` remains a parser reset, so hardware restart needs a distinct name.

## Bench contract

The established [receive fixture](../hw_tests/gnss_receive/README.md) is HILBERT
ESP32-S3 with LC29H(EA), receiving on GPIO8 at 460800 baud. Existing testing has
proved reception, not command transmission or a corrected RTK fix.

The locally inspected HILBERT Rev A schematic identifies U2 as LC29H(EA):
TXD connects directly to GPIO8; GPIO9 reaches RXD through R8 (1 kOhm), with R14
(5.1 kOhm) to ground. The receiver supply is 3.3 V and VDD_EXT is labeled 2.8 V.
The existing sketch leaves TX unconfigured. Before enabling GPIO9, verify the
actual board revision, fitted divider, and receiver input-voltage limits. Leave
reset, enable, and other pins untouched during the first firmware query.

No hardware changes or commands are part of this proposal.

## Implementation acceptance checks

- Interleaved navigation, firmware replies, and PAIR responses through one
  receiver; two receiver instances remain independent.
- Exact address matching, every defined acknowledgment result, unrelated IDs,
  processing followed by acceptance, and failure without partial output.
- Bad checksum/framing, missing/empty/extra fields, invalid integer syntax,
  numeric overflow, unknown PAIR results, and echoed version queries.
- Version success/error lifetime and replacement by the next complete line;
  parser reset, overflow recovery, and unsupported sentences.
- The existing high-precision coordinate cases through the LC29H subclass,
  including changes smaller than one E7 unit, on AVR and ESP32-S3 builds.

These are requirements for the implementation PR, not claims of tests run for
this declarations-only proposal.

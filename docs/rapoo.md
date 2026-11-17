# Rapoo mouse battery support

## Why the original program missed the mouse

`PollAllDevices()` enumerates Windows HID top-level collections once per refresh. Each vendor
driver recognises a protocol, adds a `DeviceStatus`, and claims its VID/PID so the generic drivers
do not add it again. The UI already handles percentage, charging, and the last reading while asleep.

There was no Rapoo driver. The VT3 Max Gen 2 sends its battery in a vendor-defined input report,
not the standard HID Battery Strength usage or a Windows Bluetooth battery property. The existing
generic HID/Bluetooth fallbacks therefore cannot decode its USB receiver.

## Observed hardware and packets

Tested on Windows with the owner's VT3 Max Gen 2 on 2026-11-16:

| Connection | VID:PID | Interface | Usage page:usage | Input / output size |
|---|---|---|---|---|
| 2.4 GHz receiver | `24AE:1417` | 1 | `FF00:0002` | 19 / 19 bytes |
| USB cable | `24AE:4617` | 1 | `FF00:0002` | 19 / 19 bytes |

Both identify themselves as `Rapoo Gaming Device`. The other collections include standard mouse,
keyboard and media inputs, vendor configuration collections, and a 513-byte firmware interface.
The battery driver only opens `FF00:0002` with a recognised input size. It never opens the firmware
interface and never sends an output report, feature report, handshake, or configuration command.

Captured wireless report, including report ID:

```text
07 20 03 20 03 20 03 01 19 00 04 84 00 17 46 00 02 03 BC
```

Captured cable/charging report:

```text
07 22 03 20 03 20 03 02 19 00 04 84 00 17 46 00 02 03 00
```

Offsets below count the report ID as byte zero, matching this project's `HidDev::Read()`:

| Byte(s) | Meaning | Example |
|---|---|---|
| 0 | Report ID | `07` |
| 1 low nibble | Connection: 0 receiver, 1 Bluetooth, 2 cable | `20` / `22` |
| 1 high nibble | Sensor information; not part of the connection type | `2` |
| 2 | Current DPI gear | `03` |
| 3–4, 5–6 | X/Y DPI, little endian | `0320` = 800 |
| 7 | Battery state: 0 invalid, 1 discharging, 2 charging | `01` / `02` |
| 8 | Battery percentage, 0–100 | `19` = 25% |
| 13–14 | Mouse product ID, little endian, when present | `4617` |

The live receiver capture reported `25%`, and the complete application subsequently returned
`27%, charging` over the cable. The parser test includes the actual wireless capture. Charging and cable connectivity
were also verified against live hardware; these are not inferred just from a USB device being present.
After the owner disconnected the cable and reconnected the receiver, the final executable returned
`52%` over `2.4 GHz receiver`, without the charging flag, and retained the same `rapoo:4617` key.
This also verifies the cable-to-receiver transition against live hardware.

## Other Rapoo mice

The official Rapoo Hub uses this `07` layout for its Nordic 54L mouse driver. Its manifests
list VT0, VT1, VT2, VT3, VT3S, VT7, VT9 and additional variants. Inspection of the current bundle
identified 69 VT variants using `VT_nrf54L`; this is evidence of a shared protocol, not 69 hardware
validations or a claim to cover most of the entire Rapoo catalog. Support is selected by Rapoo VID,
the dedicated collection, input length, and a valid status report, so an unlisted model on the same
protocol can still report its battery. A small name table covers common models; others retain their
HID product name or show `Rapoo wireless mouse`.

Product names and IDs vary across generations and regions. The tested `1417/4617` pair is called
`VT3 MAX` in the current official manifest; the owner identifies it as VT3 Max Gen 2. The current
manifest's separate `VT3 MAX V2` has IDs `1B06/4B06`. Community tables assigning `1417` to VT9 Pro
must not be used as the sole basis for naming or detecting this device.

Older VT firmware, documented for VT3 Pro (`1215/4415`), instead pushes a 7-byte report on the same
`FF00:0002` usage:

```text
BB B0 [mode] [DPI low] [DPI high] [battery state] [percentage]
```

For this legacy format, states 1/2 mean discharging/charging. A charging percentage of `7F` means
unknown and is preserved as unknown. Transition state 0, unrelated `BC` reports, malformed packets,
and percentages above 100 are ignored. Legacy support has automated coverage, but has not been
verified with a physical legacy mouse here. This interpretation is restricted to product IDs
`1215/4415` and their 7-byte input collection. The Nordic profile never interprets `BB` packets.

This does **not** establish support for every Rapoo device. CH584 models such as VT1 SE and VT7 SE
use report `08` in the official driver and are not supported by this decoder. Other Telink revisions use a different
interpretation of the `BB` flags, some office models use active battery queries, and Bluetooth models
may expose only the Windows battery property. These remain with the existing fallbacks or need a
separate verified implementation. Do not send guessed queries to them.

## Polling and identity

- Reading is passive. The deadline is 4 seconds per matching collection, allowing a documented
  legacy push interval of roughly 3.2 seconds. Unrelated packets do not restart that deadline.
- No queued status is flushed. Each refresh opens a fresh handle and closes it after reading.
- The embedded mouse PID, or a known cable/receiver pair for legacy devices, gives the same UI key
  across cable changes. Cable and receiver readings are merged into one entry, preferring the cable's
  state while preserving a valid receiver percentage if the cable reports an unknown level.
- A timeout does not invent a percentage or claim the device. After a successful reading, the
  existing UI can retain that reading, dimmed, for 30 minutes of silence.
- The program's existing VID/PID-based identity model cannot distinguish multiple identical mice
  with the same model ID. Physical simultaneous cable/receiver operation was not tested; merging
  both inputs is covered by the simulated transport test.

## Validation

`make test` runs a portable decoder test and a Windows driver test with simulated HID responses.
Coverage includes the actual capture, 0% and 100%, charging, missing levels, invalid/truncated frames,
unsupported collections/vendors, protocol-profile isolation, timeouts, stable keys, and cable/receiver
deduplication without discarding a known percentage.

The full application was built locally with MSVC C++17 and the Windows SDK, and `--list` / `--dump`
were run against the actual mouse. The maintained MinGW build includes `rapoo.cpp` in `build.bat`;
the Makefile discovers it automatically. MinGW itself was not installed in the test environment.

## Protocol sources

- [Official Rapoo Hub](https://hub.rapoo.cn/), inspected 2026-11-16: bundled
  [JavaScript driver and manifests](https://hid.rapoo.cn/AHUB_WEB/dist/assets/js/index-exv-sdE5.js).
  Relevant decoders are `py`/`Hk` for `VT_nrf54L` and `qM`/`QM` for connection and battery-state
  meanings. The separate `VT_CH584` handler consumes report `08` and is outside this implementation.
  Minified names may change on future releases.
- [Device-Battery-Info: RapooProtocol.cs](https://github.com/PyFlat/Device-Battery-Info/blob/4553297903ecfb20e55116014b8ec437deaf640d/src/DeviceBatteryInfo/Sources/Rapoo/RapooProtocol.cs),
  for legacy VT3 Pro reports, charging state and the unknown-level sentinel.
- [ClickSync: protocol_api_rapoo.js](https://github.com/Nuitfanee/ClickSync/blob/3a945428b25bf6b2db0e8750f819666caff9cda5/src/protocols/protocol_api_rapoo.js)
  independently corroborates passive `07` status broadcasts and the battery offset. Its parser
  clamps invalid percentages; this implementation rejects them instead.

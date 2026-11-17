# Contributing

## Reporting whether a device works

Most drivers were written from protocol documentation and could not be tested on every model.
Please open an issue with:

- the device name, and whether it is connected by receiver, cable or Bluetooth
- the output of `WirelessBattery.exe --list`
- the output of `WirelessBattery.exe --dump`
- what the vendor's own software shows as the battery level

## Adding a device

1. Run `WirelessBattery.exe --dump` and find your device's vendor and product IDs, and the
   vendor-defined collections (usage page `FFxx`) with their report sizes.
2. Find the battery protocol. Good places to look: HeadsetControl, OpenRazer, Solaar, rivalcfg,
   OpenRGB, OpenLinkHub, or a USB capture of the vendor software (Wireshark + USBPcap).
3. If the vendor already has a driver in `src/drivers/`, add the product ID (and any differences) there.
   Otherwise create `src/drivers/<vendor>.cpp`:

   ```cpp
   #include "drivers.h"

   void PollAcme(PollContext& ctx) {
       for (auto [vid, pid] : ctx.Products(0x1234)) {           // unclaimed products of this vendor
           for (const HidColl* c : ctx.Find(vid, pid, [](const HidColl& c) { return c.page() == 0xFF00; })) {
               HidDev d;
               if (!d.Open(*c)) continue;
               std::vector<BYTE> r;
               if (!d.Write({ 0x01, 0x02 }) || d.Read(r, 500) < 3) continue;   // hidapi-style buffers
               DeviceStatus s;
               s.key = L"acme:" + Hex4(pid);
               s.name = HidProduct(*c);
               s.kind = DeviceKind::Mouse;
               s.via = L"2.4 GHz receiver";
               s.percent = Clamp100(r[2]);
               ctx.Add(s, vid, pid);
               break;
           }
       }
   }
   ```

4. Declare the function in `src/drivers/drivers.h`, call it from `PollAllDevices()` in
   `src/devices.cpp` (before the generic fallbacks), and add the file to `build.bat`.
   The `Makefile` picks it up automatically.
5. Update the device table in `README.md`.

### Rules for drivers

- **Read-only:** only send requests that read state. Never change settings, lighting or modes.
- **Exact matching:** only talk to devices you recognise by vendor and product ID, or by a
  vendor-specific collection that is unique to the protocol.
- **Always time out:** a missing device must not stall the poll for more than about a second.
  Passive protocols may need longer to cover a documented push interval (for example, Delux and
  Rapoo). Keep a single absolute deadline of at most 4 seconds per matching collection, do not
  restart it for unrelated reports, and document why the exception is needed.
- **Credit sources:** cite where the protocol comes from in a comment at the top of the file.

## Code style

C++17, Win32 only, no third-party dependencies. Keep the binary small. Format roughly as the
existing code does (4-space indent, 120-column lines).

## Automatic safety check

Every pull request is scanned by the "PR safety check" for things this app should never do (network
access, launching programs, reading keyboard/clipboard/screen, startup entries, binaries, build or
release changes). A flag is not a rejection: just explain the flagged lines in the PR description.

Using AI tools is fine, but please remove `Co-authored-by` lines that credit them (for example
`Co-authored-by: Claude ...`) from your commit messages. The check fails while they are present.

// Sony INZONE H9 II (054C:0FA8) and H5 (054C:0EBF) headsets: a Sony command layer tunnelled through
// HID (report 02, 64 bytes) on the vendor collection with usage page FF04.
//   command: [02][len][01][00 FC][8+n][96 C3][addr][event][type][tid lo][tid hi][payload][sum]
//   event:   [02][len][04][FF][..][00][96 C3][addr][event][type][tid lo][tid hi][payload][sum]
// Battery = event 04, type 01 (GET) to the headset (addr 0x41); reply payload [charging][percent],
// percent FF = headset off. Both directions carry a checksum and a transaction id.
// Protocol from github.com/Sapd/HeadsetControl (sony_inzone_protocol.hpp).
#include "drivers.h"

namespace {

struct Event { BYTE id, type; USHORT tid; std::vector<BYTE> payload; };

bool ParseEvent(const std::vector<BYTE>& b, Event& e) {
    if (b.size() < 15 || b[0] != 0x02) return false;
    int len = b[1];
    if (len < 12 || len > 62 || (int)b.size() < len + 2) return false;
    if (b[2] != 0x04 || b[3] != 0xFF || b[5] != 0x00 || b[6] != 0x96 || b[7] != 0xC3) return false;
    if ((b[8] >> 4) != 0x1) return false;  // must be addressed to the PC
    unsigned sum = 0;
    for (int i = 5; i <= len; i++) sum += b[i];
    if ((BYTE)sum != b[len + 1]) return false;
    e.id = b[9];
    e.type = b[10];
    e.tid = (USHORT)(b[11] | (b[12] << 8));
    e.payload.assign(b.begin() + 13, b.begin() + len + 1);
    return true;
}

bool GetBattery(HidDev& d, int& percent, bool& charging, bool& offline) {
    static USHORT tid = 1;
    if (++tid <= 1) tid = 2;  // tid 1 is used by the dongle's own notifications
    std::vector<BYTE> req(64, 0);
    req[0] = 0x02;
    req[1] = 12;     // HCI byte count (no payload)
    req[2] = 0x01;   // HCI command
    req[3] = 0x00;
    req[4] = 0xFC;   // vendor opcode 0xFC00
    req[5] = 8;
    req[6] = 0x96;
    req[7] = 0xC3;
    req[8] = 0x41;   // PC -> headset
    req[9] = 0x04;   // battery info
    req[10] = 0x01;  // GET
    req[11] = (BYTE)tid;
    req[12] = (BYTE)(tid >> 8);
    unsigned sum = 0;
    for (int i = 6; i <= 12; i++) sum += req[i];
    req[13] = (BYTE)sum;
    d.Flush();
    if (!d.Write(req)) return false;
    ULONGLONG end = GetTickCount64() + 1500;
    std::vector<BYTE> r;
    while ((LONGLONG)end - (LONGLONG)GetTickCount64() > 0) {
        int left = (int)((LONGLONG)end - (LONGLONG)GetTickCount64());
        if (d.Read(r, left) < 15) return false;
        Event e;
        if (!ParseEvent(r, e) || e.id != 0x04 || e.tid != tid || e.payload.size() < 2) continue;
        if (e.type != 0x10 && e.type != 0xA0) continue;  // RET or active notification
        offline = e.payload[1] == 0xFF;
        percent = offline ? -1 : (e.payload[1] <= 100 ? e.payload[1] : -1);
        charging = e.payload[0] != 0;
        return true;
    }
    return false;
}

}  // namespace

void PollSonyInzone(PollContext& ctx) {
    struct Model { USHORT pid; USHORT usage; const wchar_t* name; };
    for (const Model& m : { Model{ 0x0FA8, 0x0001, L"Sony INZONE H9 II" }, Model{ 0x0EBF, 0x0002, L"Sony INZONE H5" } }) {
        if (ctx.IsClaimed(0x054C, m.pid)) continue;
        for (const HidColl* c : ctx.Find(0x054C, m.pid, [&](const HidColl& c) { return c.page() == 0xFF04 && c.outLen() >= 64; })) {
            HidDev d;
            if (!d.Open(*c)) continue;
            int pct = -1;
            bool charging = false, offline = false;
            if (!GetBattery(d, pct, charging, offline)) continue;
            DeviceStatus s;
            s.key = L"inzone:" + Hex4(m.pid);
            s.name = m.name;
            s.kind = DeviceKind::Headset;
            s.via = L"2.4 GHz receiver";
            s.connected = !offline;
            s.percent = pct;
            s.charging = charging;
            ctx.Add(s, 0x054C, m.pid);
            break;
        }
    }
}

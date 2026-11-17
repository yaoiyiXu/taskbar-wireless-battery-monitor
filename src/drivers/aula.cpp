// AULA F75 / F87 Pro / F99 wireless keyboards on the Compx 2.4 GHz receiver (VID 3554, PID FA09).
// The receiver exposes a 20-byte vendor collection on usage page FF02 / usage 02. Battery is a
// 20-byte output report 0x13 carrying command 0x4A and a trailing sum checksum. The reply is
// another 0x13 frame whose command byte, with its top bit masked, is 0x4A. Its payload starts at
// byte 5: the level (1..100), then the state byte (0x01 on battery, 0x10 with the cable in).
// The receiver also pushes unrelated 0x13 frames (a 0x0A status among them), so the report id
// alone does not identify the reply.
// From github.com/PyFlat/Device-Battery-Info (PR 6) and github.com/deepan-alve/womier-l65-linux.
#include "drivers.h"

namespace {

BYTE Sum(const std::vector<BYTE>& b, size_t n) {
    unsigned s = 0;
    for (size_t i = 0; i < n; i++) s += b[i];
    return (BYTE)(s & 0xFF);
}

bool QueryBattery(HidDev& d, std::vector<BYTE>& r) {
    std::vector<BYTE> req(20, 0);
    req[0] = 0x13;
    req[1] = 0x4A;
    req[19] = Sum(req, 19);
    d.Flush();
    if (!d.Write(req)) return false;
    ULONGLONG end = GetTickCount64() + 1000;
    for (;;) {
        int left = (int)((LONGLONG)end - (LONGLONG)GetTickCount64());
        if (left <= 0) return false;
        int n = d.Read(r, left);
        if (n < 0) return false;
        if (n < 20) continue;
        if (r[0] != 0x13) continue;
        if ((r[1] & 0x7F) != 0x4A) continue;
        if (r[19] != Sum(r, 19)) continue;
        return true;
    }
}

}  // namespace

void PollAula(PollContext& ctx) {
    for (auto [vid, pid] : ctx.Products(0x3554)) {
        if (pid != 0xFA09) continue;  // other 3554 products are mice on a different protocol
        for (const HidColl* c : ctx.Find(vid, pid, [](const HidColl& c) {
                 return c.page() == 0xFF02 && c.usage() == 0x0002 && c.outLen() >= 20 && c.inLen() >= 20;
             })) {
            HidDev d;
            if (!d.Open(*c)) continue;
            std::vector<BYTE> r;
            // The first request can go unanswered while the 2.4 GHz link wakes up; retry once.
            if (!QueryBattery(d, r) && !QueryBattery(d, r)) continue;
            int level = r[5];
            if (level < 1 || level > 100) continue;
            DeviceStatus s;
            s.key = L"aula:" + Hex4(vid) + L":" + Hex4(pid);
            s.name = L"AULA wireless keyboard";
            s.kind = DeviceKind::Keyboard;
            s.via = L"2.4 GHz receiver";
            s.percent = level;
            s.charging = r[6] == 0x10;
            ctx.Add(s, vid, pid);
            break;
        }
    }
}

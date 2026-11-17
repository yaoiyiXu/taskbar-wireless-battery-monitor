// MCHOSE G87 2.4 GHz battery query. Protocol documentation:
// https://github.com/marsuyone/mchose-battery-tray (7206b460), README G87 section.
// Independently implemented using the documented command; no settings writes.
// Observed reply: 13 4A 01 00 02 38 01 00 ... 99 (56%).
#include "drivers.h"

void PollMchose(PollContext& ctx) {
    constexpr USHORT vid = 0x41E4, pid = 0x2001;
    if (ctx.IsClaimed(vid, pid)) return;
    for (const HidColl* c : ctx.Find(vid, pid, [](const HidColl& c) {
        return c.page() == 0xFF02 && c.usage() == 2 && c.inLen() == 20 && c.outLen() == 20;
    })) {
        HidDev d;
        if (!d.Open(*c)) continue;
        std::vector<BYTE> request(20, 0), reply;
        request[0] = 0x13;
        request[1] = 0x4A;
        request[2] = 1;
        d.Flush();
        if (!d.Write(request)) continue;
        const ULONGLONG deadline = GetTickCount64() + 1000;
        for (;;) {
            const auto now = GetTickCount64();
            if (now >= deadline) break;
            const int n = d.Read(reply, (int)(deadline - now));
            if (n < 0) break;
            if (n != 20 || reply[0] != 0x13 || reply[1] != 0x4A ||
                reply[2] != 1 || reply[3] != 0 || reply[4] != 2 || reply[5] > 100) continue;
            unsigned sum = 0;
            for (size_t i = 0; i < 19; ++i) sum += reply[i];
            if ((sum & 0xFF) != reply[19]) continue;
            DeviceStatus status;
            status.key = L"mchose:41E4:2001";
            status.name = L"MCHOSE G87";
            status.kind = DeviceKind::Keyboard;
            status.via = L"2.4 GHz receiver";
            status.percent = reply[5];
            // Charging flags are not confirmed for this model; do not guess.
            status.graceSec = 1800;
            ctx.Add(status, vid, pid);
            return;
        }
    }
}

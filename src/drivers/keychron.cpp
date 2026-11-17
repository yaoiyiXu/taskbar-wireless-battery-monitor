// Keychron wireless mice (VID 3434, e.g. M4), from Keychron's own web Launcher (launcher.keychron.com).
// 64-byte output report without report id: [01][00][81][01][0...][checksum], checksum (byte 63)
// = 0xA1 - sum(bytes 0..62). The reply [01][..][..][01][state][rate][vid lo][vid hi][pid lo][pid hi]
// [charging][battery %]... carries the mouse's own vendor id, which is checked so that only a
// genuine reply is accepted. Keychron keyboards do not report their battery this way (only over
// Bluetooth, which the Bluetooth fallback already covers).
// The 8K mice (M5 8K, G6 HE 8K ... on the TurboLink 8K receiver, Launcher protocol "8k_nordic") use
// the same request, but on the receiver bit 6 of the first byte is set (41 00 81 01 ...). Their reply
// [41][..][..][01][..][max rate][..]..[state][battery %][profile] has no vendor id; state 1 =
// charging, 3 is treated as 0.
#include "drivers.h"

namespace {

void SetChecksum(std::vector<BYTE>& payload) {  // payload = 64 bytes without the report id
    unsigned sum = 0;
    for (int i = 0; i < 63; i++) sum += payload[i];
    payload[63] = (BYTE)(0xA1 - sum);
}

std::wstring CleanName(std::wstring n) {
    if (n.empty() || n.find(L"Link") != std::wstring::npos || n.find(L"Receiver") != std::wstring::npos) return L"Keychron mouse";
    return n;
}

}  // namespace

namespace {

// Sends [cmd][00][81][01] and waits up to 600 ms for a reply starting with cmd whose byte 3 is 01.
bool Query(HidDev& d, BYTE cmd, std::vector<BYTE>& r) {
    std::vector<BYTE> payload(64, 0);
    payload[0] = cmd;
    payload[2] = 0x81;
    payload[3] = 0x01;
    SetChecksum(payload);
    std::vector<BYTE> req(1, 0x00);  // report id 0 (unnumbered)
    req.insert(req.end(), payload.begin(), payload.end());
    d.Flush();
    if (!d.Write(req)) return false;
    ULONGLONG end = GetTickCount64() + 600;
    while ((LONGLONG)end - (LONGLONG)GetTickCount64() > 0) {
        int left = (int)((LONGLONG)end - (LONGLONG)GetTickCount64());
        if (d.Read(r, left) < 13) return false;
        if (r[0] == cmd && r[3] == 0x01) return true;
    }
    return false;
}

}  // namespace

void PollKeychron(PollContext& ctx) {
    for (auto [vid, pid] : ctx.Products(0x3434)) {
        for (const HidColl* c : ctx.Find(vid, pid, [](const HidColl& c) { return c.page() >= 0xFF00 && c.outLen() == 65 && c.inLen() == 65; })) {
            HidDev d;
            if (!d.Open(*c)) continue;
            std::vector<BYTE> r, r1;
            int percent = -1;
            bool charging = false, wireless = true;
            bool got1 = Query(d, 0x01, r1);
            if (got1 && (USHORT)(r1[6] | (r1[7] << 8)) == 0x3434 && r1[11] <= 100) {
                percent = r1[11];  // M4 and other mice on the classic protocol: the reply carries the vendor id
                charging = r1[10] != 0;
            } else if (Query(d, 0x41, r) && r[10] <= 3 && r[11] <= 100) {
                percent = r[11];  // 8K mouse on its receiver
                charging = r[10] == 1;
            } else if (got1 && r1[10] <= 3 && r1[11] >= 1 && r1[11] <= 100 && r1[5] >= 1 && r1[5] <= 8) {
                percent = r1[11];  // 8K mouse on the USB cable (no bit 6); byte 5 = highest polling rate level
                charging = r1[10] == 1;
                wireless = false;
            }
            if (percent < 0) continue;
            DeviceStatus s;
            s.key = L"keychron:" + Hex4(pid);
            s.name = CleanName(HidProduct(*c));
            s.kind = DeviceKind::Mouse;
            s.via = wireless ? L"2.4 GHz receiver" : L"USB cable";
            s.percent = percent;
            s.charging = charging;
            ctx.Add(s, vid, pid);
            break;
        }
    }
}

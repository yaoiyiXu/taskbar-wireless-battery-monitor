// Corsair wireless headsets, mice and keyboards (VID 1B1C).
//  * Void / Void Pro / Void Elite / HS70 / HS80 family: output C9 64 -> [2] % (bit 7 = mic up), [4] status
//    (0 offline, 1-2 ok, 4-5 charging). From github.com/Sapd/HeadsetControl.
//  * "Bragi" devices (Virtuoso, Void V2, HS80 MAX, Slipstream mice/keyboards): wire bytes
//    02 <target> 02 <property> = GET; reply [1] source, [2] 02, [3] status (0 ok), [4..7] value LE.
//    Property 0F = battery (0..1000), 10 = charge state (1 charging, 3 full). Target 08 is the device
//    or receiver itself, 09+ are devices paired to a receiver. GET is read-only; we never switch the
//    device into software mode.
#include "drivers.h"

#include <algorithm>

namespace {

const USHORT kVoidPids[] = { 0x1B1C, 0x1B27, 0x0A14, 0x0A16, 0x0A17, 0x0A1D, 0x0A1A, 0x1B2A, 0x1B23,
                             0x1B29, 0x0A55, 0x0A51, 0x0A52, 0x0A38, 0x0A4F, 0x0A2B, 0x0A75, 0x0A56 };
const USHORT kBragiPids[] = { 0x2A08, 0x2A02, 0x0A97, 0x0A64, 0x0A62, 0x0A3E, 0x0A3D };

bool Contains(const USHORT* b, size_t n, USHORT v) { return std::find(b, b + n, v) != b + n; }

bool PollVoid(const HidColl& c, DeviceStatus& s) {
    HidDev d;
    if (!d.Open(c)) return false;
    d.Flush();
    std::vector<BYTE> r;
    if (!d.Write({ 0xC9, 0x64 })) return false;
    ULONGLONG end = GetTickCount64() + 1000;
    while (true) {
        int left = (int)((LONGLONG)end - (LONGLONG)GetTickCount64());
        if (left <= 0 || d.Read(r, left) < 5) return false;
        break;
    }
    BYTE st = r[4];
    s.key = L"corsair:" + Hex4(c.pid);
    s.name = HidProduct(c);
    if (s.name.empty()) s.name = L"Corsair wireless headset";
    s.kind = DeviceKind::Headset;
    s.via = L"2.4 GHz receiver";
    s.connected = st != 0;
    s.charging = st == 4 || st == 5;
    if (st == 1 || st == 2 || s.charging) s.percent = Clamp100(r[2] & 0x7F);
    return true;
}

class Bragi {
public:
    explicit Bragi(const HidColl& c) : numbered_(c.outLen() == 64) { ok_ = d_.Open(c); }
    bool ok() const { return ok_; }
    // Returns true and sets `value` on success; `status` holds the reply status (0 = ok, 5 = no such property).
    bool Get(BYTE target, BYTE prop, unsigned& value, int timeoutMs = 300) {
        std::vector<BYTE> req = numbered_ ? std::vector<BYTE>{ 0x02, target, 0x02, prop } : std::vector<BYTE>{ 0x00, 0x02, target, 0x02, prop };
        d_.Flush();
        if (!d_.Write(req)) return false;
        BYTE source = target == 0x08 ? 0x00 : 0x01;
        ULONGLONG end = GetTickCount64() + timeoutMs;
        std::vector<BYTE> r;
        for (;;) {
            int left = (int)((LONGLONG)end - (LONGLONG)GetTickCount64());
            if (left <= 0 || d_.Read(r, left) < 8) return false;
            if (r[2] != 0x02 || r[1] != source) continue;  // notification or another app's reply
            if (r[3] != 0x00) return false;
            value = r[4] | (r[5] << 8) | (r[6] << 16) | ((unsigned)r[7] << 24);
            return true;
        }
    }

private:
    HidDev d_;
    bool numbered_;
    bool ok_ = false;
};

}  // namespace

void PollCorsair(PollContext& ctx) {
    for (auto [vid, pid] : ctx.Products(0x1B1C)) {
        if (Contains(kVoidPids, std::size(kVoidPids), pid)) {
            for (const HidColl* c : ctx.Find(vid, pid, [](const HidColl& c) { return (c.page() == 0xFFC5 || c.page() == 0xFF00) && c.outLen() > 0; })) {
                DeviceStatus s;
                if (PollVoid(*c, s)) { ctx.Add(s, vid, pid); break; }
            }
            continue;
        }
        bool knownBragi = Contains(kBragiPids, std::size(kBragiPids), pid);
        auto colls = ctx.Find(vid, pid, [knownBragi](const HidColl& c) {
            bool sizes = (c.outLen() == 64 || c.outLen() == 65) && (c.inLen() == 64 || c.inLen() == 65);
            return sizes && (c.page() == 0xFF42 || (knownBragi && c.page() >= 0xFF00) || (knownBragi && c.iface == 4));
        });
        for (const HidColl* c : colls) {
            Bragi b(*c);
            if (!b.ok()) continue;
            bool any = false;
            std::wstring product = HidProduct(*c);
            // Paired devices behind a receiver first, then the device itself (wired / Bluetooth).
            for (BYTE target : { 0x09, 0x0A, 0x0B, 0x08 }) {
                if (target == 0x08 && any) break;
                unsigned level = 0;
                bool ok = b.Get(target, 0x0F, level);
                for (int retry = 0; ok && (level == 0 || level > 1000) && retry < 2; retry++) ok = b.Get(target, 0x0F, level);
                if (!ok || level > 1000) continue;
                DeviceStatus s;
                s.key = L"corsair:" + Hex4(pid) + L":" + std::to_wstring(target);
                s.name = product.empty() ? L"Corsair wireless device" : product;
                s.kind = GuessKind(s.name);
                if (s.kind == DeviceKind::Other) s.kind = KindFromCollections(ctx.colls, vid, pid);
                s.via = target == 0x08 ? L"USB / Bluetooth" : L"Corsair receiver";
                s.percent = Clamp100((int)(level + 5) / 10);
                unsigned charge = 0;
                if (b.Get(target, 0x10, charge)) s.charging = charge == 1 || charge == 3;
                ctx.out.push_back(s);
                any = true;
            }
            if (any) { ctx.Claim(vid, pid); break; }
        }
    }
}

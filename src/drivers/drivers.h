#pragma once
// Each driver looks at the enumerated HID collections, talks to the devices it recognises and
// appends what it finds. A driver claims a device (vendor/product id) once it has handled it so
// that the generic fallbacks further down the chain do not report it twice.
#include <set>
#include <utility>
#include <vector>

#include "../devices.h"
#include "../hid.h"

struct PollContext {
    std::vector<HidColl> colls;
    std::set<std::pair<USHORT, USHORT>> claimed;
    std::vector<DeviceStatus> out;

    bool IsClaimed(USHORT vid, USHORT pid) const { return claimed.count({ vid, pid }) != 0; }
    void Claim(USHORT vid, USHORT pid) { claimed.insert({ vid, pid }); }
    void Add(const DeviceStatus& s, USHORT vid, USHORT pid) {
        out.push_back(s);
        Claim(vid, pid);
    }
    // Distinct (vid, pid) pairs for a vendor that are not claimed yet.
    std::vector<std::pair<USHORT, USHORT>> Products(USHORT vid) const {
        std::set<std::pair<USHORT, USHORT>> s;
        for (const auto& c : colls)
            if (c.vid == vid && !IsClaimed(c.vid, c.pid)) s.insert({ c.vid, c.pid });
        return { s.begin(), s.end() };
    }
    // Collections of one product, optionally filtered.
    template <class Pred>
    std::vector<const HidColl*> Find(USHORT vid, USHORT pid, Pred pred) const {
        std::vector<const HidColl*> r;
        for (const auto& c : colls)
            if (c.vid == vid && c.pid == pid && pred(c)) r.push_back(&c);
        return r;
    }
};

// G-Wolves model names (shared by the Glorious- and Kysona-protocol drivers).
const wchar_t* GWolvesModelName(USHORT pid);
bool GWolvesIsCable(USHORT pid);

// Vendor drivers (run in this order).
void PollGlorious(PollContext& ctx);  // also Hitscan
void PollAula(PollContext& ctx);      // AULA F75 / F87 Pro / F99 wireless keyboards (before Kysona, same vendor id)
void PollMchose(PollContext& ctx);    // MCHOSE G87 41E4:2001, 2.4 GHz only
void PollAtkSerial(PollContext& ctx); // ATK N9 Pro headset (virtual COM port)
void PollKeychron(PollContext& ctx);  // Keychron mice
void PollLamzu(PollContext& ctx);     // LAMZU mice on vendor ids 373E / 37B0
void PollRongyuan(PollContext& ctx);  // Rongyuan receivers (Attack Shark, Ajazz keyboards, ...)
void PollSonyInzone(PollContext& ctx);  // Sony INZONE H9 II / H5
void PollRog(PollContext& ctx);       // ASUS ROG keyboards and mice
void PollZowie(PollContext& ctx);     // BenQ ZOWIE EC-CW / DW mice
void PollRapoo(PollContext& ctx);     // Rapoo VT gaming mice (passive battery reports)
void PollMiscMice(PollContext& ctx);  // Motospeed X6, Delux M900 Pro, Ajazz AJ179 and AJ159/AJ-series
void PollRazerHeadsets(PollContext& ctx);  // Razer BlackShark V3 / V3 Pro
void PollKysona(PollContext& ctx);    // also Pulsar, ATK / VXE, Hitscan Hyperlight
void PollHyperX(PollContext& ctx);
void PollLogitech(PollContext& ctx);
void PollRazer(PollContext& ctx);
void PollSteelSeries(PollContext& ctx);
void PollCorsair(PollContext& ctx);
void PollCoolerMaster(PollContext& ctx);
void PollPlayStation(PollContext& ctx);
void PollGameSir(PollContext& ctx);    // Nova Lite / Nova 2 / Nova 2 Lite
// Fallbacks.
void PollHeadsetControl(PollContext& ctx);  // only if headsetcontrol.exe is installed
void PollGenericHid(PollContext& ctx);
void PollBluetooth(PollContext& ctx);
void PollXInput(PollContext& ctx);

#include "../src/drivers/drivers.h"
#include <cassert>
#include <cstdio>
#include <map>
static ULONGLONG clockMs = 1000;
static int writes = 0, features = 0;
static bool headsetReplies = false;
ULONGLONG TestClock() { return clockMs; }
void TestSleep(DWORD ms) { clockMs += ms; }
bool HidDev::Open(const HidColl& c) { c_ = c; return true; }
void HidDev::Close() {}
void HidDev::Flush() {}
bool HidDev::Write(const std::vector<BYTE>&, int) { ++writes; return true; }
bool HidDev::GetFeature(BYTE, std::vector<BYTE>&) { ++features; return false; }
int HidDev::Read(std::vector<BYTE>& out, int) {
    clockMs += 1000;
    if (c_.vid == 0x054C) {
        out.assign(78, 0); out[0] = 0x31; out[54] = 5;
        return 78;
    }
    if (!headsetReplies) return -1;
    out.assign(64, 0);
    out[0] = 2; out[2] = 0x60; out[10] = 0x21; out[13] = 80;
    return 64;
}
std::wstring Hex4(USHORT) { return L"test"; }
int Clamp100(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
#define GetTickCount64 TestClock
#define Sleep TestSleep
namespace sony {
#include "../src/drivers/playstation.cpp"
}
namespace blackshark {
#include "../src/drivers/razer_headset.cpp"
}
#undef GetTickCount64
#undef Sleep
int main() {
    PollContext ctx;
    HidColl ps;
    ps.vid = 0x054C; ps.pid = 0x0CE6;
    ps.caps.UsagePage = 1; ps.caps.Usage = 5; ps.caps.InputReportByteLength = 78;
    ctx.colls = {ps};
    sony::PollPlayStation(ctx);
    assert(features == 0 && writes == 0);
    assert(ctx.out.size() == 1 && ctx.out[0].percent == 55);
    HidColl headset;
    headset.vid = 0x1532; headset.pid = 0x0576; headset.iface = 5;
    headset.caps.OutputReportByteLength = 64; headset.caps.InputReportByteLength = 64;
    ctx = {}; ctx.colls = {headset};
    blackshark::PollRazerHeadsets(ctx);
    assert(writes == 4 && ctx.out.empty() && ctx.IsClaimed(0x1532, 0x0576));
    ctx = {}; ctx.colls = {headset};
    blackshark::PollRazerHeadsets(ctx);
    assert(writes == 4 && ctx.out.empty() && ctx.IsClaimed(0x1532, 0x0576));
    clockMs += 300000;
    headsetReplies = true;
    ctx = {}; ctx.colls = {headset};
    blackshark::PollRazerHeadsets(ctx);
    assert(writes == 8 && ctx.out.size() == 1 && ctx.out[0].percent == 80);
    ctx = {}; ctx.colls = {headset};
    blackshark::PollRazerHeadsets(ctx);
    assert(writes == 8 && ctx.out.size() == 1 && ctx.out[0].percent == 80);
    puts("PlayStation passive read and BlackShark failed/successful-attempt rate-limit checks passed.");
}

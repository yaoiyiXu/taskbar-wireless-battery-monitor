// Exercise the actual driver with a simulated HID transport; no USB writes or hardware needed.
#include "../src/drivers/drivers.h"

#include <cassert>
#include <cstdio>
#include <map>

namespace {
std::map<std::wstring, std::vector<std::vector<BYTE>>> reports;
int opens = 0;

HidColl Collection(USHORT pid, const wchar_t* path, USHORT length = 19) {
    HidColl c;
    c.path = path;
    c.vid = 0x24AE;
    c.pid = pid;
    c.caps.UsagePage = 0xFF00;
    c.caps.Usage = 2;
    c.caps.InputReportByteLength = length;
    return c;
}

std::vector<BYTE> Status(bool cable, BYTE percent = 25) {
    return { 7, (BYTE)(cable ? 0x22 : 0x20), 3, 0x20, 3, 0x20, 3,
             (BYTE)(cable ? 2 : 1), percent, 0, 4, 0x84, 0, 0x17, 0x46, 0, 2, 3, 0xBC };
}
}  // namespace

// Stand-ins for the Win32 HID transport. The production driver is linked unchanged.
bool HidDev::Open(const HidColl& c) {
    c_ = c;
    h_ = reinterpret_cast<HANDLE>(1);
    ++opens;
    return true;
}
void HidDev::Close() { h_ = INVALID_HANDLE_VALUE; }
int HidDev::Read(std::vector<BYTE>& out, int timeoutMs) {
    assert(timeoutMs > 0 && timeoutMs <= 4000);
    auto& queue = reports[c_.path];
    if (queue.empty()) return -1;
    out = queue.front();
    queue.erase(queue.begin());
    return (int)out.size();
}
std::wstring HidProduct(const HidColl&) { return L"Rapoo Gaming Device"; }
std::wstring Hex4(USHORT v) {
    wchar_t text[8];
    swprintf(text, 8, L"%04X", v);
    return text;
}

int main() {
    auto receiver = Collection(0x1417, L"receiver");
    auto cable = Collection(0x4617, L"cable");
    PollContext ctx;
    ctx.colls = { receiver, cable };
    // Ignore an unrelated packet and accept the next status; cable wins over receiver.
    reports = { { L"receiver", { { 0xBC, 0 }, Status(false) } }, { L"cable", { Status(true, 27) } } };
    PollRapoo(ctx);
    assert(ctx.out.size() == 1 && ctx.out[0].key == L"rapoo:4617");
    assert(ctx.out[0].percent == 27 && ctx.out[0].charging);
    assert(ctx.out[0].via == L"USB cable" && ctx.out[0].kind == DeviceKind::Mouse);
    assert(ctx.out[0].graceSec == 1800);
    assert(ctx.IsClaimed(0x24AE, 0x1417) && ctx.IsClaimed(0x24AE, 0x4617));
    auto stableKey = ctx.out[0].key;

    ctx = {};
    ctx.colls = { receiver };
    reports = { { L"receiver", { Status(false) } } };
    PollRapoo(ctx);
    assert(ctx.out.size() == 1 && ctx.out[0].key == stableKey);
    assert(!ctx.out[0].charging && ctx.out[0].percent == 25);
    assert(ctx.out[0].via == L"2.4 GHz receiver");

    ctx = {};
    ctx.colls = { receiver };
    reports.clear();
    PollRapoo(ctx); // timeout must not claim an unknown/sleeping device or invent a level
    assert(ctx.out.empty() && ctx.claimed.empty());

    ctx = {};
    ctx.colls = { Collection(0x1215, L"legacy", 7), Collection(0x4415, L"legacy-cable", 7) };
    reports = { { L"legacy", { { 0xBB, 0xB0, 0x81, 0x20, 3, 1, 37 } } },
                { L"legacy-cable", { { 0xBB, 0xB0, 0x81, 0x20, 3, 2, 0x7F } } } };
    PollRapoo(ctx);
    assert(ctx.out.size() == 1 && ctx.out[0].key == L"rapoo:4415");
    assert(ctx.out[0].charging && ctx.out[0].percent == 37);
    assert(ctx.out[0].via == L"USB cable");

    // A cable-only unknown level stays unknown; a later receiver reading can fill it without
    // clearing the cable's charging state.
    ctx = {};
    ctx.colls = { Collection(0x4415, L"legacy-cable", 7) };
    reports = { { L"legacy-cable", { { 0xBB, 0xB0, 0x81, 0x20, 3, 2, 0x7F } } } };
    PollRapoo(ctx);
    assert(ctx.out.size() == 1 && ctx.out[0].charging && ctx.out[0].percent == -1);
    ctx.colls = { Collection(0x1215, L"legacy", 7) };
    reports = { { L"legacy", { { 0xBB, 0xB0, 0x81, 0x20, 3, 1, 37 } } } };
    PollRapoo(ctx);
    assert(ctx.out.size() == 1 && ctx.out[0].charging && ctx.out[0].percent == 37);
    assert(ctx.out[0].via == L"USB cable");

    // The same marker does not select legacy semantics on a modern Nordic collection.
    ctx = {};
    ctx.colls = { receiver };
    reports = { { L"receiver", { { 0xBB, 0xB0, 0x81, 0x20, 3, 2, 50 } } } };
    PollRapoo(ctx);
    assert(ctx.out.empty() && ctx.claimed.empty());

    // Same collection/protocol on an unlisted PID should work, without an invented model name.
    ctx = {};
    ctx.colls = { Collection(0x7ABC, L"new-model") };
    auto unknownModel = Status(false, 62);
    unknownModel[13] = 0;
    unknownModel[14] = 0;
    reports = { { L"new-model", { unknownModel } } };
    PollRapoo(ctx);
    assert(ctx.out.size() == 1 && ctx.out[0].name == L"Rapoo wireless mouse");
    assert(ctx.out[0].key == L"rapoo:7ABC" && ctx.out[0].percent == 62);

    opens = 0;
    ctx = {};
    auto otherVendor = receiver;
    otherVendor.vid = 0x1234;
    auto config = receiver;
    config.caps.Usage = 14;
    auto wrongPage = receiver;
    wrongPage.caps.UsagePage = 1;
    ctx.colls = { otherVendor, config, wrongPage, Collection(0x1215, L"silent-sibling", 10),
                  Collection(0x1C00, L"unrecognised-telink", 7) };
    PollRapoo(ctx);
    assert(opens == 0 && ctx.out.empty());
    ctx.colls = { receiver };
    ctx.Claim(receiver.vid, receiver.pid);
    PollRapoo(ctx);
    assert(opens == 0);
    std::puts("Rapoo driver tests passed (discovery, identity, deduplication, timeout, legacy, filtering).");
}

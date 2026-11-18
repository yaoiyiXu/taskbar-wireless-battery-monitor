// Exercise the generated mod without starting a UI, BLE scan or device queries.
#include <windows.h>
#include <windhawk_api.h>
#include <cassert>
#include <iostream>
#include <set>
#include <string>

LANGID testLanguage = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
LANGID TestUILanguage() { return testLanguage; }
std::wstring testIniPath;
int testDeviceOpens = 0;
HANDLE TestDeviceOpen(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE) {
    ++testDeviceOpens;
    return INVALID_HANDLE_VALUE; // No hardware access, including when testing an allowed PID.
}

BOOL TestSetStringValue(PCWSTR name, PCWSTR value) {
    return WritePrivateProfileStringW(L"Mod", name, value, testIniPath.c_str());
}
size_t TestGetStringValue(PCWSTR name, PWSTR buffer, size_t count) {
    return GetPrivateProfileStringW(L"Mod", name, L"", buffer, (DWORD)count, testIniPath.c_str());
}

#define GetUserDefaultUILanguage TestUILanguage
#define CreateFileW TestDeviceOpen
#define Wh_SetStringValue TestSetStringValue
#define Wh_GetStringValue TestGetStringValue
#include "../windhawk/taskbar-wireless-battery-monitor.wh.cpp"
#undef GetUserDefaultUILanguage
#undef CreateFileW
#undef Wh_SetStringValue
#undef Wh_GetStringValue

const std::set<std::wstring> expectedKeys = {
    L"airpods-nearby:1420", L"bt:{12345678-1234-1234-1234-123456789ABC}",
    L"hid:046D:C52B", L"ps:0CE6:123456"
};

void TestLanguages() {
    wb_app::TrayEntry entry;
    entry.st.name = L"Test headset";
    entry.lastPct = 72;
    entry.st.charging = true;
    entry.st.asleep = true;
    for (LANGID language : {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                            MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN)}) {
        testLanguage = language;
        assert(zh::Text(L"Low") == L"Low");
        assert(zh::Text(L"Xbox controller 1") == L"Xbox controller 1");
        assert(wb_app::StatusText(entry) == L"72% (last known), charging, asleep");
        assert(wb_driver_airpods::Level(-1, false) == L"Unknown");
        assert(wb_driver_airpods::Level(90, true) == L"~90% (charging)");
        entry.st.connected = false;
        assert(wb_app::StatusText(entry) == L"Off / disconnected");
        entry.st.connected = true;
    }
    for (LANGID language : {MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)}) {
        testLanguage = language;
        assert(zh::Text(L"Low") == L"偏低");
        assert(zh::Text(L"Xbox controller 1") == L"Xbox 手柄 1");
        assert(wb_app::StatusText(entry) == L"72%（上次记录），充电中，休眠中");
        assert(wb_driver_airpods::Level(-1, false) == L"未知");
        assert(wb_driver_airpods::Level(90, true) == L"约90%（充电中）");
    }
    assert(zh::Text(L"hid:046D:C52B") == L"hid:046D:C52B");
    testLanguage = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
}

void TestStoppedPollAndMutex() {
    assert(!ModConfig::airpodsEnabled);
    HANDLE stop = CreateEventW(nullptr, TRUE, TRUE, nullptr);
    assert(stop);
    assert(PollAllDevices(stop).empty());
    assert(!wb_driver_airpods::GetReceiver().watcher);
    CloseHandle(stop);

    HANDLE otherInstance = CreateMutexW(nullptr, FALSE, L"Local\\TaskbarWirelessBatteryTray");
    assert(otherInstance);
    for (int i = 0; i < 10; ++i) assert(wb_app::Run(nullptr) == 0);
    DWORD before = 0, after = 0;
    assert(GetProcessHandleCount(GetCurrentProcess(), &before));
    for (int i = 0; i < 200; ++i) assert(wb_app::Run(nullptr) == 0);
    assert(GetProcessHandleCount(GetCurrentProcess(), &after));
    // Windows/runtime may lazily create a small number of process-global handles.
    // A leaked mutex on each call would add 200 handles.
    assert(after <= before + 2);
    CloseHandle(otherInstance);
}

int wmain(int argc, wchar_t** argv) {
    assert(argc == 3);
    testIniPath = argv[2];
    TestLanguages();
    TestStoppedPollAndMutex();
    std::wstring mode = argv[1];
    if (mode == L"--shutdown") {
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD { Sleep(8000); ExitProcess(99); }, nullptr, 0, nullptr);
        ModHost::uiThread = CreateThread(nullptr, 0, [](LPVOID) -> DWORD { Sleep(INFINITE); return 0; }, nullptr, 0, nullptr);
        WhTool_ModUninit(); // Must terminate this disposable process in five seconds.
        return 98;
    }
    ModSafety::ConfigureIgnored(L"28bd:0947, 046D:C52B");
    assert(ModSafety::IsIgnored(0x28BD, 0x0947));
    assert(ModSafety::IsIgnored(0x046D, 0xC52B));
    assert(!ModSafety::IsIgnored(0x046D, 0xC547));
    HidColl pen, vendor, mouse, ignored;
    pen.vid = vendor.vid = 0x28BD;
    pen.pid = vendor.pid = 0x9999;
    pen.caps.UsagePage = 0x0D;
    vendor.caps.UsagePage = 0xFF00;
    mouse.vid = 0x046D; mouse.pid = 0xC547;
    ignored.vid = 0x046D; ignored.pid = 0xC52B;
    std::vector<HidColl> candidates{pen, vendor, mouse, ignored};
    ModSafety::Filter(candidates);
    assert(candidates.size() == 1 && candidates[0].pid == 0xC547);
    assert(ModSafety::KysonaKnown(0x3554, 0xF55D));
    assert(!ModSafety::KysonaKnown(0x3554, 0xFFFF));
    ModSafety::ConfigureIgnored(L"046D:C52Z");
    assert(ModSafety::IsIgnored(0x046D, 0xC547)); // malformed exclusion fails closed
    ModSafety::ConfigureIgnored(L"");
    assert(!ModSafety::IsIgnored(0x046D, 0xC547));
    for (USHORT vid : {(USHORT)0x258A, (USHORT)0x3770, (USHORT)0x33E4}) {
        PollContext ctx;
        HidColl unknown;
        unknown.vid = vid; unknown.pid = 0xFFFF; unknown.caps.FeatureReportByteLength = 65;
        ctx.colls = {unknown};
        int before = testDeviceOpens;
        PollGlorious(ctx);
        assert(testDeviceOpens == before);
    }
    for (USHORT vid : {(USHORT)0x3554, (USHORT)0x373B, (USHORT)0x3770, (USHORT)0x33E4, (USHORT)0x3710}) {
        PollContext ctx;
        HidColl unknown;
        unknown.vid = vid; unknown.pid = 0xFFFF;
        unknown.caps.InputReportByteLength = unknown.caps.OutputReportByteLength = 17;
        ctx.colls = {unknown};
        int before = testDeviceOpens;
        PollKysona(ctx);
        assert(testDeviceOpens == before);
    }
    PollContext tablet;
    pen.caps.NumberFeatureValueCaps = vendor.caps.NumberFeatureValueCaps = 1;
    tablet.colls = {pen, vendor};
    int beforeTablet = testDeviceOpens;
    PollGenericHid(tablet);
    assert(testDeviceOpens == beforeTablet);
    HidColl allowed;
    allowed.vid = 0x258A; allowed.pid = 0x2022; allowed.caps.FeatureReportByteLength = 65;
    PollContext known; known.colls = {allowed};
    int beforeKnown = testDeviceOpens;
    PollGlorious(known);
    assert(testDeviceOpens > beforeKnown); // prove that the open spy actually observes allowed devices
    HANDLE upstream = CreateMutexW(nullptr, FALSE, L"Local\\WirelessBatteryTray");
    assert(upstream && !WhTool_ModInit());
    CloseHandle(upstream);
    if (mode == L"--save") {
        wb_app::g_hidden = expectedKeys;
        wb_app::SaveHidden();
        wchar_t raw[1024]{};
        TestGetStringValue(L"HiddenDevices", raw, ARRAYSIZE(raw));
        std::wstring stored(raw);
        assert(stored.find(L'|') != std::wstring::npos);
        assert(stored.find_first_of(L"\r\n") == std::wstring::npos);
    } else if (mode == L"--load") {
        // A fresh process reads the actual INI written by --save.
        wb_app::LoadHidden();
        assert(wb_app::g_hidden == expectedKeys);
        wb_app::g_hidden.clear();
        wb_app::SaveHidden();
        wb_app::g_hidden.insert(L"stale:test");
        wb_app::LoadHidden();
        assert(wb_app::g_hidden.empty());
    } else if (mode == L"--legacy") {
        std::wstring flattened;
        for (const auto& key : expectedKeys) flattened += key + L" ";
        assert(TestSetStringValue(L"HiddenDevices", flattened.c_str()));
        wb_app::LoadHidden();
        assert(wb_app::g_hidden == expectedKeys);
        wb_app::SaveHidden();
    } else {
        assert(false);
    }
    std::cout << "Windhawk localization, INI persistence and stop/instance regression checks passed.\n";
}

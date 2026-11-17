// Compile with Windhawk's own compiler + WH_EDITING to exercise the generated ABI/native engine.
#include "../windhawk/taskbar-wireless-battery-monitor.wh.cpp"
#include <iostream>

int wmain() {
    auto text = DiagnoseAirPods(12);
    int n = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), utf8.data(), n, nullptr, nullptr);
    std::cout << utf8;
    StartAirPods();
    auto devices = PollAllDevices();
    std::cout << "Native engine devices: " << devices.size() << "\n";
    StopAirPods();
    return text.find(L"BLE status=1, HRESULT=0x00000000") != std::wstring::npos ? 0 : 1;
}

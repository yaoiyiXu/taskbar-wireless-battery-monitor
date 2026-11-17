#pragma once
#include <string>

// Translate at presentation boundaries; preserve protocol values and device keys.
namespace zh {
inline std::wstring Text(const std::wstring& value) {
    struct Entry { const wchar_t* source; const wchar_t* translated; };
    static const Entry entries[] = {
        { L"Empty", L"耗尽" },
        { L"Low", L"偏低" },
        { L"Medium", L"中等" },
        { L"Full", L"充足" },
        { L"Good", L"良好" },
        { L"Critical", L"极低" },
        { L"Bluetooth", L"蓝牙" },
        { L"USB cable", L"USB 有线" },
        { L"2.4 GHz receiver", L"2.4 GHz 接收器" },
        { L"USB / Bluetooth", L"USB / 蓝牙" },
        { L"Logitech receiver", L"Logitech 接收器" },
        { L"Razer receiver", L"Razer 接收器" },
        { L"Corsair receiver", L"Corsair 接收器" },
        { L"Wireless adapter", L"无线适配器" },
        { L"Xbox Wireless", L"Xbox 无线" },
        { L"Xbox Wireless / Bluetooth", L"Xbox 无线 / 蓝牙" },
        { L"ROG Omni Receiver", L"ROG Omni 接收器" },
        { L"2.4 GHz receiver / USB", L"2.4 GHz 接收器 / USB" },
        { L"UWB base station", L"UWB 基站" },
        { L"Wireless device", L"无线设备" },
        { L"Bluetooth device", L"蓝牙设备" },
        { L"Wireless mouse", L"无线鼠标" },
        { L"Wireless keyboard", L"无线键盘" },
        { L"Headset", L"耳机" },
        { L"Xbox controller", L"Xbox 手柄" },
        { L"Xbox headset", L"Xbox 耳机" },
        { L"AULA wireless keyboard", L"AULA 无线键盘" },
        { L"Corsair wireless headset", L"Corsair 无线耳机" },
        { L"Corsair wireless device", L"Corsair 无线设备" },
        { L"LAMZU wireless mouse", L"LAMZU 无线鼠标" },
        { L"Logitech wireless device", L"Logitech 无线设备" },
        { L"Rapoo wireless mouse", L"Rapoo 无线鼠标" },
        { L"Redragon wireless mouse", L"Redragon 无线鼠标" },
        { L"Keychron mouse", L"Keychron 鼠标" },
        { L"Ajazz mouse", L"Ajazz 鼠标" },
        { L"AJAZZ mouse", L"AJAZZ 鼠标" },
        { L"ZOWIE DW mouse", L"ZOWIE DW 鼠标" },
    };
    for (const auto& entry : entries)
        if (value == entry.source) return entry.translated;
    const std::wstring prefix = L"Xbox controller ";
    if (value.rfind(prefix, 0) == 0) return L"Xbox 手柄 " + value.substr(prefix.size());
    return value;
}
} // namespace zh

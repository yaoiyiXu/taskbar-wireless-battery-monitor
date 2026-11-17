#pragma once
// Independent decoder of Apple proximity advertisements. Layout reference:
// SpriteOvO/AirPodsDesktop, Source/Core/AppleCP.h and AppleCP.cpp (GPL-3.0-or-later).
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace airpods {
struct Battery {
    int left = -1, right = -1, box = -1;
    bool leftCharging = false, rightCharging = false, boxCharging = false;
    uint16_t model = 0;
};

inline const wchar_t* ModelName(uint16_t model) {
    switch (model) {
        case 0x0220: return L"AirPods 1";
        case 0x0F20: return L"AirPods 2";
        case 0x1320: return L"AirPods 3";
        case 0x1920: return L"AirPods 4";
        case 0x1B20: return L"AirPods 4 ANC";
        case 0x0E20: return L"AirPods Pro";
        case 0x1420: return L"AirPods Pro 2";
        case 0x2420: return L"AirPods Pro 2 USB-C";
        case 0x2720: return L"AirPods Pro 3";
        case 0x0A20: return L"AirPods Max";
        case 0x1F20: return L"AirPods Max USB-C";
        default: return nullptr;
    }
}

inline int Percent(unsigned nibble) { return nibble <= 10 ? int(nibble * 10) : -1; }

inline bool Decode(const uint8_t* data, size_t size, Battery& result) {
    if (!data || size != 27 || data[0] != 0x07 || data[1] != 0x19 || data[2] != 0x01) return false;
    Battery next;
    next.model = (uint16_t(data[3]) << 8) | data[4];
    if (!ModelName(next.model)) return false;
    // Low nibble and bit 4 describe the broadcasting bud; status bit 5 identifies its side.
    const bool fromLeft = (data[5] & 0x20) != 0;
    const int current = Percent(data[6] & 0x0F), other = Percent(data[6] >> 4);
    next.left = fromLeft ? current : other;
    next.right = fromLeft ? other : current;
    next.box = Percent(data[7] & 0x0F);
    next.leftCharging = (data[7] & (fromLeft ? 0x10 : 0x20)) != 0;
    next.rightCharging = (data[7] & (fromLeft ? 0x20 : 0x10)) != 0;
    next.boxCharging = (data[7] & 0x40) != 0;
    if (next.left < 0 && next.right < 0 && next.box < 0) return false;
    result = next;
    return true;
}

inline int EarPercent(const Battery& b) {
    if (b.left < 0) return b.right;
    if (b.right < 0) return b.left;
    return std::min(b.left, b.right);
}
} // namespace airpods

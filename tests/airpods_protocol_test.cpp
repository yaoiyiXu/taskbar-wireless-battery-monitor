#include "../src/drivers/airpods_protocol.h"
#include <array>
#include <cassert>
#include <cstdio>

int main() {
    // Synthetic protocol fixture, not a claim of a captured user's device.
    // Pro 2, left transmitter: current=60%, other=80%, case=50%, current+case charging.
    std::array<uint8_t, 27> packet{0x07, 0x19, 0x01, 0x14, 0x20, 0x20, 0x86, 0x55};
    airpods::Battery b;
    assert(airpods::Decode(packet.data(), packet.size(), b));
    assert(b.model == 0x1420 && b.left == 60 && b.right == 80 && b.box == 50);
    assert(b.leftCharging && !b.rightCharging && b.boxCharging);
    assert(airpods::EarPercent(b) == 60);

    packet[5] = 0; // the same low nibble now describes the right transmitter
    assert(airpods::Decode(packet.data(), packet.size(), b));
    assert(b.left == 80 && b.right == 60 && !b.leftCharging && b.rightCharging);
    packet[5] = 0x60; // unrelated status bit must not reverse the transmitter's side
    assert(airpods::Decode(packet.data(), packet.size(), b));
    assert(b.left == 60 && b.right == 80);

    packet[6] = 0xFA; packet[7] = 0x0F;
    assert(airpods::Decode(packet.data(), packet.size(), b));
    assert(b.left == 100 && b.right == -1 && b.box == -1);
    assert(airpods::EarPercent(b) == 100);
    packet[6] = 0x00; packet[7] = 0x00;
    assert(airpods::Decode(packet.data(), packet.size(), b));
    assert(b.left == 0 && b.right == 0 && b.box == 0); // zero is valid, not unknown
    packet[6] = 0xBC; packet[7] = 0x0A;
    assert(airpods::Decode(packet.data(), packet.size(), b));
    assert(b.left == -1 && b.right == -1 && b.box == 100); // case-only observation
    packet[7] = 0x0F;
    assert(!airpods::Decode(packet.data(), packet.size(), b)); // all unavailable
    packet[6] = 0xA9;

    for (size_t length = 0; length < packet.size(); ++length)
        assert(!airpods::Decode(packet.data(), length, b));
    assert(!airpods::Decode(nullptr, packet.size(), b));
    assert(!airpods::Decode(packet.data(), 28, b));
    for (size_t field : {size_t(0), size_t(1), size_t(2)}) {
        auto wrong = packet; wrong[field] = 0;
        assert(!airpods::Decode(wrong.data(), wrong.size(), b));
    }
    packet[3] = 0xFF;
    assert(!airpods::Decode(packet.data(), packet.size(), b));
    packet[3] = 0x24;
    assert(airpods::Decode(packet.data(), packet.size(), b));
    assert(b.model == 0x2420); // USB-C Pro 2 has its own model identifier
    puts("AirPods tests passed: Pro 2 variants, left/right mapping, charging, coarse levels, unknowns, malformed frames.");
}

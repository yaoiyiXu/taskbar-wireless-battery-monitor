// Runs every driver in order. Vendor drivers go first and claim the devices they handle; the
// generic fallbacks then only look at what is left.
#include "devices.h"

#include <cstdio>

#include "drivers/drivers.h"
#include "hid.h"
#include "drivers/airpods.h"

std::vector<DeviceStatus> PollAllDevices() {
    StartAirPods();
    PollContext ctx;
    ctx.colls = EnumHid();
    PollGlorious(ctx);
    PollAula(ctx);       // before Kysona, which shares the 3554 vendor id and would probe it first
    PollMchose(ctx);
    PollAtkSerial(ctx);  // before Kysona, which shares the ATK vendor id
    PollKysona(ctx);
    PollKeychron(ctx);
    PollLamzu(ctx);
    PollRongyuan(ctx);
    PollSonyInzone(ctx);
    PollRog(ctx);
    PollZowie(ctx);
    PollRapoo(ctx);
    PollMiscMice(ctx);
    PollRazerHeadsets(ctx);  // before the generic Razer driver, which shares the vendor id
    PollHyperX(ctx);
    PollLogitech(ctx);
    PollRazer(ctx);
    PollSteelSeries(ctx);
    PollCorsair(ctx);
    PollCoolerMaster(ctx);
    PollPlayStation(ctx);
    PollGameSir(ctx);
    PollHeadsetControl(ctx);
    PollGenericHid(ctx);
    PollBluetooth(ctx);
    PollAirPods(ctx);
    PollXInput(ctx);
    return ctx.out;
}

std::wstring DumpHidCollections() {
    std::wstring out = L"VID:PID    接口  用途页:用途  输入 输出 特征  产品\n";
    for (const auto& c : EnumHid()) {
        wchar_t line[160];
        swprintf(line, 160, L"%04X:%04X  %2d  %04X:%04X    %3u %3u %3u   ", c.vid, c.pid, c.iface, c.page(), c.usage(), c.inLen(), c.outLen(),
                 c.featLen());
        out += line + HidProduct(c) + L"\n";
    }
    return out;
}

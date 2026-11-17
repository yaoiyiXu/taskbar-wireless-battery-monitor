// Apple proximity battery receiver, independently implemented against Windows WinRT.
// Protocol reference: SpriteOvO/AirPodsDesktop, Source/Core/AppleCP.{h,cpp} (GPL-3.0-or-later).
#include "drivers.h"
#include "airpods.h"
#include "airpods_protocol.h"
#include <roapi.h>
#include <winstring.h>
#ifdef __MINGW32__
#define boolean bool
#endif
#include <windows.devices.bluetooth.advertisement.h>
#include <robuffer.h>
#ifdef __MINGW32__
#undef boolean
#endif
#include <atomic>
#include <map>
#include <memory>
#include <mutex>

namespace {
using namespace ABI::Windows::Devices::Bluetooth::Advertisement;
using Received = ABI::Windows::Foundation::ITypedEventHandler<BluetoothLEAdvertisementWatcher*, BluetoothLEAdvertisementReceivedEventArgs*>;
using DataList = ABI::Windows::Foundation::Collections::IVectorView<BluetoothLEManufacturerData*>;

template<class T> struct Com {
    T* p = nullptr;
    ~Com() { if (p) p->Release(); }
    T** out() { return &p; }
    T* operator->() const { return p; }
};

struct Seen { airpods::Battery battery; ULONGLONG when = 0; INT16 rssi = -127; };
struct State {
    std::mutex mutex;
    std::map<UINT64, Seen> seen;
    HANDLE refresh = nullptr;
    bool pending = false;
    ULONGLONG signalled = 0;
    unsigned events = 0, apple = 0, decoded = 0;
};

class Handler final : public Received {
    std::atomic<ULONG> refs{1};
    std::shared_ptr<State> state;
public:
    explicit Handler(std::shared_ptr<State> s) : state(std::move(s)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == __uuidof(IUnknown) || iid == __uuidof(Received) || iid == __uuidof(IAgileObject)) {
            *out = static_cast<Received*>(this); AddRef(); return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { ULONG n = --refs; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE Invoke(IBluetoothLEAdvertisementWatcher*, IBluetoothLEAdvertisementReceivedEventArgs* args) override {
        Com<IBluetoothLEAdvertisement> ad;
        Com<DataList> list;
        if (FAILED(args->get_Advertisement(ad.out())) || !ad.p ||
            FAILED(ad->GetManufacturerDataByCompanyId(0x004C, list.out())) || !list.p) return S_OK;
        UINT32 count = 0; list->get_Size(&count);
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            ++state->events; if (count) ++state->apple;
        }
        for (UINT32 i = 0; i < count; ++i) {
            Com<IBluetoothLEManufacturerData> item;
            Com<ABI::Windows::Storage::Streams::IBuffer> buffer;
            Com<::Windows::Storage::Streams::IBufferByteAccess> access;
            if (FAILED(list->GetAt(i, item.out())) || !item.p || FAILED(item->get_Data(buffer.out())) || !buffer.p) continue;
            UINT32 length = 0;
            if (FAILED(buffer->get_Length(&length)) || length != 27 ||
                FAILED(buffer->QueryInterface(__uuidof(::Windows::Storage::Streams::IBufferByteAccess), (void**)access.out()))) continue;
            BYTE* bytes = nullptr;
            if (FAILED(access->Buffer(&bytes))) continue;
            airpods::Battery battery;
            if (!airpods::Decode(bytes, length, battery)) continue;
            UINT64 address = 0; INT16 rssi = -127;
            if (FAILED(args->get_BluetoothAddress(&address)) || FAILED(args->get_RawSignalStrengthInDBm(&rssi)) || rssi < -85) continue;
            const auto now = GetTickCount64();
            std::lock_guard<std::mutex> lock(state->mutex);
            ++state->decoded;
            for (auto it = state->seen.begin(); it != state->seen.end();)
                if (now - it->second.when > 120000) it = state->seen.erase(it); else ++it;
            if (state->seen.size() >= 32 && !state->seen.count(address)) continue;
            auto previous = state->seen.find(address);
            bool changed = previous == state->seen.end();
            if (!changed) {
                const auto& old = previous->second.battery;
                changed = old.model != battery.model || old.left != battery.left || old.right != battery.right ||
                    old.box != battery.box || old.leftCharging != battery.leftCharging ||
                    old.rightCharging != battery.rightCharging || old.boxCharging != battery.boxCharging ||
                    now - previous->second.when > 30000;
            }
            state->seen[address] = {battery, now, rssi};
            state->pending = state->pending || changed;
            // One UI refresh at most every 3 seconds; no disk logs or raw packet retention.
            if (state->pending && state->refresh && now - state->signalled >= 3000) {
                SetEvent(state->refresh); state->signalled = now; state->pending = false;
            }
        }
        return S_OK;
    }
};

struct Receiver {
    std::shared_ptr<State> state = std::make_shared<State>();
    IBluetoothLEAdvertisementWatcher* watcher = nullptr;
    EventRegistrationToken token{};
    HRESULT error = S_OK;
    ULONGLONG attempted = 0;
    bool initialized = false;

    void stop() {
        if (watcher) {
            watcher->Stop(); watcher->remove_Received(token); watcher->Release(); watcher = nullptr;
        }
        if (initialized) { RoUninitialize(); initialized = false; }
    }
    void start() {
        if (watcher) {
            BluetoothLEAdvertisementWatcherStatus status;
            if (SUCCEEDED(watcher->get_Status(&status)) && status == BluetoothLEAdvertisementWatcherStatus_Started) return;
            stop();
        }
        const auto now = GetTickCount64();
        if (attempted && now - attempted < 10000) return;
        attempted = now;
        error = RoInitialize(RO_INIT_MULTITHREADED);
        if (FAILED(error)) return;
        initialized = true;
        const wchar_t* cls = L"Windows.Devices.Bluetooth.Advertisement.BluetoothLEAdvertisementWatcher";
        HSTRING name = nullptr; IInspectable* instance = nullptr;
        error = WindowsCreateString(cls, (UINT32)wcslen(cls), &name);
        if (SUCCEEDED(error)) error = RoActivateInstance(name, &instance);
        if (name) WindowsDeleteString(name);
        if (SUCCEEDED(error)) {
            error = instance->QueryInterface(__uuidof(IBluetoothLEAdvertisementWatcher), (void**)&watcher);
            instance->Release();
        }
        if (FAILED(error)) { stop(); return; }
        error = watcher->put_ScanningMode(BluetoothLEScanningMode_Passive);
        auto* handler = new Handler(state);
        if (SUCCEEDED(error)) error = watcher->add_Received(handler, &token);
        handler->Release();
        if (SUCCEEDED(error)) error = watcher->Start();
        if (FAILED(error)) stop();
    }
};

Receiver& GetReceiver() { static auto* r = new Receiver; return *r; }

std::wstring Level(int pct, bool charging) {
    if (pct < 0) return L"未知";
    return L"约" + std::to_wstring(pct) + L"%" + (charging ? L"（充电中）" : L"");
}
} // namespace

void SetAirPodsRefreshEvent(void* event) {
    auto s = GetReceiver().state;
    std::lock_guard<std::mutex> lock(s->mutex); s->refresh = (HANDLE)event;
}

void StartAirPods() { GetReceiver().start(); }
void StopAirPods() {
    SetAirPodsRefreshEvent(nullptr);
    GetReceiver().stop();
}

void PollAirPods(PollContext& ctx) {
    StartAirPods();
    auto s = GetReceiver().state;
    std::lock_guard<std::mutex> lock(s->mutex);
    const auto now = GetTickCount64();
    // Rotating BLE addresses cannot prove ownership. Report one nearby source per model,
    // explicitly labelled as unverified, rather than attributing it to a paired device.
    std::map<uint16_t, Seen> strongest;
    for (const auto& [address, seen] : s->seen) {
        if (now - seen.when > 120000) continue;
        auto it = strongest.find(seen.battery.model);
        if (it == strongest.end() || seen.rssi > it->second.rssi || now - it->second.when > 15000)
            strongest[seen.battery.model] = seen;
    }
    for (const auto& [model, seen] : strongest) {
        const auto& b = seen.battery;
        DeviceStatus status;
        status.key = L"airpods-nearby:" + Hex4(model);
        status.name = std::wstring(airpods::ModelName(model)) + L"（附近广播）";
        status.kind = DeviceKind::Headset;
        status.via = L"Apple BLE";
        status.percent = airpods::EarPercent(b);
        const bool onlyBox = status.percent < 0;
        if (onlyBox) status.percent = b.box;
        status.levelText = (onlyBox ? L"盒约" : L"约") + std::to_wstring(status.percent) + L"%";
        status.charging = onlyBox ? b.boxCharging :
            (((b.left >= 0 && b.leftCharging) && (b.right < 0 || b.rightCharging)) ||
             (b.left < 0 && b.right >= 0 && b.rightCharging));
        status.detailText = L"左耳" + Level(b.left, b.leftCharging) + L"；右耳" + Level(b.right, b.rightCharging) +
            L"；充电盒" + Level(b.box, b.boxCharging) + L"；附近广播，归属未验证";
        if (now - seen.when > 30000) {
            status.asleep = true; status.percent = -1;
            status.levelText.clear(); status.detailText += L"；历史读数";
        }
        ctx.out.push_back(status);
    }
}

std::wstring DiagnoseAirPods(unsigned seconds) {
    StartAirPods();
    Sleep(std::min(seconds, 30u) * 1000);
    auto& r = GetReceiver();
    BluetoothLEAdvertisementWatcherStatus status = BluetoothLEAdvertisementWatcherStatus_Aborted;
    if (r.watcher) r.watcher->get_Status(&status);
    std::wstring output;
    {
        std::lock_guard<std::mutex> lock(r.state->mutex);
        wchar_t line[200];
        swprintf(line, 200, L"BLE status=%d, HRESULT=0x%08lX, events=%u, apple=%u, airpods=%u\n",
                 (int)status, (unsigned long)r.error, r.state->events, r.state->apple, r.state->decoded);
        output = line;
        for (const auto& [address, seen] : r.state->seen) {
            const auto& b = seen.battery;
            output += std::wstring(airpods::ModelName(b.model)) + L": 左耳" + Level(b.left, b.leftCharging) +
                L"，右耳" + Level(b.right, b.rightCharging) + L"，充电盒" + Level(b.box, b.boxCharging) +
                L"，信号 " + std::to_wstring(seen.rssi) + L" dBm\n";
        }
    }
    StopAirPods();
    return output;
}

void RefreshAirPods(std::vector<DeviceStatus>& devices) {
    devices.erase(std::remove_if(devices.begin(), devices.end(), [](const DeviceStatus& s) {
        return s.key.compare(0, 15, L"airpods-nearby:") == 0;
    }), devices.end());
    PollContext ctx;
    PollAirPods(ctx);
    devices.insert(devices.end(), ctx.out.begin(), ctx.out.end());
}

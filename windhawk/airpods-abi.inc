// Minimal declarations for the stable Windows Bluetooth advertisement ABI.
// Windhawk 1.7.3's SDK lacks windows.devices.bluetooth.advertisement.h.
// Interface order and IDs match the Windows SDK / mingw-w64 public WinRT headers.
// Unused method signatures below keep the original vtable slots; they are never invoked.
namespace WindhawkAirPodsAbi {
enum BluetoothLEAdvertisementWatcherStatus { BluetoothLEAdvertisementWatcherStatus_Created, BluetoothLEAdvertisementWatcherStatus_Started,
    BluetoothLEAdvertisementWatcherStatus_Stopping, BluetoothLEAdvertisementWatcherStatus_Stopped, BluetoothLEAdvertisementWatcherStatus_Aborted };
enum BluetoothLEScanningMode { BluetoothLEScanningMode_Passive, BluetoothLEScanningMode_Active };
struct IBluetoothLEAdvertisementWatcher;
struct IBluetoothLEAdvertisementReceivedEventArgs;
struct IBluetoothLEManufacturerData;

MIDL_INTERFACE("90eb4eca-d465-5ea0-a61c-033c8c5ecef2") Received : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Invoke(IBluetoothLEAdvertisementWatcher*, IBluetoothLEAdvertisementReceivedEventArgs*) = 0;
};
MIDL_INTERFACE("78ab070e-ad7e-5912-a4f1-7be33e4560af") DataList : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE GetAt(UINT32, IBluetoothLEManufacturerData**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Size(UINT32*) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedIndexOf() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetMany() = 0;
};
MIDL_INTERFACE("912dba18-6963-4533-b061-4694dafb34e5") IBluetoothLEManufacturerData : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_CompanyId(UINT16*) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_CompanyId(UINT16) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Data(ABI::Windows::Storage::Streams::IBuffer**) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Data(ABI::Windows::Storage::Streams::IBuffer*) = 0;
};
MIDL_INTERFACE("066fb2b7-33d1-4e7d-8367-cf81d0f79653") IBluetoothLEAdvertisement : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE UnusedGetFlags() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedPutFlags() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetLocalName() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedPutLocalName() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetServiceUuids() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetManufacturerData() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetDataSections() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetManufacturerDataByCompanyId(UINT16, DataList**) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetSectionsByType() = 0;
};
MIDL_INTERFACE("27987ddf-e596-41be-8d43-9e6731d4a913") IBluetoothLEAdvertisementReceivedEventArgs : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_RawSignalStrengthInDBm(INT16*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_BluetoothAddress(UINT64*) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetAdvertisementType() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetTimestamp() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Advertisement(IBluetoothLEAdvertisement**) = 0;
};
MIDL_INTERFACE("a6ac336f-f3d3-4297-8d6c-c81ea6623f40") IBluetoothLEAdvertisementWatcher : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE UnusedGetMinSamplingInterval() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetMaxSamplingInterval() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetMinOutOfRangeTimeout() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetMaxOutOfRangeTimeout() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Status(BluetoothLEAdvertisementWatcherStatus*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ScanningMode(BluetoothLEScanningMode*) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_ScanningMode(BluetoothLEScanningMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetSignalStrengthFilter() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedPutSignalStrengthFilter() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedGetAdvertisementFilter() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedPutAdvertisementFilter() = 0;
    virtual HRESULT STDMETHODCALLTYPE Start() = 0;
    virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
    virtual HRESULT STDMETHODCALLTYPE add_Received(Received*, EventRegistrationToken*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_Received(EventRegistrationToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedAddStopped() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnusedRemoveStopped() = 0;
};
}
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(WindhawkAirPodsAbi::Received, 0x90eb4eca, 0xd465, 0x5ea0, 0xa6, 0x1c, 0x03, 0x3c, 0x8c, 0x5e, 0xce, 0xf2)
__CRT_UUID_DECL(WindhawkAirPodsAbi::IBluetoothLEAdvertisementWatcher, 0xa6ac336f, 0xf3d3, 0x4297, 0x8d, 0x6c, 0xc8, 0x1e, 0xa6, 0x62, 0x3f, 0x40)
#endif

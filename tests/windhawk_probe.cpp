// Compile with Windhawk's own compiler + WH_EDITING to exercise the generated ABI/native engine.
#include "../windhawk/taskbar-wireless-battery-monitor.wh.cpp"
#include <iostream>

int wmain() {
    ModConfig::airpodsEnabled = true; // Hardware probe explicitly opts into BLE reception.
    StartAirPods();
    Sleep(12000);
    auto& receiver = wb_driver_airpods::GetReceiver();
    WindhawkAirPodsAbi::BluetoothLEAdvertisementWatcherStatus status =
        WindhawkAirPodsAbi::BluetoothLEAdvertisementWatcherStatus_Aborted;
    if (receiver.watcher) receiver.watcher->get_Status(&status);
    bool started = status == WindhawkAirPodsAbi::BluetoothLEAdvertisementWatcherStatus_Started;
    std::cout << "BLE status=" << (int)status << ", HRESULT=" << (unsigned long)receiver.error << "\n";
    auto devices = PollAllDevices();
    std::cout << "Native engine devices: " << devices.size() << "\n";
    StopAirPods();
    return started ? 0 : 1;
}

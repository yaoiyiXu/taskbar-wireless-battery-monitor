#pragma once
#include <string>
#include "../devices.h"

// Live, read-only BLE reception; no pairing, settings or audio changes.
void StartAirPods();
void StopAirPods();
void SetAirPodsRefreshEvent(void* event);
std::wstring DiagnoseAirPods(unsigned seconds);
void RefreshAirPods(std::vector<DeviceStatus>& devices);

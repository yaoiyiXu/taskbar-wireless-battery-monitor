// The UI and device polling are kept on owned threads. Windhawk only controls lifetime/settings.
namespace ModHost {
HANDLE uiThread = nullptr;
DWORD uiThreadId = 0;
std::atomic<bool> stopRequested{false};
std::atomic<HWND> window{nullptr};
HMODULE module = nullptr;

DWORD WINAPI Run(LPVOID) {
    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    int result = wb_app::Run(module);
    window = nullptr;
    UnregisterClassW(L"TaskbarWirelessBatteryWidget", module);
    UnregisterClassW(L"TaskbarWirelessBatteryWnd", module);
    Wh_Log(L"Battery UI thread exited, result=%d", result);
    return 0;
}
}

BOOL WhTool_ModInit() {
    for (PCWSTR name : {L"Local\\TaskbarWirelessBatteryTray", L"Local\\WirelessBatteryTray"}) {
        HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE, name);
        if (existing) {
            CloseHandle(existing);
            Wh_Log(L"Exit both the original and Chinese standalone WirelessBattery.exe before enabling this mod.");
            return FALSE;
        }
    }
    ModConfig::Load();
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&ModHost::Run), &ModHost::module)) return FALSE;
    ModHost::stopRequested = false;
    ModHost::uiThread = CreateThread(nullptr, 0, ModHost::Run, nullptr, 0, &ModHost::uiThreadId);
    if (!ModHost::uiThread) return FALSE;
    Wh_Log(L"Taskbar Wireless Battery Monitor started, host PID=%lu", GetCurrentProcessId());
    return TRUE;
}

void WhTool_ModSettingsChanged() {
    ModConfig::Load();
    if (HWND window = ModHost::window.load()) PostMessageW(window, WM_APP + 80, 0, 0);
}

void WhTool_ModUninit() {
    if (ModHost::uiThread) {
        ModHost::stopRequested = true;
        if (HWND window = ModHost::window.load()) PostMessageW(window, WM_CLOSE, 0, 0);
        if (WaitForSingleObject(ModHost::uiThread, 5000) != WAIT_OBJECT_0) {
            // Only the dedicated -tool-mod process reaches this callback.
            // Never return to DLL unload with a live UI/driver thread.
            ExitProcess(0);
        }
        CloseHandle(ModHost::uiThread); ModHost::uiThread = nullptr;
    }
}

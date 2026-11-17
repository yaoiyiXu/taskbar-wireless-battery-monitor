// The UI and device polling are kept on owned threads. Windhawk only controls lifetime/settings.
namespace ModHost {
HANDLE uiThread = nullptr, readyEvent = nullptr;
DWORD uiThreadId = 0;
std::atomic<bool> ready{false};
HMODULE module = nullptr;

DWORD WINAPI Run(LPVOID) {
    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    wb_app::wWinMain(module, nullptr, const_cast<PWSTR>(L""), 0);
    UnregisterClassW(L"TaskbarWirelessBatteryWidget", module);
    UnregisterClassW(L"TaskbarWirelessBatteryWnd", module);
    SetEvent(readyEvent);
    return 0;
}
}

BOOL WhTool_ModInit() {
    HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\TaskbarWirelessBatteryTray");
    if (existing) {
        CloseHandle(existing);
        Wh_Log(L"请先退出独立版 WirelessBattery.exe，再启用此模组。");
        return FALSE;
    }
    ModConfig::Load();
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&ModHost::Run), &ModHost::module)) return FALSE;
    ModHost::readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ModHost::readyEvent) return FALSE;
    ModHost::uiThread = CreateThread(nullptr, 0, ModHost::Run, nullptr, 0, &ModHost::uiThreadId);
    if (!ModHost::uiThread) { CloseHandle(ModHost::readyEvent); return FALSE; }
    WaitForSingleObject(ModHost::readyEvent, INFINITE);
    if (!ModHost::ready.load()) {
        WaitForSingleObject(ModHost::uiThread, INFINITE);
        CloseHandle(ModHost::uiThread); ModHost::uiThread = nullptr;
        CloseHandle(ModHost::readyEvent); ModHost::readyEvent = nullptr;
        return FALSE;
    }
    Wh_Log(L"任务栏无线设备电量监控已启动，宿主 PID=%lu", GetCurrentProcessId());
    return TRUE;
}

void WhTool_ModSettingsChanged() {
    ModConfig::Load();
    if (wb_app::g_hwnd) PostMessageW(wb_app::g_hwnd, WM_APP + 80, 0, 0);
}

void WhTool_ModUninit() {
    if (ModHost::uiThread) {
        if (wb_app::g_hwnd) PostMessageW(wb_app::g_hwnd, WM_CLOSE, 0, 0);
        WaitForSingleObject(ModHost::uiThread, INFINITE);
        CloseHandle(ModHost::uiThread); ModHost::uiThread = nullptr;
    }
    if (ModHost::readyEvent) { CloseHandle(ModHost::readyEvent); ModHost::readyEvent = nullptr; }
}

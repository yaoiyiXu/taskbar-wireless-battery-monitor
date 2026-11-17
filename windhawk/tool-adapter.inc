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
    int result = wb_app::wWinMain(module, nullptr, const_cast<PWSTR>(L""), 0);
    window = nullptr;
    UnregisterClassW(L"TaskbarWirelessBatteryWidget", module);
    UnregisterClassW(L"TaskbarWirelessBatteryWnd", module);
    Wh_Log(L"电量界面线程结束，结果=%d", result);
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
    ModHost::stopRequested = false;
    ModHost::uiThread = CreateThread(nullptr, 0, ModHost::Run, nullptr, 0, &ModHost::uiThreadId);
    if (!ModHost::uiThread) return FALSE;
    Wh_Log(L"任务栏无线设备电量监控已启动，宿主 PID=%lu", GetCurrentProcessId());
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
        WaitForSingleObject(ModHost::uiThread, INFINITE);
        CloseHandle(ModHost::uiThread); ModHost::uiThread = nullptr;
    }
}

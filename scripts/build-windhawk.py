"""Generate the single-file tool mod while isolating each original translation unit."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parent.parent
DEST = ROOT / 'windhawk' / 'taskbar-wireless-battery-monitor.wh.cpp'

def read(path):
    return (ROOT / path).read_text(encoding='utf-8-sig')

def strip_includes(text):
    return re.sub(r'^\s*#\s*(?:include\b[^\n]*|pragma once)[\r\n]*', '', text, flags=re.M)

def replace_function(text, signature, replacement):
    start = text.index(signature)
    brace = text.index('{', start)
    level = 1
    end = brace + 1
    while level:
        level += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[:start] + replacement + text[end:]

runtime_strings = json.loads(read('windhawk/runtime-strings.json'))

def localize(text):
    """Keep the standalone app Chinese; make the catalog mod English-first."""
    def replace(match):
        literal = match.group()
        if not re.search(r'[\u3400-\u9fff]', literal):
            return literal
        chinese = json.loads(literal[1:])
        if chinese not in runtime_strings:
            raise ValueError(f'Missing English runtime string: {chinese!r}')
        english = json.dumps(runtime_strings[chinese], ensure_ascii=False)
        return f'ui::Text(L{english}, {literal})'
    return re.sub(r'L"(?:[^"\\]|\\.)*"', replace, text)

headers = ['src/devices.h', 'src/hid.h', 'src/drivers/drivers.h', 'src/widget.h',
           'src/localization.h', 'src/drivers/airpods.h', 'src/drivers/airpods_protocol.h', 'src/drivers/rapoo_protocol.h']
sources = sorted(p for p in (ROOT / 'src/drivers').glob('*.cpp') if p.stem != 'headsetcontrol')
inputs = [ROOT / p for p in headers] + sources + [ROOT / ('src/' + f) for f in ['hid.cpp', 'devices.cpp', 'widget.cpp', 'main.cpp']]
all_text = '\n'.join(p.read_text(encoding='utf-8') for p in inputs)
includes = sorted(set(re.findall(r'^\s*#include\s*<([^>]+)>', all_text, re.M)))
special = {'windows.h', 'initguid.h', 'hidsdi.h', 'hidpi.h', 'optional', 'windows.system.power.h', 'windows.devices.power.h',
           'windows.gaming.input.h', 'windows.devices.bluetooth.advertisement.h', 'robuffer.h'}
preamble = '#include <windows.h>\n#include <windhawk_utils.h>\n#include <initguid.h>\n#include <hidsdi.h>\n#include <hidpi.h>\n' + ''.join(f'#include <{i}>\n' for i in includes if i not in special)
preamble += '''
#ifdef __MINGW32__
#define boolean bool
#endif
#include <windows.system.power.h>
namespace ABI { namespace Windows { namespace Devices { namespace Power {
using ABI::Windows::System::Power::BatteryStatus;
} } } }
#include <windows.devices.power.h>
#include <windows.gaming.input.h>
#include <windows.storage.streams.h>
#include <robuffer.h>
#ifdef __MINGW32__
#undef boolean
#endif
'''

config = '''
namespace ModConfig {
inline std::atomic<int> pollMs{60000}, warning{15}, fontSize{13}, gap{14};
inline std::atomic<bool> taskbar{true}, hideSleeping{false}, airpodsEnabled{false};
inline void* bleEvent = nullptr;
inline void Load() {
    pollMs = std::clamp(Wh_GetIntSetting(L"pollSeconds"), 10, 600) * 1000;
    warning = std::clamp(Wh_GetIntSetting(L"warningPercent"), 0, 50);
    fontSize = std::clamp(Wh_GetIntSetting(L"fontSize"), 10, 24);
    gap = std::clamp(Wh_GetIntSetting(L"deviceGap"), 4, 40);
    hideSleeping = !!Wh_GetIntSetting(L"hideSleeping");
    airpodsEnabled = !!Wh_GetIntSetting(L"enableAirPods");
    auto mode = WindhawkUtils::StringSetting::make(L"displayMode");
    taskbar = wcscmp(mode, L"tray") != 0;
}
}
namespace ModHost { extern std::atomic<bool> stopRequested; extern std::atomic<HWND> window; }
'''
parts = [read('windhawk/mod-header.inc'),
         '// Source modules and reproducible bundler: https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor\n',
         '// Driver namespaces isolate file-local helpers from the original translation units.\n',
         preamble, read('windhawk/airpods-abi.inc'), read('windhawk/localization.inc')]
for path in headers:
    header = strip_includes(read(path))
    if path == 'src/localization.h':
        header = header.replace('    struct Entry', '    if (!ui::IsChinese()) return value;\n    struct Entry')
    if path == 'src/devices.h':
        header = header.replace('PollAllDevices()', 'PollAllDevices(HANDLE stopEvent = nullptr)')
        header = re.sub(r'// Prints every HID collection.*?std::wstring DumpHidCollections\(\);', '', header, flags=re.S)
    if path == 'src/drivers/airpods.h':
        header = re.sub(r'^std::wstring DiagnoseAirPods.*\n', '', header, flags=re.M)
    if path == 'src/drivers/drivers.h':
        header = re.sub(r'^void PollHeadsetControl.*\n', '', header, flags=re.M)
    parts.append(header)
parts += [config, strip_includes(read('src/hid.cpp'))]
public_polls = set(re.findall(r'void (Poll\w+)\(PollContext& ctx\);', read('src/drivers/drivers.h')))
for path in sources:
    name = path.stem
    text = path.read_text(encoding='utf-8')
    if name in ('airpods', 'xinput'):
        text = text[text.index('namespace {'):]
    if name == 'airpods':
        text = text.replace('using namespace ABI::Windows::Devices::Bluetooth::Advertisement;', 'using namespace WindhawkAirPodsAbi;')
        text = re.sub(r'^using Received = .*?;\n', '', text, flags=re.M)
        text = re.sub(r'^using DataList = .*?;\n', '', text, flags=re.M)
        text = text.replace('    PollAirPods(ctx);', '    wb_driver_airpods::PollAirPods(ctx);')
        text = replace_function(text, 'std::wstring DiagnoseAirPods(', '')
        text = localize(text)
    text = strip_includes(text)
    ns = 'wb_driver_' + name
    parts.append(f'\nnamespace {ns} {{\n{text}\n}}\n')
    for function in sorted(public_polls):
        if re.search(r'void ' + function + r'\(PollContext& ctx\)\s*\{', text):
            if function == 'PollAirPods':
                parts.append(f'void {function}(PollContext& ctx) {{ if (ModConfig::airpodsEnabled) {ns}::{function}(ctx); }}\n')
            else:
                parts.append(f'void {function}(PollContext& ctx) {{ {ns}::{function}(ctx); }}\n')
    if name == 'glorious':
        parts.append(f'const wchar_t* GWolvesModelName(USHORT p) {{ return {ns}::GWolvesModelName(p); }}\nbool GWolvesIsCable(USHORT p) {{ return {ns}::GWolvesIsCable(p); }}\n')
    if name == 'airpods':
        parts.append(f'''
void StartAirPods() {{
    if (ModConfig::airpodsEnabled) {{
        {ns}::SetAirPodsRefreshEvent(ModConfig::bleEvent); {ns}::StartAirPods();
    }} else {{ {ns}::StopAirPods(); }}
}}
void StopAirPods() {{ {ns}::StopAirPods(); }}
void SetAirPodsRefreshEvent(void* e) {{ ModConfig::bleEvent = e; {ns}::SetAirPodsRefreshEvent(e); }}
void RefreshAirPods(std::vector<DeviceStatus>& v) {{
    if (ModConfig::airpodsEnabled) {ns}::RefreshAirPods(v);
    else {{
        {ns}::StopAirPods();
        v.erase(std::remove_if(v.begin(), v.end(), [](const DeviceStatus& d) {{ return d.key.rfind(L"airpods-nearby:", 0) == 0; }}), v.end());
    }}
}}
''')
poll = strip_includes(read('src/devices.cpp'))
poll = replace_function(poll, 'std::wstring DumpHidCollections()', '')
poll = re.sub(r'^    PollHeadsetControl\(ctx\);\n', '', poll, flags=re.M)
poll = poll.replace('PollAllDevices()', 'PollAllDevices(HANDLE stopEvent)')
poll = poll.replace('    StartAirPods();', '    if (stopEvent && WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0) return {};\n    StartAirPods();')
# A stop skips remaining drivers, while each active query retains its own timeout.
poll = re.sub(r'^(    Poll\w+\(ctx\);)', r'    if (stopEvent && WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0) return ctx.out;\n\1', poll, flags=re.M)
parts.append('namespace wb_poll {\n' + poll + '\n}\n')
parts.append('std::vector<DeviceStatus> PollAllDevices(HANDLE stopEvent) { return wb_poll::PollAllDevices(stopEvent); }\n')

widget = localize(strip_includes(read('src/widget.cpp'))).replace('MakeFont(Px(13),', 'MakeFont(Px(ModConfig::fontSize.load()),').replace('Px(14) : 0;', 'Px(ModConfig::gap.load()) : 0;')
parts.append('''namespace wb_render {
namespace widget {
bool Create(HINSTANCE, HWND, UINT, bool);
void Destroy();
bool Alive();
void SetItems(const std::vector<WidgetItem>&, const std::wstring&);
void SetLightTheme(bool);
bool Reposition();
}
''' + widget + '\n}\n')
parts.append('''namespace widget {
bool Create(HINSTANCE i, HWND w, UINT m, bool l) { return wb_render::widget::Create(i,w,m,l); }
void Destroy() { wb_render::widget::Destroy(); }
bool Alive() { return wb_render::widget::Alive(); }
void SetItems(const std::vector<WidgetItem>& i, const std::wstring& s) { wb_render::widget::SetItems(i,s); }
void SetLightTheme(bool b) { wb_render::widget::SetLightTheme(b); }
bool Reposition() { return wb_render::widget::Reposition(); }
}
''')

app = strip_includes(read('src/main.cpp'))
for signature in ('std::wstring ExePath()', 'bool LoadBoolSetting(', 'void SaveBoolSetting(',
                  'void SaveWidgetMode(', 'void SaveHideLastKnown(', 'bool AutostartEnabled()',
                  'void SetAutostart(', 'void SetMode(', 'void SetHideLastKnown(', 'int ListToConsole('):
    app = replace_function(app, signature, '')
app = re.sub(r'^const wchar_t\* k(?:RunKey|RunValue|SettingsKey) = .*\n', '', app, flags=re.M)
app = re.sub(r'^// `WirelessBattery.exe.*\n', '', app, flags=re.M)
app = app.replace('enum MenuId : UINT { ID_REFRESH = 100, ID_AUTOSTART, ID_HIDE_LAST_KNOWN, ID_EXIT, ID_MODE_WIDGET, ID_MODE_TRAY, ID_DEVICE_FIRST = 1000 };',
                  'enum MenuId : UINT { ID_REFRESH = 100, ID_DEVICE_FIRST = 1000 };')
app = re.sub(r'int WINAPI wWinMain\(.*?    g_inst = inst;', 'int Run(HINSTANCE inst) {\n    g_inst = inst;', app, flags=re.S)
app = app.replace('auto devices = PollAllDevices();', 'auto devices = PollAllDevices(g_stopEvt);')
app = app.replace('devices = PollAllDevices();', 'devices = PollAllDevices(g_stopEvt);')
app = re.sub(r'constexpr DWORD POLL_INTERVAL_MS = .*?;', '', app)
app = re.sub(r'constexpr int LOW_BATTERY = .*?;', '', app)
app = app.replace('POLL_INTERVAL_MS', '(DWORD)ModConfig::pollMs.load()').replace('LOW_BATTERY', 'ModConfig::warning.load()')
app = replace_function(app, 'void LoadHidden()', '''void LoadHidden() {
    g_hidden.clear();
    std::vector<wchar_t> data(65536, 0);
    Wh_GetStringValue(L"HiddenDevices", data.data(), data.size());
    std::wstring text(data.data());
    for (size_t start = 0; start < text.size();) {
        // Accept the old newline format and the spaces written by portable 1.7.3.
        // Keys contain fixed prefixes, hex IDs, numbers and GUIDs, never these separators.
        size_t end = text.find_first_of(L"| \\r\\n", start);
        if (end != start) g_hidden.insert(text.substr(start, end == std::wstring::npos ? end : end-start));
        if (end == std::wstring::npos) break;
        start = end+1;
    }
}''')
app = replace_function(app, 'void SaveHidden()', '''void SaveHidden() {
    std::wstring text;
    // A single-line separator also works with Windhawk's portable INI storage.
    for (const auto& key : g_hidden) {
        if (!text.empty()) text += L'|';
        text += key;
    }
    Wh_SetStringValue(L"HiddenDevices", text.c_str());
}''')
app = replace_function(app, 'bool LoadWidgetMode()', 'bool LoadWidgetMode() { return ModConfig::taskbar.load(); }')
app = replace_function(app, 'bool LoadHideLastKnown()', 'bool LoadHideLastKnown() { return ModConfig::hideSleeping.load(); }')
app = re.sub(r'^.*AppendMenuW\(m,.*(?:ID_HIDE_LAST_KNOWN|ID_MODE_WIDGET|ID_MODE_TRAY|ID_AUTOSTART|ID_EXIT).*$\n?', '', app, flags=re.M)
app = re.sub(r'^.*CheckMenuRadioItem\(m,.*$\n?', '', app, flags=re.M)
app = re.sub(r'^        case ID_(?:AUTOSTART|HIDE_LAST_KNOWN|MODE_WIDGET|MODE_TRAY|EXIT):.*\n', '', app, flags=re.M)
app = app.replace('    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);\n    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);',
                  '    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);')
app = app.replace('// Devices the user chose to hide (by key), stored as a REG_MULTI_SZ.',
                  '// Device keys the user chose to hide, stored in Windhawk mod storage.')
app = app.replace('POINT pt;\n    GetCursorPos', 'AppendMenuW(m, MF_STRING | MF_GRAYED, 0, L"显示设置和启停请在 Windhawk 中调整");\n    POINT pt;\n    GetCursorPos')
app = app.replace('case WM_RESULTS: {', '''case WM_APP + 80:
            g_hideLastKnown = ModConfig::hideSleeping.load();
            g_wantWidget = ModConfig::taskbar.load();
            RedrawAll(); Reconcile(); UpdateWidget();
            SetEvent(g_refreshEvt);
            return 0;
        case WM_RESULTS: {''')
app = app.replace('WaitForSingleObject(g_worker, 5000)', 'WaitForSingleObject(g_worker, INFINITE)')
app = app.replace('    HANDLE mutex = CreateMutexW', '    // Shared with WirelessBattery.exe so the standalone app and mod cannot overlap.\n    HANDLE mutex = CreateMutexW')
app = app.replace('if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;',
                  'if (!mutex) return 1;\n    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mutex); return 0; }')
app = app.replace('if (!g_hwnd) return 1;', 'if (!g_hwnd) { ReleaseMutex(mutex); CloseHandle(mutex); return 1; }\n    ModHost::window = g_hwnd;\n    if (ModHost::stopRequested) PostMessageW(g_hwnd, WM_CLOSE, 0, 0);')
app = app.replace('    ReleaseMutex(mutex);\n    return 0;',
                  '    ReleaseMutex(mutex);\n    CloseHandle(mutex);\n    CloseHandle(g_refreshEvt);\n    CloseHandle(g_stopEvt);\n    CloseHandle(g_airpodsEvt);\n    return 0;')
parts.append('namespace wb_app {\n' + localize(app) + '\n}\n')
parts += [read('windhawk/tool-adapter.inc'), read('windhawk/tool-host.inc')]
text = '\n'.join(parts).rstrip() + '\n'
text = re.sub(r'\n{3,}', '\n\n', text)
DEST.parent.mkdir(exist_ok=True)
DEST.write_text(text, encoding='utf-8', newline='\n')
print(f'Generated {DEST} ({len(text.encode("utf-8")):,} bytes)')

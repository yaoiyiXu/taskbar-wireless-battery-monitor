# 任务栏无线设备电量监控

在 Windows 10 / 11 任务栏直接查看无线鼠标、键盘、耳机和游戏手柄的电量。简体中文界面，支持多品牌外设、蓝牙及部分 2.4 GHz 接收器。

[下载安装版或便携版](https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor/releases/latest) · [反馈问题](https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor/issues) · [完整支持型号（英文）](README.en.md#supported-devices) · [更新记录](CHANGELOG.md)

本项目基于 [Wireless Battery](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor) 和社区 [简体中文 PR #43](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor/pull/43) 制作，是独立中文衍生版。保留原作者、贡献者、协议来源及 GPL-3.0-or-later 许可证，详见 [来源说明](NOTICE.md)。

## Windhawk 模组（0.1.2）

也可通过 Windhawk 管理本工具的启停和设置。模组在独立宿主中运行，保留内置厂商协议和 AirPods 广播解析，暂不调用外部 HeadsetControl。

- [单文件模组源码](windhawk/taskbar-wireless-battery-monitor.wh.cpp)：在 Windhawk 中创建新模组，粘贴完整代码并编译。
- [本地安装与验证说明](windhawk/本地安装与验证.md)：安装前先退出独立版，避免重复运行。
- [源码生成脚本](scripts/build-windhawk.py)和[本地编译检查脚本](scripts/test-windhawk-build.ps1)：维护多文件源码并生成单个 `.wh.cpp`。

已提交 [Windhawk 官方目录审核（PR #7564）](https://github.com/ramensoftware/windhawk-mods/pull/7564)，尚未正式收录。0.1.2 按系统语言选择中文或英文，修复便携版隐藏设备配置，AirPods 广播接收默认关闭，需要在模组设置中手动开启。

0.1.1 已在 Windhawk 1.7.3 中通过本地加载验证，用户反馈基本运行正常。0.1.2 已完成 x86 / x64 编译，以及中英文显示、INI 跨进程保存与恢复、旧配置迁移和实例互斥回归检查；其他系统、设备和任务栏组合仍需验证。

## 功能

- **任务栏电量条**：设备图标和电量直接显示在右下角通知区域旁，支持明暗主题和 DPI 缩放。
- **托盘图标模式**：每台设备显示独立数字图标；竖向任务栏或组件创建失败时自动采用此模式。
- **中文状态提示**：菜单、悬浮提示、休眠/充电状态、低电量通知、命令行及安装界面使用简体中文。
- **充电与低电量显示**：蓝色闪电表示充电，不超过 15% 时显示红色；托盘模式支持低电量通知。
- **自动刷新**：默认每 60 秒读取一次，插拔设备或电脑唤醒后自动更新，也可手动刷新。
- **AirPods 电量**：通过苹果 BLE 广播读取耳机与充电盒电量，不依赖 Windows 的电量属性；悬停可查看左右耳、充电盒及充电状态。
- **休眠与历史读数**：无法读取新电量时以暗色显示历史值，也可隐藏仅有历史读数的设备。
- **按设备显示或隐藏**：点击菜单中的设备即可切换，设置自动保存。
- **可选开机自启动**：通过菜单启用，仅作用于当前用户。
- **轻量部署**：原生 C++17 / Win32，无额外运行时、后台服务或自定义驱动；提供便携版与安装版。

多数设备无需运行厂商管理软件。ATK N9 Pro 等采用独占通信的设备有例外，读取时须关闭 ATK 桌面程序。

## 安装与使用

1. 打开 [最新发行版](https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor/releases/latest)。
2. 下载 `WirelessBattery-Setup-版本.exe` 并安装；或下载 `WirelessBattery-版本-portable.zip`，解压后运行 `WirelessBattery.exe`。
3. 点击任务栏电量区域或托盘图标，打开中文菜单。

安装版默认按当前用户安装，无需管理员权限。便携版无需安装；发行包附带 `headsetcontrol.exe`，请与主程序放在同一目录，用于补充耳机支持。

Windows 11 可能将托盘图标放入折叠区，可将图标拖到任务栏上。程序尚无代码签名，Windows 可能显示 SmartScreen 提示；请核对下载来源，或自行编译。

本版使用独立安装目录、程序实例、设置和自启动标识，不会覆盖上游配置；建议日常只运行一个版本，避免重复显示。

## 设备支持

**支持品牌不代表支持全部型号。** 以下为协议适配范围；精确型号、连接方式及限制请参阅 [完整支持表](README.en.md#supported-devices)。

| 类型 | 已有适配 |
| --- | --- |
| 鼠标、键盘 | Logitech、Razer、SteelSeries、Corsair、Cooler Master、Glorious、Hitscan、Kysona、Pulsar、ATK / VXE、G-Wolves、Keychron 鼠标、LAMZU、Redragon、AULA、ASUS ROG、ZOWIE、部分雷柏 VT、Motospeed、Delux、Ajazz、Attack Shark 等 |
| 耳机 | 部分 Logitech G、Razer BlackShark V3、SteelSeries Arctis / Nova、Corsair、HyperX、Sony INZONE、ATK N9 Pro，以及 HeadsetControl 支持的耳机 |
| 手柄 | Sony DualShock 4、DualSense、DualSense Edge；Xbox 手柄；部分 GameSir Nova 系列 |
| 迈从键盘 | MCHOSE G87，限 `41E4:2001` 2.4 GHz 接收器；充电状态、有线、蓝牙和 G87 V2 未验证 |
| 苹果耳机 | AirPods 1/2/3/4、Pro、Pro 2（Lightning / USB-C）、Pro 3、Max 的已知 proximity 广播标识；按广播解析，除下述接收验证外未逐型号实机验证 |
| 通用设备 | 暴露标准 HID 电池信息的设备；Windows 本身能读取电量的蓝牙设备 |

上游列出实机验证过的 Glorious Model O Wireless、HyperX Cloud III Wireless、Cooler Master MM712、雷柏 VT3 Max Gen 2 和 Redragon M916 Pro 1K；其他许多型号来自协议资料，尚未逐一实测。本项目保留这些适配，**没有重新验证所有硬件**。

社区 G87 适配曾读取到 55%～56%，但最终将超时缩短为一秒后的醒机行为仍需实机确认，不代表所有 G87 均兼容。

## 已知限制

- **AirPods 广播电量是约 10% 一档的估计值**，任务栏显示“约”；左右耳均有数据时显示较低的一侧，只有盒电量时显示“盒约”。悬停查看各部件，未知值不会当成 0%。
- AirPods 充电盒的读数依赖开盖等广播时机；首次使用可将充电盒放在电脑旁开盖。超过 30 秒未更新的读数标成历史值，超过 2 分钟移除。
- BLE 广播使用随机地址，无法验证归属。因此显示为“附近广播”，不会假定它属于某个已配对的设备。同型号附近有多组时仅显示较近的一组，可能读到他人的设备。此版本不提供已验证的设备绑定。
- 需要蓝牙适配器支持 Windows BLE 广播接收。扫描为被动接收，不修改配对、音频设置或固件；无电量数据时不编造百分比。
- 任务栏组件仅显示在主显示器上；Windows 11 居中任务栏过于拥挤时可能与应用按钮重叠，可切换托盘模式。
- 休眠设备可能不响应。部分设备历史读数最多保留 30 分钟，暗色读数不代表实时电量。
- 蓝牙电量依赖 Windows；Windows 读不到的电量，本工具也不能凭空获取。
- 部分设备只上报“偏低 / 中等 / 充足”等粗略等级，不代表精确百分比。
- Keychron 键盘的 2.4 GHz 固件不提供电量，请使用蓝牙；Keychron 鼠标有单独适配。
- 部分雷柏 VT 查询最多等待 4 秒；办公 MT/M 系列、CH584 等其他协议不在当前驱动覆盖范围内。
- MSI 无线鼠标/键盘、Razer Kraken / Barracuda 耳机、HyperX Pulsefire 鼠标等尚未支持；Xbox Wireless Headset 请尝试蓝牙。
- HeadsetControl 自身诊断信息及厂商型号名称保留原文。

## 命令行

```powershell
.\WirelessBattery.exe --list   # 列出设备和电量，然后退出
.\WirelessBattery.exe --dump   # 列出 HID 接口，辅助适配
.\WirelessBattery.exe --quit   # 退出当前中文版实例
.\WirelessBattery.exe --airpods # 接收 12 秒广播并输出电量诊断，不输出地址或加密载荷
```

终端输出使用 Unicode，重定向时使用 UTF-8。反馈兼容性时请附设备型号、连接方式及上述输出，分享前遮盖个人设备标识。

## 从源码编译

需要 Windows、MinGW-w64（`g++`、`windres`）。将工具链的 `bin` 加入 PATH 后运行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
powershell -ExecutionPolicy Bypass -File scripts/test-windows.ps1
```

也可在 MSYS2 MINGW64 环境运行 `make` 和 `make test`，或使用 `build.bat`。新版 WinLibs 自动嵌入清单时优先使用 PowerShell 脚本，它只调整构建目录中的 specs 副本。

打包需要 Inno Setup 6.5.0 或更新版本，过程会下载并校验 HeadsetControl 4.3.0：

```powershell
powershell -ExecutionPolicy Bypass -File installer/build-installer.ps1 -Version 1.1.0-zh.1
```

GitHub Actions 在推送主分支或 PR 时构建、测试并生成安装版与便携版；推送 `v*` 标签时自动发布。发布页同时提供 HeadsetControl 对应源码，本项目源码可下载同一标签的 Source code 归档。

## 许可证与贡献

遵循 [GNU GPL v3.0 or later](LICENSE)。允许使用、修改与再分发；分发衍生版本时须遵守许可证并提供对应源码。第三方声明见 [NOTICE.md](NOTICE.md) 和 [THIRD_PARTY_NOTICES.txt](installer/THIRD_PARTY_NOTICES.txt)。

欢迎提交设备兼容反馈和适配 PR。驱动只应查询电量，不修改灯效、设置或固件；必须匹配已知设备并设置超时。详细开发约定见 [CONTRIBUTING.md](CONTRIBUTING.md)。

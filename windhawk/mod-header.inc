// ==WindhawkMod==
// @id              taskbar-wireless-battery-monitor
// @name            Taskbar Wireless Battery Monitor
// @name:zh-CN      任务栏无线设备电量监控
// @description     Show wireless peripheral battery levels on the taskbar or in the system tray.
// @description:zh-CN 显示多品牌无线外设电量，支持任务栏和托盘显示，以及可选的 AirPods 附近广播接收。
// @version         0.1.2
// @author          yaoiyiXu
// @github          https://github.com/yaoiyiXu
// @homepage        https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor
// @include         windhawk.exe
// @license         GPL-3.0-or-later
// @compilerOptions -lhid -lsetupapi -lcfgmgr32 -lole32 -lcomctl32 -lgdi32 -lruntimeobject -lshell32 -ladvapi32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Taskbar Wireless Battery Monitor

Display battery levels for supported wireless mice, keyboards, headsets and
controllers on the primary taskbar, or as one system tray icon per device.
Click the battery area to refresh or show/hide individual devices. Hover for details.
Configure the display mode, refresh interval, low-battery threshold, font size and
device spacing in Windhawk. Windhawk manages startup and enabling/disabling the mod.
The interface uses Chinese on Chinese Windows installations and English otherwise.

This tool runs in a dedicated Windhawk process, without injecting into Explorer.
Exit the standalone WirelessBattery.exe before enabling it. If the display does
not start, exit that app and disable/re-enable the mod.

## Device support and limitations

- Includes the native protocol drivers from [Wireless Battery](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor)
  and its Chinese adaptation. Brand support does not mean every model is supported.
  External HeadsetControl integration is not included.
- Vendor-specific battery queries send HID output/feature reports, or serial
  commands for supported ATK headsets, to peripherals and read their replies.
  These community-derived protocols have not been verified on every supported
  model. Some matches use vendor IDs and report sizes rather than exact model IDs.
  Queries can conflict with vendor utilities that access a device exclusively.
- Standard Bluetooth battery readings depend on what Windows exposes.
- Only the primary, horizontal taskbar is supported. On Windows 11 a crowded or
  left-aligned taskbar can overlap the widget; Windows 10 taskbar resizing can
  flicker. Switch to tray mode if the taskbar layout is unsuitable.
- In taskbar mode low battery is indicated by red text; notification balloons
  are available in tray mode. Sleep/last-known readings are dimmed.
- Disabling the mod waits for the active driver query to finish. Remaining
  drivers are skipped when stopping; an individual device timeout can still
  take several seconds.

## Optional AirPods reception

AirPods BLE reception is **off by default**. Enable it in settings if needed.
It passively receives nearby Apple proximity broadcasts while enabled, without
changing pairing or audio settings. Battery levels are approximate, in steps
of about 10%; open the charging case near the computer to obtain case readings.

Broadcast ownership cannot be verified. Entries are labelled "nearby broadcast"
and may belong to someone else, especially when several pairs of the same model
are nearby. Readings older than 30 seconds are marked as historical at the next
refresh, and entries expire after about two minutes without a broadcast.
Raw Bluetooth addresses and encrypted payloads are not written to disk.

## Credits and license

GNU GPL v3.0 or later. Based on [Wireless Battery](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor)
and the [Chinese / G87 adaptation](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor/pull/43).
AirPods protocol reference: [SpriteOvO/AirPodsDesktop](https://github.com/SpriteOvO/AirPodsDesktop)
(GPL-3.0-or-later). The dedicated host follows the official Windhawk "Mods as tools" example.
[Source and full notices](https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor).

## 中文说明

在主显示器任务栏或系统托盘显示受支持无线外设的电量。点击电量区域可刷新、
显示或隐藏设备，悬停可查看详情；显示模式、刷新周期等在 Windhawk 设置页调整。
中文 Windows 显示中文界面，其他语言系统默认显示英文。启用前请退出独立版
WirelessBattery.exe；启动和启停由 Windhawk 管理。

AirPods 广播接收默认关闭，可在设置中手动开启。电量约 10% 一档，充电盒读数
可能需要在电脑旁开盖。广播归属无法验证，可能显示附近其他人的耳机。
超过 30 秒未更新的数据在下一次刷新时标成历史，约两分钟无广播后移除。

厂商协议会向设备发送 HID 查询报文，部分 ATK 耳机使用串口命令。协议来自社区，
并未逐型号验证，部分驱动根据厂商 ID 和报文长度匹配设备；可能与独占设备的
厂商软件冲突。支持某品牌不代表支持其全部型号，不包含外部 HeadsetControl 扩展。

仅支持主显示器的横向任务栏；Windows 11 拥挤或左对齐时可能重叠，Windows 10
调整任务按钮区域时可能闪烁，可切换托盘模式。任务栏模式通过红字提示低电量，
托盘模式另有气泡通知。关闭时仍需等待当前设备查询结束，可能耗时数秒。
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- displayMode: taskbar
  $name: Display mode
  $name:zh-CN: 显示模式
  $options:
  - taskbar: Taskbar icons and battery levels
  - tray: One tray icon per device
  $options:zh-CN:
  - taskbar: 任务栏图标与电量
  - tray: 每个设备一个托盘图标
- pollSeconds: 60
  $name: Device refresh interval (seconds, 10–600)
  $name:zh-CN: 常规设备刷新周期（秒，10～600）
- warningPercent: 15
  $name: Low-battery threshold (0–50)
  $name:zh-CN: 低电量阈值（0～50）
- fontSize: 13
  $name: Battery font size (10–24)
  $name:zh-CN: 电量字号（10～24）
- deviceGap: 14
  $name: Device spacing (4–40)
  $name:zh-CN: 设备间距（4～40）
- hideSleeping: false
  $name: Hide sleeping devices and last-known readings
  $name:zh-CN: 隐藏休眠或仅有历史电量的设备
- enableAirPods: false
  $name: Receive nearby AirPods battery broadcasts
  $name:zh-CN: 接收附近 AirPods 电量广播
  $description: Approximate readings; ownership is unverified and nearby people's AirPods may appear.
  $description:zh-CN: 电量为近似值，广播归属未验证，可能显示附近其他人的耳机。
*/
// ==/WindhawkModSettings==

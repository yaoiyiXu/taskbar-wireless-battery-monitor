// ==WindhawkMod==
// @id              taskbar-wireless-battery-monitor
// @name            任务栏无线设备电量监控
// @description     显示多品牌无线外设与 AirPods 附近广播电量，在独立宿主运行。
// @version         0.1.1
// @author          yaoiyiXu
// @github          https://github.com/yaoiyiXu
// @homepage        https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor
// @include         windhawk.exe
// @license         GPL-3.0-or-later
// @compilerOptions -lhid -lsetupapi -lcfgmgr32 -lole32 -lcomctl32 -lgdi32 -lruntimeobject -lshell32 -ladvapi32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# 任务栏无线设备电量监控 · 本地测试版

基于 Wireless Battery 和本项目中文版，同一套原生厂商驱动与 AirPods BLE 解析。
工具型模组运行在独立 Windhawk 宿主中，不向 explorer.exe 注入。

**启用前请先退出独立版 WirelessBattery.exe，避免同时占用任务栏。**

默认在任务栏右侧显示图标和电量。点击可隐藏设备或立即刷新；悬停可看详情。
显示模式、刷新周期、低电量阈值、字号及 AirPods 接收在 Windhawk 设置页调整。
开机启动和启停由 Windhawk 管理。

AirPods 电量约 10% 一档。充电盒读数可能需要在电脑旁开盖。
广播归属无法验证，明确显示“附近广播”；同型号附近有多组时可能读到其他设备。
30 秒以上未更新的数据在下一次刷新时标成历史，约两分钟无广播后移除。

此测试版包含所有现有原生协议驱动；暂不调用外部 HeadsetControl 扩展。
包含 x86 和 x64 宿主支持；多显示器、任务栏布局与其他模组的组合需要验证。

来源与许可证：GNU GPL v3.0 or later。
上游：https://github.com/krisgfx9/Wireless-Device-Battery-Monitor
中文及 G87：https://github.com/krisgfx9/Wireless-Device-Battery-Monitor/pull/43
AirPods 协议参考：SpriteOvO/AirPodsDesktop (GPL-3.0-or-later)。
工具宿主适配参考 Windhawk 官方 “Mods as tools” 文档。
完整源代码及声明：https://github.com/yaoiyiXu/taskbar-wireless-battery-monitor
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- displayMode: taskbar
  $name: 显示模式
  $options:
  - taskbar: 任务栏图标与电量
  - tray: 每个设备一个托盘图标
- pollSeconds: 60
  $name: 常规设备刷新周期（秒，10～600）
- warningPercent: 15
  $name: 低电量阈值（0～50）
- fontSize: 13
  $name: 电量字号（10～24）
- deviceGap: 14
  $name: 设备间距（4～40）
- hideSleeping: false
  $name: 隐藏休眠或仅有历史电量的设备
- enableAirPods: true
  $name: 接收附近 AirPods 电量广播
*/
// ==/WindhawkModSettings==

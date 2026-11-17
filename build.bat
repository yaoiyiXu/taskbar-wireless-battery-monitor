@echo off
rem Builds WirelessBattery.exe with MinGW-w64 (g++ and windres on PATH).
setlocal
cd /d "%~dp0"
if not exist build mkdir build
windres -i res\app.rc -o build\app_res.o --include-dir res || exit /b 1
g++ -finput-charset=UTF-8 -fexec-charset=UTF-8 -std=c++17 -Os -municode -mwindows -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections ^
    -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -Wall -Wno-missing-field-initializers ^
    src\main.cpp src\devices.cpp src\widget.cpp src\hid.cpp ^
    src\drivers\mchose.cpp src\drivers\airpods.cpp ^
    src\drivers\glorious.cpp src\drivers\aula.cpp src\drivers\kysona.cpp src\drivers\atk_serial.cpp src\drivers\keychron.cpp src\drivers\lamzu.cpp src\drivers\rongyuan.cpp src\drivers\sony_inzone.cpp src\drivers\rog.cpp src\drivers\zowie.cpp src\drivers\misc_mice.cpp src\drivers\razer_headset.cpp src\drivers\hyperx.cpp src\drivers\logitech.cpp src\drivers\razer.cpp ^
    src\drivers\steelseries.cpp src\drivers\corsair.cpp src\drivers\coolermaster.cpp src\drivers\playstation.cpp src\drivers\gamesir.cpp src\drivers\xinput.cpp src\drivers\rapoo.cpp ^
    src\drivers\generic.cpp src\drivers\headsetcontrol.cpp ^
    build\app_res.o -o WirelessBattery.exe ^
    -lhid -lsetupapi -lcfgmgr32 -lole32 -lcomctl32 -lgdi32 -lruntimeobject || exit /b 1
echo Built WirelessBattery.exe

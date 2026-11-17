# Windows/WinLibs build, including toolchains that automatically inject a manifest.
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
New-Item -ItemType Directory -Force build | Out-Null
$extraFlags = @()
$specs = (& g++ -dumpspecs) -join "`n"
$defaultManifest = '%{!shared:%:if-exists(default-manifest.o%s)}'
if ($specs.Contains($defaultManifest)) {
    # The app supplies its own DPI-aware manifest. Keep the toolchain untouched.
    $specs.Replace($defaultManifest, '') | Set-Content build/local.specs -Encoding ascii
    $extraFlags += '-specs=build/local.specs'
}
& windres -i res/app.rc -o build/app_res.o --include-dir res
if ($LASTEXITCODE) { throw 'Resource compilation failed' }
$sources = @(Get-ChildItem src/*.cpp, src/drivers/*.cpp | ForEach-Object FullName)
& g++ @extraFlags -finput-charset=UTF-8 -fexec-charset=UTF-8 -std=c++17 -Os -municode -mwindows -static -s -ffunction-sections -fdata-sections '-Wl,--gc-sections' -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -Wall -Wno-missing-field-initializers @sources build/app_res.o -o WirelessBattery.exe -lhid -lsetupapi -lcfgmgr32 -lole32 -lcomctl32 -lgdi32 -lruntimeobject
if ($LASTEXITCODE) { throw 'Compilation failed' }
Write-Host 'Built WirelessBattery.exe'

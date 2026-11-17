param(
    [Parameter(Mandatory = $true)][string]$WindhawkDir,
    [switch]$BuildProbe
)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
& python scripts/build-windhawk.py
if ($LASTEXITCODE) { throw 'Mod generation failed' }
$compiler = Join-Path $WindhawkDir 'Compiler\bin\clang++.exe'
$includes = Join-Path $WindhawkDir 'Compiler\include'
$engineVersion = (Get-Content (Join-Path $WindhawkDir 'windhawk.ini') | Where-Object { $_ -like 'EnginePath=*' }) -replace '^EnginePath=', ''
$flags = @('-std=c++23', '-target', 'x86_64-w64-mingw32', '-DUNICODE', '-D_UNICODE',
    '-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0A00', '-DNTDDI_VERSION=0x0A000008',
    '-D__USE_MINGW_ANSI_STDIO=0', '-DWH_MOD', '-include', 'windhawk_api.h', '-I', $includes, '-O2')
$libs = @('-lhid', '-lsetupapi', '-lcfgmgr32', '-lole32', '-lcomctl32', '-lgdi32', '-lruntimeobject', '-lshell32', '-ladvapi32')
New-Item -ItemType Directory -Path build -Force | Out-Null
$sourceText = Get-Content -LiteralPath windhawk/taskbar-wireless-battery-monitor.wh.cpp -Raw
$version = [regex]::Match($sourceText, '@version\s+(\S+)').Groups[1].Value
foreach ($arch in @('32', '64')) {
    $target = if ($arch -eq '32') { 'i686-w64-mingw32' } else { 'x86_64-w64-mingw32' }
    $library = Join-Path $WindhawkDir ($engineVersion + '\' + $arch + '\windhawk.lib')
    & $compiler @flags -target $target '-DWH_MOD_ID=L"local@taskbar-wireless-battery-monitor"' ('-DWH_MOD_VERSION=L"' + $version + '"') -shared windhawk/taskbar-wireless-battery-monitor.wh.cpp $library '-Wl,--export-all-symbols' @libs -o ("build/windhawk-battery-$arch.dll")
    if ($LASTEXITCODE) { throw "Windhawk $arch-bit DLL build failed" }
    Write-Host "Windhawk $arch-bit DLL compilation and link passed."
}
if ($BuildProbe) {
    & $compiler @flags -DWH_EDITING -municode tests/windhawk_probe.cpp @libs -o build/windhawk-probe.exe
    if ($LASTEXITCODE) { throw 'Windhawk probe build failed' }
    $runtime = Join-Path $WindhawkDir 'Compiler\x86_64-w64-mingw32\bin'
    Copy-Item -LiteralPath (Join-Path $runtime 'libc++.dll') -Destination 'build\libc++.whl' -Force
    Copy-Item -LiteralPath (Join-Path $runtime 'libunwind.dll') -Destination 'build\libunwind.whl' -Force
    Write-Host 'Probe executable built. Run with Windhawk Compiler bin directories on PATH.'
}

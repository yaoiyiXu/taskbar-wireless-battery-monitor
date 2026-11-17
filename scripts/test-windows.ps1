# Run existing protocol and driver regression tests without an MSYS shell.
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
New-Item -ItemType Directory -Force build | Out-Null
$flags = @('-std=c++17', '-O2', '-Wall', '-static', '-finput-charset=UTF-8', '-fexec-charset=UTF-8', '-D_WIN32_WINNT=0x0A00', '-DWINVER=0x0A00')
$tests = @(
    @{ Name = 'airpods_protocol'; Sources = @('tests/airpods_protocol_test.cpp') },
    @{ Name = 'rapoo_protocol'; Sources = @('tests/rapoo_protocol_test.cpp') },
    @{ Name = 'rapoo_driver'; Sources = @('tests/rapoo_driver_test.cpp', 'src/drivers/rapoo.cpp') },
    @{ Name = 'mchose_driver'; Sources = @('tests/mchose_driver_test.cpp', 'src/drivers/mchose.cpp') }
)
foreach ($test in $tests) {
    $target = Join-Path 'build' ($test.Name + '_test.exe')
    & g++ @flags @($test.Sources) -o $target
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $($test.Name)" }
    & (Join-Path $PWD $target)
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $($test.Name)" }
}
Write-Host 'All protocol and driver tests passed.'

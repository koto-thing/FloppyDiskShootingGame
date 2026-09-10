# GPUやコントローラー実機なしで入力の回帰テストを実行する
param([switch]$Hardware)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools not found' }
$msvc = Get-ChildItem "$vs/VC/Tools/MSVC" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$sdk = "${env:ProgramFiles(x86)}/Windows Kits/10"
$version = Get-ChildItem "$sdk/Include" -Directory | Where-Object { Test-Path "$($_.FullName)/cppwinrt" } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$msvc -or !$version) { throw 'MSVC or Windows SDK with C++/WinRT not found' }
$output = Join-Path $repo 'Temp/Switch2InputCheck'
New-Item -ItemType Directory -Force $output | Out-Null

# このプロセスだけにSDKのパスを設定する
$env:INCLUDE = "$($msvc.FullName)/include;$($version.FullName)/ucrt;$($version.FullName)/shared;$($version.FullName)/um;$($version.FullName)/cppwinrt;$($version.FullName)/winrt"
$env:LIB = "$($msvc.FullName)/lib/x64;$sdk/Lib/$($version.Name)/ucrt/x64;$sdk/Lib/$($version.Name)/um/x64"
Set-Content -LiteralPath "$output/main.cpp" -Encoding utf8 -Value @'
#include <cstdio>
#include <exception>
void RunInputTests();
/** @brief 入力テストを実行する @return 成功時0、失敗時1 */
int main() {
    try { RunInputTests(); std::puts("Input tests passed"); return 0; }
    catch (const std::exception& e) { std::puts(e.what()); return 1; }
}
'@
$testMain = if ($Hardware) { "$repo/Tests/NintendoProConnectionCheck.cpp" } else { "$output/main.cpp" }
& "$($msvc.FullName)/bin/Hostx64/x64/cl.exe" /nologo /std:c++20 /EHsc /MT /utf-8 /W4 "/Fo$output/" "/Fe$output/InputTests.exe" $testMain "$repo/Tests/InputTests.cpp" "$repo/Engine/Input/Input.cpp" "$repo/Engine/Input/WindowsInputBackend.cpp" "$repo/Engine/Input/Switch2ProInput.cpp" /link user32.lib
if ($LASTEXITCODE) { throw 'Input test build failed' }
& "$output/InputTests.exe"
if ($LASTEXITCODE) { throw 'Input tests failed' }

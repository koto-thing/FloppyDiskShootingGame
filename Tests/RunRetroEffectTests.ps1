# 実際のレトロシェーダーをWARPで描画して検査する
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools not found' }
$msvc = Get-ChildItem "$vs/VC/Tools/MSVC" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$sdk = "${env:ProgramFiles(x86)}/Windows Kits/10"
$version = Get-ChildItem "$sdk/Include" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$output = Join-Path $repo 'obj/RetroEffectTests'
New-Item -ItemType Directory -Force $output | Out-Null
$env:INCLUDE = "$($msvc.FullName)/include;$($version.FullName)/ucrt;$($version.FullName)/shared;$($version.FullName)/um;$($version.FullName)/winrt"
$env:LIB = "$($msvc.FullName)/lib/x64;$sdk/Lib/$($version.Name)/ucrt/x64;$sdk/Lib/$($version.Name)/um/x64"
Push-Location $repo
try {
    & "$($msvc.FullName)/bin/Hostx64/x64/cl.exe" /nologo /std:c++20 /EHsc /MT /utf-8 /O1 /UNDEBUG "/Fo$output/" "/Fe$output/RetroEffectTests.exe" Tests/RetroEffectTests.cpp /link d3d11.lib d3dcompiler.lib
    if ($LASTEXITCODE) { throw 'Retro effect test build failed' }
    & "$output/RetroEffectTests.exe"
    if ($LASTEXITCODE) { throw 'Retro effect tests failed' }
} finally { Pop-Location }

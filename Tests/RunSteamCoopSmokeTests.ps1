<#
.SYNOPSIS
Steam SDKの実通信で単独ロビーを検証する（フレンド招待は送らない）
#>
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio not found' }
$properties = & "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyDiskShootingGame.vcxproj" /nologo /p:Configuration=SteamRelease /p:Platform=x64 -getProperty:SteamworksSdkRoot,SteamLibrary,SteamRuntime,SteamAppId,IntDir
if ($LASTEXITCODE) { throw 'Steam build property evaluation failed' }
$settings = ($properties -join "`n" | ConvertFrom-Json).Properties
if (!(Test-Path "$($settings.IntDir)/BuildVersion.h")) { throw 'Build SteamRelease first' }
$output = Join-Path $repo 'obj/SteamCoopSmokeTests'
$null = New-Item -ItemType Directory -Force -Path $output
$arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/utf-8', '/UNDEBUG',
    "/DSPACEYAKUZA_STEAM_APP_ID=$($settings.SteamAppId)",
    ('/I"' + $settings.SteamworksSdkRoot + '/public"'), ('/I"' + $settings.IntDir + '."'),
    ('/Fo"' + $output + '/"'), ('/Fe"' + $output + '/SteamCoopSmokeTests.exe"'),
    ('"' + $PSScriptRoot + '/SteamCoopSmokeTests.cpp"'),
    ('"' + $repo + '/Infrastructure/ExternalServices/SteamCoopSession.cpp"'),
    '/link', ('"' + $settings.SteamLibrary + '"'))
$response = Join-Path $output 'compile.rsp'
Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
cmd /c "call `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul && cl.exe @`"$response`""
if ($LASTEXITCODE) { throw 'Steam smoke test compilation failed' }
Copy-Item -LiteralPath $settings.SteamRuntime -Destination $output -Force
& "$output/SteamCoopSmokeTests.exe"
if ($LASTEXITCODE -eq 2) { throw 'Steam is unavailable; sign into Steam and rerun this test' }
if ($LASTEXITCODE) { throw 'Steam lobby smoke test failed' }

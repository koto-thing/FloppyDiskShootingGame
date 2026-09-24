# OnlineReleaseの実シーンで翻訳後の中央配置を検査する
param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools not found' }
if (!$SkipBuild) {
    & "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyDiskShootingGame.vcxproj" /p:Configuration=OnlineRelease /p:Platform=x64 /m /v:minimal /nologo
    if ($LASTEXITCODE) { throw 'OnlineRelease build failed' }
}
$objects = Join-Path $repo 'obj/Online/x64/OnlineRelease/FloppyDiskShootingGame'
$engine = Join-Path $repo 'bin/Online/x64/OnlineRelease/FloppyEngine.lib'
if (!(Test-Path "$objects/OptionScene.obj") -or !(Test-Path $engine)) {
    throw 'Build OnlineRelease before using -SkipBuild'
}
$output = Join-Path $repo 'obj/LocalizedLayoutTests'
New-Item -ItemType Directory -Force $output | Out-Null
$arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/O1', '/utf-8', '/UNDEBUG',
    '/DSPACEYAKUZA_EDITION_Online=1', ('/Fo"' + "$output/" + '"'),
    ('/Fe"' + "$output/LocalizedLayoutTests.exe" + '"'),
    ('"' + "$PSScriptRoot/LocalizedLayoutTests.cpp" + '"'))
$arguments += Get-ChildItem $objects -Filter '*.obj' | Where-Object Name -ne 'main.obj' | ForEach-Object { '"' + $_.FullName + '"' }
$arguments += '"' + $engine + '"'
$arguments += @('/link', '/LTCG', '/SUBSYSTEM:CONSOLE', 'd3d12.lib', 'dxgi.lib', 'd3dcompiler.lib',
    'user32.lib', 'gdi32.lib', 'shell32.lib', 'winhttp.lib')
$response = Join-Path $output 'LocalizedLayoutTests.rsp'
Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
cmd /c "call `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul && cl.exe @`"$response`""
if ($LASTEXITCODE) { throw 'Localized layout test link failed' }
& "$output/LocalizedLayoutTests.exe"
if ($LASTEXITCODE) { throw 'Localized layout test failed' }

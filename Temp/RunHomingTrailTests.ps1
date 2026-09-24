# Releaseオブジェクトへ小さなassertチェックをリンクして、GPUなしで実行する
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
$output = Join-Path $repo 'Temp/HomingLifetimeBuild'
$objects = Join-Path $repo 'Temp/HomingLaunchGameObj'
& "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyEngine.vcxproj" /p:Configuration=Release /p:Platform=x64 "/p:OutDir=$output/" "/p:IntDir=$repo/Temp/HomingLifetimeEngineObj/" /m /v:minimal /nologo
if ($LASTEXITCODE) { throw 'Engine build failed' }
& "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyDiskShootingGame.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:BuildProjectReferences=false "/p:DistributionDir=$repo/dist/Floppy/x64/HomingTrailPreview/" "/p:OutDir=$output/" "/p:IntDir=$objects/" /m /v:minimal /nologo
if ($LASTEXITCODE) { throw 'Release build failed' }

# 開発環境のINCLUDE/LIB/PATHを現在のプロセスだけに取り込む
cmd /c "call `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
}
$response = Join-Path $output 'HomingShotTests.rsp'
$arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/utf-8', '/UNDEBUG',
    ('"' + "$repo/Tests/HomingShotTests.cpp" + '"'),
    ('/Fo"' + "$output/HomingShotTests.obj" + '"'),
    ('/Fe"' + "$output/HomingShotTests.exe" + '"'))
$arguments += Get-ChildItem $objects -Filter '*.obj' | Where-Object Name -ne 'main.obj' | ForEach-Object { '"' + $_.FullName + '"' }
$arguments += '"' + "$output/FloppyEngine.lib" + '"'
$arguments += @('/link', '/LTCG', '/SUBSYSTEM:CONSOLE', 'd3d12.lib', 'dxgi.lib', 'd3dcompiler.lib', 'user32.lib', 'gdi32.lib', 'shell32.lib')
Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
& cl.exe "@$response"
if ($LASTEXITCODE) { throw 'Homing test link failed' }
& "$output/HomingShotTests.exe"
if ($LASTEXITCODE) { throw 'Homing tests failed' }





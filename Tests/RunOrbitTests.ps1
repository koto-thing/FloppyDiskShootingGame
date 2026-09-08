# Releaseオブジェクトへ小さなassertチェックをリンクして、GPUなしで実行する
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
$output = Join-Path $repo 'Temp/OrbitBuild'
$objects = Join-Path $repo 'Temp/OrbitGameObj'
& "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyEngine.vcxproj" /p:Configuration=Release /p:Platform=x64 "/p:OutDir=$output/" "/p:IntDir=$repo/Temp/OrbitEngineObj/" /m /v:minimal /nologo
if ($LASTEXITCODE) { throw 'Engine build failed' }
& "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyDiskShootingGame.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:BuildProjectReferences=false "/p:OutDir=$output/" "/p:IntDir=$objects/" /m /v:minimal /nologo
if ($LASTEXITCODE) { throw 'Release build failed' }

# 開発環境のINCLUDE/LIB/PATHを現在のプロセスだけに取り込む
cmd /c "call `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
}
$response = Join-Path $output 'OrbitShotTests.rsp'
$arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/utf-8', '/UNDEBUG',
    ('"' + "$PSScriptRoot/OrbitShotTests.cpp" + '"'),
    ('/Fo"' + "$output/OrbitShotTests.obj" + '"'),
    ('/Fe"' + "$output/OrbitShotTests.exe" + '"'))
$arguments += Get-ChildItem $objects -Filter '*.obj' | Where-Object Name -ne 'main.obj' | ForEach-Object { '"' + $_.FullName + '"' }
$arguments += '"' + "$output/FloppyEngine.lib" + '"'
$arguments += @('/link', '/LTCG', '/SUBSYSTEM:CONSOLE', 'd3d12.lib', 'dxgi.lib', 'd3dcompiler.lib', 'user32.lib', 'gdi32.lib', 'shell32.lib')
Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
& cl.exe "@$response"
if ($LASTEXITCODE) { throw 'Orbit test link failed' }
& "$output/OrbitShotTests.exe"
if ($LASTEXITCODE) { throw 'Orbit tests failed' }


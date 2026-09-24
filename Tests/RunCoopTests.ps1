# FloppyReleaseの実オブジェクトで協力プレイと既存弾道の回帰チェックを実行する
param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools not found' }
if (!$SkipBuild) {
    & "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyDiskShootingGame.vcxproj" /p:Configuration=FloppyRelease /p:Platform=x64 /m /v:minimal /nologo
    if ($LASTEXITCODE) { throw 'FloppyRelease build failed' }
}
$objects = Join-Path $repo 'obj/Floppy/x64/FloppyRelease/FloppyDiskShootingGame'
$engine = Join-Path $repo 'bin/Floppy/x64/FloppyRelease/FloppyEngine.lib'
if (!(Test-Path "$objects/SideScrollingShooter.obj") -or !(Test-Path $engine)) {
    throw 'Build FloppyRelease before using -SkipBuild'
}
$output = Join-Path $repo 'obj/CoopTests'
New-Item -ItemType Directory -Force $output | Out-Null

$gameObjects = Get-ChildItem $objects -Filter '*.obj' | Where-Object Name -ne 'main.obj' | ForEach-Object { '"' + $_.FullName + '"' }
foreach ($test in @('CoopGameplayTests', 'CooperativeSetupTests', 'HomingShotTests', 'OrbitShotTests', 'ResumeCodeTests')) {
    # ゲームと同じ最適化で大型モデル状態の未最適化一時コピーによるスタック超過を防ぐ
    $arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/O1', '/utf-8', '/UNDEBUG',
        ('/Fo"' + "$output/" + '"'), ('/Fe"' + "$output/$test.exe" + '"'),
        ('"' + "$PSScriptRoot/$test.cpp" + '"'))
    if ($test -eq 'CoopGameplayTests') { $arguments += '"' + "$PSScriptRoot/CoopStageTests.cpp" + '"' }
    $arguments += $gameObjects
    $arguments += '"' + $engine + '"'
    $arguments += @('/link', '/LTCG', '/SUBSYSTEM:CONSOLE', 'd3d12.lib', 'dxgi.lib', 'd3dcompiler.lib', 'user32.lib', 'gdi32.lib', 'shell32.lib')
    $response = Join-Path $output "$test.rsp"
    Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
    # Developer Command Prompt内で完結させ、PowerShellの版によるPATH反映差を避ける
    cmd /c "call `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul && cl.exe @`"$response`""
    if ($LASTEXITCODE) { throw "$test link failed" }
    & "$output/$test.exe"
    if ($LASTEXITCODE) { throw "$test failed" }
}

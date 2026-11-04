param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools not found' }
$output = Join-Path $repo 'Temp/ModelCollisionBuild'
$objects = Join-Path $repo 'Temp/ModelCollisionGameObj'
New-Item -ItemType Directory -Force $output | Out-Null
Push-Location $repo
try {
    if (!$SkipBuild) {
        # 他の作業中ビルドとPDB・オブジェクトを共有しない
        foreach ($project in @('FloppyEngine', 'FloppyDiskShootingGame')) {
            $intermediate = if ($project -eq 'FloppyEngine') { 'Temp/ModelCollisionEngineObj/' } else { 'Temp/ModelCollisionGameObj/' }
            & "$vs/MSBuild/Current/Bin/MSBuild.exe" "$project.vcxproj" /p:Configuration=FloppyRelease /p:Platform=x64 "/p:IntDir=$intermediate" /p:OutDir=Temp/ModelCollisionBuild/ /p:BuildProjectReferences=false /m /v:quiet /nologo
            if ($LASTEXITCODE) { throw "$project build failed" }
        }
    }
    # 実ゲームと同じ最適化でリンクし、assertだけは有効にする
    $gameObjects = Get-ChildItem $objects -Filter '*.obj' | Where-Object Name -ne 'main.obj' | ForEach-Object { '"' + $_.FullName + '"' }
    foreach ($test in @('ModelShotCollisionTests', 'EnemyModelHitTests', 'HomingShotTests', 'OrbitShotTests', 'CoopGameplayTests')) {
        $arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/O1', '/utf-8', '/UNDEBUG',
            ('/Fo"' + "$output/" + '"'), ('/Fe"' + "$output/$test.exe" + '"'), ('"' + "$repo/Tests/$test.cpp" + '"'))
        if ($test -eq 'CoopGameplayTests') { $arguments += '"' + "$repo/Tests/CoopStageTests.cpp" + '"' }
        if ($test -ne 'ModelShotCollisionTests') {
            $arguments += $gameObjects
            $arguments += '"' + "$output/FloppyEngine.lib" + '"'
            $arguments += @('/link', '/LTCG', '/SUBSYSTEM:CONSOLE', 'd3d12.lib', 'dxgi.lib', 'd3dcompiler.lib', 'user32.lib', 'gdi32.lib', 'shell32.lib')
        }
        $response = Join-Path $output "$test.rsp"
        Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
        cmd /c "call `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul && cl.exe @`"$response`""
        if ($LASTEXITCODE) { throw "$test link failed" }
        & "$output/$test.exe"
        if ($LASTEXITCODE) { throw "$test failed" }
    }
} finally { Pop-Location }

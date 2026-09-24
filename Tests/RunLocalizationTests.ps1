# 翻訳カタログと永続化を各版の定義で検査する
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vsPath = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vsPath) { throw 'Visual Studio C++ tools not found' }
$output = Join-Path $repo 'obj/LocalizationTests'
New-Item -ItemType Directory -Force $output | Out-Null
Push-Location $repo
try {
    foreach ($edition in @('Floppy', 'Online', 'Steam')) {
        $sources = @('Tests/LocalizationTests.cpp')
        if ($edition -ne 'Floppy') {
            $sources += @('Application/UseCases/Localization.cpp', 'Infrastructure/Repositories/SettingsRepository.cpp')
        }
        $arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/O1', '/utf-8', '/UNDEBUG',
            "/DSPACEYAKUZA_EDITION_$edition=1", ('/Fo"' + "$output/" + '"'),
            ('/Fe"' + "$output/$edition.exe" + '"'))
        $arguments += $sources
        $arguments += @('/link', 'user32.lib')
        $response = Join-Path $output "$edition.rsp"
        Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
        cmd /c "call `"$vsPath/VC/Auxiliary/Build/vcvars64.bat`" >nul && cl.exe @`"$response`""
        if ($LASTEXITCODE) { throw "$edition localization compile failed" }
        & "$output/$edition.exe"
        if ($LASTEXITCODE) { throw "$edition localization test failed" }

        # 配布ビルドの入力に翻訳コードとフォントが含まれる条件を検査する
        $result = & "$vsPath/MSBuild/Current/Bin/MSBuild.exe" FloppyDiskShootingGame.vcxproj /nologo /p:Configuration=Release /p:Platform=x64 "/p:Distribution=$edition" -getItem:ClCompile,ResourceCompile,DistributionFile
        if ($LASTEXITCODE) { throw 'Distribution evaluation failed' }
        $items = ($result -join "`n" | ConvertFrom-Json).Items
        $expected = $edition -ne 'Floppy'
        if (($items.ClCompile.Identity -contains 'Application\UseCases\Localization.cpp') -ne $expected) { throw 'Catalog edition isolation failed' }
        if (($items.ResourceCompile.Identity -contains 'Resources\UnicodeFont.rc') -ne $expected) { throw 'Font edition isolation failed' }
        if (($items.DistributionFile.Identity -contains 'Resources\UnicodeFont-LICENSE.txt') -ne $expected) { throw 'License edition isolation failed' }
    }
} finally {
    Pop-Location
}

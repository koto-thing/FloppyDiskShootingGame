<#
.SYNOPSIS
配布版の出力分離・入力検証・容量制限を検査する
#>
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio not found' }
$msbuild = "$vs/MSBuild/Current/Bin/MSBuild.exe"

# ビルドせずに各版の出力先と既定値を検証する
foreach ($edition in @('', 'Floppy', 'Online', 'Steam')) {
    $arguments = @("$repo/FloppyDiskShootingGame.vcxproj", '/nologo', '/p:Configuration=Release', '/p:Platform=x64', '-getProperty:Distribution,OutDir,IntDir,DistributionDir', '-getItem:ClCompile')
    if ($edition) { $arguments += "/p:Distribution=$edition" }
    $result = & $msbuild @arguments
    if ($LASTEXITCODE) { throw "Property evaluation failed: $edition" }
    $evaluated = $result -join "`n" | ConvertFrom-Json
    $properties = $evaluated.Properties
    $expected = if ($edition) { $edition } else { 'Floppy' }
    if ($properties.Distribution -ne $expected) { throw 'Incorrect default distribution' }
    $defines = $evaluated.Items.ClCompile[0].PreprocessorDefinitions.Split(';')
    if ($defines -notcontains "SPACEYAKUZA_EDITION_$expected=1" -or $defines -notcontains 'NDEBUG') { throw 'Missing edition or Release definition' }
    foreach ($name in @('OutDir', 'IntDir', 'DistributionDir')) {
        if (!$properties.$name.Contains("\$expected\x64\Release\")) { throw "Output not isolated: $name" }
    }
}
& $msbuild "$repo/FloppyDiskShootingGame.vcxproj" /nologo /v:quiet /t:ValidateDistribution /p:Distribution=Invalid
if (!$LASTEXITCODE) { throw 'Invalid distribution was accepted' }

# IDEの6構成を全プロジェクトで評価し、最適化・出力先・版を検証する
foreach ($edition in @('Floppy', 'Online', 'Steam')) {
    foreach ($flavor in @('Debug', 'Release')) {
        $configuration = "$edition$flavor"
        foreach ($format in @('sln', 'slnx')) {
            & $msbuild "$repo/FloppyDiskShootingGame.$format" /nologo /v:quiet /t:ValidateSolutionConfiguration "/p:Configuration=$configuration" /p:Platform=x64
            if ($LASTEXITCODE) { throw "Solution configuration missing: $configuration ($format)" }
        }
        foreach ($project in @('FloppyDiskShootingGame', 'FloppyEngine', 'FloppyEngine.Tests')) {
            $result = & $msbuild "$repo/$project.vcxproj" /nologo "/p:Configuration=$configuration" /p:Platform=x64 -getProperty:Distribution,BuildFlavor,OutDir,IntDir,ConfigurationType,UseDebugLibraries -getItem:ClCompile,ProjectConfiguration
            if ($LASTEXITCODE) { throw "IDE property evaluation failed: $project $configuration" }
            $evaluated = $result -join "`n" | ConvertFrom-Json
            $properties = $evaluated.Properties
            if ($properties.Distribution -ne $edition -or $properties.BuildFlavor -ne $flavor -or !$properties.ConfigurationType) { throw "Incorrect IDE configuration: $project $configuration" }
            if ($properties.UseDebugLibraries -ne ($flavor -eq 'Debug').ToString()) { throw 'Incorrect debug library setting' }
            if ($evaluated.Items.ProjectConfiguration.Identity -notcontains "$configuration|x64") { throw 'IDE project configuration missing' }
            foreach ($name in @('OutDir', 'IntDir')) {
                if (!$properties.$name.Contains("\$edition\x64\$configuration\")) { throw "IDE output not isolated: $name" }
            }
            if ($evaluated.Items.ClCompile[0].PreprocessorDefinitions.Split(';') -notcontains "SPACEYAKUZA_EDITION_$edition=1") { throw 'IDE edition definition missing' }
        }
    }
}
& $msbuild "$repo/FloppyDiskShootingGame.vcxproj" /nologo /v:quiet /t:ValidateDistribution /p:Configuration=SteamRelease /p:Distribution=Floppy
if (!$LASTEXITCODE) { throw 'Conflicting edition was accepted' }

# 上限ちょうどを許可し、別ファイルを含む1バイト超過を拒否する
$fixture = Join-Path $repo "obj/DistributionCheck/$([guid]::NewGuid())"
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
[IO.File]::WriteAllBytes("$fixture/game.exe", [byte[]]::new(1474560))
& "$repo/Scripts/CheckDistributionSize.ps1" -Path $fixture
[IO.File]::WriteAllBytes("$fixture/resource.dat", [byte[]]::new(1))
$rejected = $false
try { & "$repo/Scripts/CheckDistributionSize.ps1" -Path $fixture }
catch {
    if ($_.Exception.Message -notlike '*exceeds 1.44MB*') { throw }
    $rejected = $true
}
if (!$rejected) { throw 'Oversized distribution was accepted' }
Write-Host 'Distribution tests passed'

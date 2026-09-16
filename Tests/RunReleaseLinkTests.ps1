<#
.SYNOPSIS
中間オブジェクトを再利用する連続ReleaseビルドでLNK1143の再発を検証する
.PARAMETER Distribution
検証する配布版
#>
param([ValidateSet('Floppy', 'Online', 'Steam')][string]$Distribution = 'Steam')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio not found' }
$configuration = "${Distribution}Release"
$object = Join-Path $repo "obj/$Distribution/x64/$configuration/FloppyDiskShootingGame/TextRenderingService.obj"
$executable = Join-Path $repo "bin/$Distribution/x64/$configuration/SpaceYakuza.exe"

# Cleanせずに2回リンクし、問題のオブジェクトをそのまま再利用する
for ($run = 1; $run -le 2; ++$run) {
    & "$vs/MSBuild/Current/Bin/MSBuild.exe" "$repo/FloppyDiskShootingGame.vcxproj" /m /nologo /v:minimal "/p:Configuration=$configuration" /p:Platform=x64
    if ($LASTEXITCODE) { throw "$configuration build $run failed" }
    if (!(Test-Path -LiteralPath $executable)) { throw 'Executable missing' }
    if ($run -eq 1) {
        $objectHash = (Get-FileHash -LiteralPath $object).Hash
        $objectTime = (Get-Item -LiteralPath $object).LastWriteTimeUtc
        $exeHash = (Get-FileHash -LiteralPath $executable).Hash
    }
}
if ((Get-FileHash -LiteralPath $object).Hash -ne $objectHash -or
    (Get-Item -LiteralPath $object).LastWriteTimeUtc -ne $objectTime) { throw 'Object was rebuilt; incremental regression was not exercised' }
if ((Get-FileHash -LiteralPath $executable).Hash -eq $exeHash) { throw 'Executable was not relinked with the next build number' }
Write-Host "$configuration consecutive link tests passed"

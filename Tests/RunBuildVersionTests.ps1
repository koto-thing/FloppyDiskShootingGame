# 採番、版の独立性、中間生成物の再作成、桁上がり、破損検出を検証する
$ErrorActionPreference = 'Stop'
$generator = Join-Path $PSScriptRoot '../Scripts/GenerateBuildVersion.ps1'
$root = Join-Path $PSScriptRoot "../obj/BuildVersionTests/$([guid]::NewGuid())"
$counters = Join-Path $root 'counters'
$header = Join-Path $root 'obj/BuildVersion.h'
foreach ($case in @(@('Floppy', '001'), @('Steam', '001'), @('Online', '001'), @('Floppy', '002'))) {
    $edition, $expected = $case
    & $generator -Distribution $edition -CounterDirectory $counters -HeaderPath $header
    if (![IO.File]::ReadAllText($header).Contains("$edition v$expected")) { throw 'Incorrect build label' }
    Remove-Item -LiteralPath $header
}
[IO.File]::WriteAllText("$counters/Floppy.txt", '999')
& $generator -Distribution Floppy -CounterDirectory $counters -HeaderPath $header
if (![IO.File]::ReadAllText($header).Contains('Floppy v1000')) { throw 'Counter overflow at 1000' }
[IO.File]::WriteAllText("$counters/Floppy.txt", 'broken')
$rejected = $false
try { & $generator -Distribution Floppy -CounterDirectory $counters -HeaderPath $header } catch { $rejected = $true }
if (!$rejected -or [IO.File]::ReadAllText("$counters/Floppy.txt") -ne 'broken') { throw 'Corrupt counter was reset' }
Write-Host 'Build version tests passed'

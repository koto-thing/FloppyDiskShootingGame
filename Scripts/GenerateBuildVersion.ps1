<#
.SYNOPSIS
版ごとのビルド番号を保存し、タイトル表示用ヘッダーを生成する
.DESCRIPTION
同じ作業フォルダー内では構成とCPU種別をまたいで版ごとに採番する
番号はローカルの.buildnumbersに保存するため、別PCへの移行時は同フォルダーもコピーする
ビルド失敗時は欠番とし、CleanやRebuildでは番号をリセットしない
.PARAMETER Distribution
配布版
.PARAMETER CounterDirectory
Cleanで削除しない採番保存先
.PARAMETER HeaderPath
中間フォルダー内の生成ヘッダー
#>
param(
    [Parameter(Mandatory = $true)][ValidateSet('Floppy', 'Online', 'Steam')][string]$Distribution,
    [Parameter(Mandatory = $true)][string]$CounterDirectory,
    [Parameter(Mandatory = $true)][string]$HeaderPath
)
$ErrorActionPreference = 'Stop'

# 同じ版の同時採番は排他し、競合時は番号を重複させずビルドを失敗させる
$null = New-Item -ItemType Directory -Force -Path $CounterDirectory
$counterPath = Join-Path $CounterDirectory "$Distribution.txt"
$lock = [IO.File]::Open("$counterPath.lock", 'OpenOrCreate', 'ReadWrite', 'None')
try {
    # 既存値が破損している場合はリセットせず停止する
    [long]$number = 0
    if (Test-Path -LiteralPath $counterPath) {
        $saved = [IO.File]::ReadAllText($counterPath).Trim()
        if (![long]::TryParse($saved, [ref]$number) -or $number -lt 0 -or $number -eq [long]::MaxValue) {
            throw "Invalid build counter: $counterPath"
        }
    }
    $number++

    # 採番を先に確定し、ビルド失敗後も番号を再利用しない
    [IO.File]::WriteAllText("$counterPath.tmp", $number.ToString())
    if (Test-Path -LiteralPath $counterPath) {
        [IO.File]::Replace("$counterPath.tmp", $counterPath, "$counterPath.bak")
    } else {
        [IO.File]::Move("$counterPath.tmp", $counterPath)
    }

    # 最低3桁で表示し、1000以降も桁を切り捨てない
    $label = '{0} v{1:D3}' -f $Distribution, $number
    $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $HeaderPath)
    [IO.File]::WriteAllText($HeaderPath, "#pragma once`r`n#define SPACEYAKUZA_BUILD_LABEL `"$label`"`r`n")
    Write-Host "Build version: $label"
} finally {
    $lock.Dispose()
}

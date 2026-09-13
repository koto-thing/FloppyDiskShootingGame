<#
.SYNOPSIS
Floppy版の配布フォルダー全体が1.44MB以内か検査する
.PARAMETER Path
検査する配布フォルダー
#>
param([Parameter(Mandatory = $true)][string]$Path)
$ErrorActionPreference = 'Stop'

# 隠しファイルを含む配布物全体を集計する
if (!(Test-Path -LiteralPath $Path -PathType Container)) { throw "Distribution directory not found: $Path" }
$files = @(Get-ChildItem -LiteralPath $Path -File -Recurse -Force)
if (!$files.Count) { throw "Distribution directory is empty: $Path" }
$bytes = ($files | Measure-Object -Property Length -Sum).Sum
if ($bytes -gt 1474560) { throw "Floppy distribution exceeds 1.44MB: $bytes / 1474560 bytes" }
Write-Host "Floppy distribution: $bytes / 1474560 bytes"

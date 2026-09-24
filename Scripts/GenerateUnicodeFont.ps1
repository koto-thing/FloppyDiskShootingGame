<#
.SYNOPSIS
Online/Steam専用のUnicode BMPビットマップを公式Unifontから再生成する
.DESCRIPTION
通常のビルドでは実行不要、生成済みResources/UnicodeFont.binを使用する
各Unicode文字を16行のリトルエンディアン16ビット整数として保存する
#>
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$cache = Join-Path $repo 'obj/UnicodeFont'
New-Item -ItemType Directory -Force $cache | Out-Null
$source = Join-Path $cache 'unifont-16.0.02.hex.gz'
if (!(Test-Path $source)) {
    Invoke-WebRequest 'https://unifoundry.com/pub/unifont/unifont-16.0.02/font-builds/unifont-16.0.02.hex.gz' -OutFile $source
}
if ((Get-FileHash $source).Hash -ne 'B093C9D81275AAFB5E1FC0E62624B0665B8165C8759609D1F23187C81288CDFF') {
    throw 'Unifont source checksum mismatch'
}

# 全角16列と半角8列を共通の16列へ中央配置する
$bitmap = [byte[]]::new(65536 * 32)
$stream = [IO.Compression.GZipStream]::new([IO.File]::OpenRead($source), [IO.Compression.CompressionMode]::Decompress)
$reader = [IO.StreamReader]::new($stream)
try {
    while ($null -ne ($line = $reader.ReadLine())) {
        $parts = $line.Split(':')
        $codepoint = [Convert]::ToInt32($parts[0], 16)
        if ($codepoint -gt 65535) { continue }
        $digits = $parts[1].Length / 16
        if ($digits -ne 2 -and $digits -ne 4) { throw 'Unexpected glyph width' }
        for ($row = 0; $row -lt 16; ++$row) {
            $bits = [Convert]::ToUInt16($parts[1].Substring($row * $digits, $digits), 16)
            if ($digits -eq 2) { $bits = $bits -shl 4 }
            $bitmap[$codepoint * 32 + $row * 2] = $bits -band 255
            $bitmap[$codepoint * 32 + $row * 2 + 1] = $bits -shr 8
        }
    }
} finally { $reader.Dispose() }
[IO.File]::WriteAllBytes((Join-Path $repo 'Resources/UnicodeFont.bin'), $bitmap)
Write-Host 'Generated UnicodeFont.bin (65536 glyphs, 2097152 bytes)'

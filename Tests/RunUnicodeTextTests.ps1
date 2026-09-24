# 翻訳後の文字計測、UTF-8境界、同梱フォント、D3D12シェーダーを検査する
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools not found' }
$msvc = Get-ChildItem "$vs/VC/Tools/MSVC" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$sdk = "${env:ProgramFiles(x86)}/Windows Kits/10"
$version = Get-ChildItem "$sdk/Include" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$output = Join-Path $repo 'obj/UnicodeTextTests'
New-Item -ItemType Directory -Force $output | Out-Null
$env:INCLUDE = "$($msvc.FullName)/include;$($version.FullName)/ucrt;$($version.FullName)/shared;$($version.FullName)/um;$($version.FullName)/winrt"
$env:LIB = "$($msvc.FullName)/lib/x64;$sdk/Lib/$($version.Name)/ucrt/x64;$sdk/Lib/$($version.Name)/um/x64"

# 実際のゲームと同じリソースをテスト実行ファイルへ埋め込む
Push-Location $repo
try {
    & "$sdk/bin/$($version.Name)/x64/rc.exe" /nologo "/fo$output/UnicodeFont.res" 'Resources/UnicodeFont.rc'
    if ($LASTEXITCODE) { throw 'Unicode font resource build failed' }
    $sources = @('Tests/UnicodeTextTests.cpp', 'Engine/Graphics/Renderer.cpp', 'Engine/UI/Button.cpp',
        'Engine/Graphics/Camera2D.cpp', 'Engine/Graphics/Camera3D.cpp', 'Engine/Geometry/Rect.cpp',
        'Engine/Geometry/Ray.cpp', 'Engine/Geometry/Box.cpp', 'Engine/Geometry/Circle.cpp',
        'Infrastructure/ExternalServices/TextRenderingService.cpp', 'Engine/Diagnostics/Debug.cpp')
    & "$($msvc.FullName)/bin/Hostx64/x64/cl.exe" /nologo /std:c++20 /EHsc /MT /utf-8 /O1 /UNDEBUG /DUNICODE /D_UNICODE /DSPACEYAKUZA_EDITION_Online=1 "/Fo$output/" "/Fe$output/UnicodeTextTests.exe" @sources "$output/UnicodeFont.res" /link d3d12.lib dxgi.lib d3dcompiler.lib user32.lib
    if ($LASTEXITCODE) { throw 'Unicode text test build failed' }
    & "$output/UnicodeTextTests.exe"
    if ($LASTEXITCODE) { throw 'Unicode text tests failed' }
} finally { Pop-Location }


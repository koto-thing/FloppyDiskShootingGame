<#
.SYNOPSIS
教材のGoサーバーとC++クライアントを実通信で検証する（Server配下は変更しない）
#>
param([string]$GoExe = 'go')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$output = Join-Path $repo ('obj/OnlineTests/' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Force -Path $output
$goCommand = (Get-Command $GoExe -ErrorAction Stop).Source
$tutorial = Get-Content -LiteralPath "$repo/Docs/OnlineServerTutorial.md" -Raw -Encoding UTF8
$blocks = [regex]::Matches($tutorial, '(?s)```go\r?\n(.*?)\r?\n```')
if ($blocks.Count -ne 2) { throw 'Expected main.go and main_test.go tutorial blocks' }
$utf8 = New-Object System.Text.UTF8Encoding($false)
[IO.File]::WriteAllText("$output/main.go", $blocks[0].Groups[1].Value, $utf8)
[IO.File]::WriteAllText("$output/main_test.go", $blocks[1].Groups[1].Value, $utf8)
[IO.File]::WriteAllText("$output/go.mod", "module spaceyakuza/server`ngo 1.22`n", $utf8)
[IO.File]::WriteAllText("$output/BuildVersion.h", '#define SPACEYAKUZA_BUILD_LABEL "Online test"', $utf8)

$previousCache = $env:GOCACHE
$previousUrl = $env:SPACEYAKUZA_SERVER_URL
$previousListen = $env:SPACEYAKUZA_LISTEN
$previousScores = $env:SPACEYAKUZA_SCORE_FILE
$previousLocal = $env:LOCALAPPDATA
$previousCert = $env:SPACEYAKUZA_TLS_CERT
$previousKey = $env:SPACEYAKUZA_TLS_KEY
$serverProcess = $null
try {
    $env:GOCACHE = "$repo/obj/OnlineTests/GoCache"
    Push-Location $output
    try {
        & $goCommand test -v ./...
        if ($LASTEXITCODE) { throw 'Go tutorial tests failed' }
        & $goCommand build -o "$output/server.exe" .
        if ($LASTEXITCODE) { throw 'Go tutorial build failed' }
    } finally { Pop-Location }

    $vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
    if (!$vs) { throw 'Visual Studio not found' }
    $arguments = @('/nologo', '/std:c++20', '/EHsc', '/MT', '/utf-8', '/UNDEBUG',
        ('/I"' + $output + '"'), ('/Fo"' + $output + '/"'), ('/Fe"' + $output + '/OnlineCoopTests.exe"'),
        ('"' + $repo + '/Tests/OnlineCoopTests.cpp"'),
        ('"' + $repo + '/Infrastructure/ExternalServices/OnlineCoopSession.cpp"'),
        ('"' + $repo + '/Infrastructure/Repositories/SettingsRepository.cpp"'), '/link', 'winhttp.lib')
    $response = Join-Path $output 'compile.rsp'
    Set-Content -LiteralPath $response -Value ($arguments -join ' ') -Encoding utf8
    cmd /c "call `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul && cl.exe @`"$response`""
    if ($LASTEXITCODE) { throw 'Online client test compilation failed' }

    # 空きポートと隔離した保存先だけを使用する
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $port = $listener.LocalEndpoint.Port
    $listener.Stop()
    $env:SPACEYAKUZA_SERVER_URL = "http://127.0.0.1:$port"
    $env:SPACEYAKUZA_LISTEN = "127.0.0.1:$port"
    $env:SPACEYAKUZA_SCORE_FILE = "$output/scores.log"
    $env:LOCALAPPDATA = "$output/UserData"
    $env:SPACEYAKUZA_TLS_CERT = $null
    $env:SPACEYAKUZA_TLS_KEY = $null
    $serverProcess = Start-Process -FilePath "$output/server.exe" -WorkingDirectory $output -WindowStyle Hidden -PassThru -RedirectStandardError "$output/server.log"
    $started = $false
    for ($attempt = 0; $attempt -lt 30; $attempt++) {
        try {
            $null = Invoke-WebRequest "$env:SPACEYAKUZA_SERVER_URL/v1/rankings?coop=0" -UseBasicParsing -TimeoutSec 1
            $started = $true
            break
        } catch { Start-Sleep -Milliseconds 100 }
    }
    if (!$started -or $serverProcess.HasExited) { throw 'Test server did not start' }
    & "$output/OnlineCoopTests.exe"
    if ($LASTEXITCODE) { throw 'Online client integration tests failed' }
    Write-Host "Online tests passed: $output"
} finally {
    if ($serverProcess -and !$serverProcess.HasExited) { Stop-Process -Id $serverProcess.Id -Force }
    $env:GOCACHE = $previousCache
    $env:SPACEYAKUZA_SERVER_URL = $previousUrl
    $env:SPACEYAKUZA_LISTEN = $previousListen
    $env:SPACEYAKUZA_SCORE_FILE = $previousScores
    $env:LOCALAPPDATA = $previousLocal
    $env:SPACEYAKUZA_TLS_CERT = $previousCert
    $env:SPACEYAKUZA_TLS_KEY = $previousKey
}

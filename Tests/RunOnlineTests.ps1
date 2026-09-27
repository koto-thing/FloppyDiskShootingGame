<#
.SYNOPSIS
実際のGoサーバーとC++クライアントを隔離した保存先で実通信検証する
#>
param([string]$GoExe = 'go', [string]$PythonExe = 'python', [string]$ProbeServerUrl = '')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$output = Join-Path $repo ('obj/OnlineTests/' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Force -Path $output
$goCommand = (Get-Command $GoExe -ErrorAction Stop).Source
$utf8 = New-Object System.Text.UTF8Encoding($false)
Get-ChildItem -LiteralPath "$repo/Server" -File | Where-Object { $_.Extension -eq '.go' -or $_.Name -in @('go.mod', 'go.sum') } |
    Copy-Item -Destination $output
[IO.File]::WriteAllText("$output/BuildVersion.h", '#define SPACEYAKUZA_BUILD_LABEL "Online test"', $utf8)

$previousCache = $env:GOCACHE
$previousUrl = $env:SPACEYAKUZA_SERVER_URL
$previousListen = $env:SPACEYAKUZA_LISTEN
$previousScores = $env:SPACEYAKUZA_SCORE_FILE
$previousLocal = $env:LOCALAPPDATA
$previousCert = $env:SPACEYAKUZA_TLS_CERT
$previousKey = $env:SPACEYAKUZA_TLS_KEY
$serverProcess = $null
$proxyProcess = $null
try {
    $env:GOCACHE = "$repo/obj/OnlineTests/GoCache"
    Push-Location $output
    try {
        & $goCommand test -v ./...
        if ($LASTEXITCODE) { throw 'Go server tests failed' }
        & $goCommand build -o "$output/server.exe" .
        if ($LASTEXITCODE) { throw 'Go server build failed' }
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

    # 指定した公開経路は専用部屋で計測し、通常試験のスコア送信を実行しない
    if ($ProbeServerUrl) {
        $env:SPACEYAKUZA_SERVER_URL = $ProbeServerUrl
        $env:LOCALAPPDATA = "$output/UserData"
        & "$output/OnlineCoopTests.exe" --probe
        if ($LASTEXITCODE) { throw 'Online route probe failed' }
        Get-Content -LiteralPath "$output/UserData/SpaceYakuza/online-network.log"
        Write-Host "Online route probe completed: $output"
        return
    }

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
    # 往復80〜130msと揺らぎを加えて、同じC++クライアントを再検証する
    $pythonCommand = (Get-Command $PythonExe -ErrorAction Stop).Source
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $proxyPort = $listener.LocalEndpoint.Port
    $listener.Stop()
    $blackhole = "$output/blackhole.marker"
    $proxyProcess = Start-Process -FilePath $pythonCommand -ArgumentList @('"' + "$repo/Tests/OnlineDelayProxy.py" + '"', $proxyPort, $port, '"' + $blackhole + '"') -WindowStyle Hidden -PassThru -RedirectStandardError "$output/proxy.log"
    $env:SPACEYAKUZA_SERVER_URL = "http://127.0.0.1:$proxyPort"
    $started = $false
    for ($attempt = 0; $attempt -lt 30; $attempt++) {
        try {
            $null = Invoke-WebRequest "$env:SPACEYAKUZA_SERVER_URL/v1/rankings?coop=0" -UseBasicParsing -TimeoutSec 2
            $started = $true
            break
        } catch { Start-Sleep -Milliseconds 100 }
    }
    if (!$started -or $proxyProcess.HasExited) { throw 'Delay proxy did not start' }
    # ロビー後に周期的な大きい揺らぎを加え、送信先行量の適応と回復を検証する
    & "$output/OnlineCoopTests.exe" --jitter $blackhole
    if ($LASTEXITCODE) { throw 'Online client jitter adaptation tests failed' }
    & "$output/OnlineCoopTests.exe" --bursts $blackhole
    if ($LASTEXITCODE) { throw 'Online client recurring jitter tests failed' }
    & "$output/OnlineCoopTests.exe" --delayed $blackhole
    if ($LASTEXITCODE) { throw 'Online client delayed integration tests failed' }
    Write-Host "Online tests passed: $output"
} finally {
    if ($proxyProcess -and !$proxyProcess.HasExited) { Stop-Process -Id $proxyProcess.Id -Force }
    if ($serverProcess -and !$serverProcess.HasExited) { Stop-Process -Id $serverProcess.Id -Force }
    $env:GOCACHE = $previousCache
    $env:SPACEYAKUZA_SERVER_URL = $previousUrl
    $env:SPACEYAKUZA_LISTEN = $previousListen
    $env:SPACEYAKUZA_SCORE_FILE = $previousScores
    $env:LOCALAPPDATA = $previousLocal
    $env:SPACEYAKUZA_TLS_CERT = $previousCert
    $env:SPACEYAKUZA_TLS_KEY = $previousKey
}

param(
  [string]$Target = '192.168.23.110',
  [int]$Port = 80,
  [string]$Path = '/ws'
)

$ErrorActionPreference = 'Stop'

function New-WSClient([string]$uri){
  $ws = [System.Net.WebSockets.ClientWebSocket]::new()
  $cts = [System.Threading.CancellationTokenSource]::new()
  $u = [Uri]::new($uri)
  [void]($ws.ConnectAsync($u, $cts.Token).GetAwaiter().GetResult())
  return $ws
}

function Receive-Text([System.Net.WebSockets.ClientWebSocket]$ws, [int]$timeoutMs = 3000){
  $cts = [System.Threading.CancellationTokenSource]::new()
  $cts.CancelAfter($timeoutMs)
  $buffer = New-Object byte[] 4096
  $ms = New-Object System.IO.MemoryStream
  try {
    do {
      $seg = New-Object System.ArraySegment[byte]($buffer, 0, $buffer.Length)
      $res = $ws.ReceiveAsync($seg, $cts.Token).GetAwaiter().GetResult()
      if ($res.Count -gt 0) { $ms.Write($buffer, 0, $res.Count) }
      if ($res.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close) { return $null }
    } while (-not $res.EndOfMessage)
  } catch {
    return $null
  }
  $txt = [System.Text.Encoding]::UTF8.GetString($ms.ToArray())
  return $txt
}

function Send-Json([System.Net.WebSockets.ClientWebSocket]$ws, $obj){
  $json = ($obj | ConvertTo-Json -Compress -Depth 4)
  $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
  $seg = New-Object System.ArraySegment[byte]($bytes, 0, $bytes.Length)
  [void]($ws.SendAsync($seg, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [System.Threading.CancellationToken]::None).GetAwaiter().GetResult())
}

function Await-State($ws, [scriptblock]$predicate, [int]$timeoutMs=4000){
  $t0 = [Environment]::TickCount
  while ([Environment]::TickCount - $t0 -lt $timeoutMs){
    $txt = Receive-Text $ws 1000
    if ([string]::IsNullOrEmpty($txt)) { continue }
    try { $m = $txt | ConvertFrom-Json } catch { continue }
    if ($m.type -eq 'state' -and (& $predicate $m)) { return $m }
  }
  return $null
}

function Assert($cond, $msg){ if (-not $cond){ throw "ASSERT FAIL: $msg" } else { Write-Host "[OK] $msg" -ForegroundColor Green } }

$uri = "ws://$Target`:$Port$Path"
Write-Host "Connecting WS1 -> $uri" -ForegroundColor Cyan
$ws1 = New-WSClient $uri
Write-Host ("WS1 type: {0}" -f $ws1.GetType().FullName)
try {
  # Prompt a state broadcast to ensure immediate message
  Send-Json $ws1 @{ cmd='arm'; on=$false }
  $m1 = Await-State $ws1 { param($m) $true } 5000
  Assert ($null -ne $m1) "Receive initial telemetry"
  Assert ($m1.cfg -ne $null -and $m1.cfg.width -ge 5) "Has cfg with sane width"

  Write-Host "Connecting WS2 -> $uri" -ForegroundColor Cyan
  $ws2 = New-WSClient $uri
  try {
    $m2 = Await-State $ws2 { param($m) $true } 5000
    Assert ($null -ne $m2) "WS2 initial telemetry"

    # Snapshot config
    $orig = @{ mode=$m1.cfg.mode; width=[int]$m1.cfg.width; spacing=[int]$m1.cfg.spacing; repeat=[int]$m1.cfg.repeat }
    $newW = [Math]::Min(100, ($orig.width + 1))
    $newS = [Math]::Min(100, ($orig.spacing + 1))
    $newR = if ($orig.repeat -lt 4) { $orig.repeat + 1 } else { 1 }

    Write-Host "Sending cfg change on WS1 -> mode=$($orig.mode) width=$newW spacing=$newS repeat=$newR" -ForegroundColor Yellow
    Send-Json $ws1 @{ cmd='cfg'; mode=$orig.mode; width=$newW; spacing=$newS; repeat=$newR }

    $ok1 = Await-State $ws1 { param($m) $m.cfg.width -eq $newW -and $m.cfg.spacing -eq $newS -and $m.cfg.repeat -eq $newR } 4000
    $ok2 = Await-State $ws2 { param($m) $m.cfg.width -eq $newW -and $m.cfg.spacing -eq $newS -and $m.cfg.repeat -eq $newR } 4000
    Assert ($null -ne $ok1) "WS1 saw cfg reflected"
    Assert ($null -ne $ok2) "WS2 saw cfg reflected"

    Write-Host "Arming via WS1" -ForegroundColor Yellow
    Send-Json $ws1 @{ cmd='arm'; on=$true }
    $arm1 = Await-State $ws1 { param($m) $m.armed -eq $true } 4000
    $arm2 = Await-State $ws2 { param($m) $m.armed -eq $true } 4000
    Assert ($null -ne $arm1) "WS1 armed"
    Assert ($null -ne $arm2) "WS2 armed"

    Write-Host "Attempt cfg while armed (should be ignored)" -ForegroundColor Yellow
    $tryW = if ($newW -lt 100) { $newW + 1 } else { 99 }
    Send-Json $ws1 @{ cmd='cfg'; mode=$orig.mode; width=$tryW; spacing=$newS; repeat=$newR }
    Start-Sleep -Milliseconds 200
    $chk = Await-State $ws1 { param($m) $m.armed -eq $true } 1500
    Assert ($chk -ne $null -and $chk.cfg.width -eq $newW) "Cfg unchanged while armed"

    Write-Host "FIRE via WS1 (auto-disarm on completion)" -ForegroundColor Yellow
    Send-Json $ws1 @{ cmd='fire' }
    $sawPulse = Await-State $ws1 { param($m) $m.pulseActive -eq $true } 2500
    if ($sawPulse -eq $null) { Write-Warning "Did not observe pulseActive=true (timing)." }
    $dis = Await-State $ws2 { param($m) $m.armed -eq $false } 4000
    Assert ($dis -ne $null) "Auto-disarm observed"

    Write-Host "Restoring original cfg" -ForegroundColor Yellow
    Send-Json $ws1 @{ cmd='cfg'; mode=$orig.mode; width=$orig.width; spacing=$orig.spacing; repeat=$orig.repeat }
    $rest = Await-State $ws2 { param($m) $m.cfg.width -eq $orig.width -and $m.cfg.spacing -eq $orig.spacing -and $m.cfg.repeat -eq $orig.repeat } 4000
    Assert ($rest -ne $null) "Original cfg restored"

    Write-Host "All tests PASSED" -ForegroundColor Green
  } finally {
    if ($ws2) { try { $ws2.Abort(); $ws2.Dispose() } catch {} }
  }
} finally {
  if ($ws1) { try { $ws1.Abort(); $ws1.Dispose() } catch {} }
}

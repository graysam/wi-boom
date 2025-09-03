# PowerShell WebSocket test script for HV Trigger Async
# Usage: pwsh -File .tools/ws_test.ps1 [-Host 10.11.12.1]

param(
  [string]$Host = '10.11.12.1'
)

Add-Type -AssemblyName System.Net.Http
Add-Type -AssemblyName System.Net.WebSockets

function Receive-WSMessage {
  param([System.Net.WebSockets.ClientWebSocket]$ws, [int]$timeoutMs = 2000)
  $buffer = New-Object System.ArraySegment[byte] (,@(New-Object byte[] 2048))
  $cts = New-Object System.Threading.CancellationTokenSource $timeoutMs
  $sb = New-Object System.Text.StringBuilder
  do {
    $res = $ws.ReceiveAsync($buffer, $cts.Token).GetAwaiter().GetResult()
    if ($res.Count -gt 0) {
      $sb.Append([System.Text.Encoding]::UTF8.GetString($buffer.Array, 0, $res.Count)) | Out-Null
    }
  } while (-not $res.EndOfMessage)
  return $sb.ToString()
}

function Send-WSMessage {
  param([System.Net.WebSockets.ClientWebSocket]$ws, [string]$text)
  $bytes = [System.Text.Encoding]::UTF8.GetBytes($text)
  $buffer = New-Object System.ArraySegment[byte] (,$bytes)
  $null = $ws.SendAsync($buffer, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [Threading.CancellationToken]::None).GetAwaiter().GetResult()
}

Write-Host "Connecting to ws://$Host/ws ..."
$ws = [System.Net.WebSockets.ClientWebSocket]::new()
$uri = [Uri]("ws://$Host/ws")
$ws.ConnectAsync($uri, [Threading.CancellationToken]::None).GetAwaiter().GetResult()
Write-Host "Connected. Waiting for telemetry..."

# Read a couple of telemetry frames
1..3 | ForEach-Object {
  try { $msg = Receive-WSMessage -ws $ws -timeoutMs 3000; if ($msg) { Write-Host "<-" $msg } } catch {}
}

# Set config, arm, fire, disarm
$cfg = @{ cmd = 'cfg'; mode = 'buzz'; width = 12; spacing = 25; repeat = 2 } | ConvertTo-Json -Compress
Write-Host "->" $cfg
Send-WSMessage -ws $ws -text $cfg
Start-Sleep -Milliseconds 300

$arm = @{ cmd = 'arm'; on = $true } | ConvertTo-Json -Compress
Write-Host "->" $arm
Send-WSMessage -ws $ws -text $arm
Start-Sleep -Milliseconds 300

$fire = @{ cmd = 'fire' } | ConvertTo-Json -Compress
Write-Host "->" $fire
Send-WSMessage -ws $ws -text $fire
Start-Sleep -Milliseconds 300

$disarm = @{ cmd = 'arm'; on = $false } | ConvertTo-Json -Compress
Write-Host "->" $disarm
Send-WSMessage -ws $ws -text $disarm

# Drain a few state frames
1..5 | ForEach-Object {
  try { $msg = Receive-WSMessage -ws $ws -timeoutMs 1000; if ($msg) { Write-Host "<-" $msg } } catch {}
}

$ws.Dispose()
Write-Host "Done."


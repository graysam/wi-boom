Add-Type -AssemblyName System.Net.WebSockets
Add-Type -AssemblyName System.Runtime.Extensions

Param([string]$Url = "ws://10.11.12.1/ws")
$ws = [System.Net.WebSockets.ClientWebSocket]::new()
$uri = [System.Uri]::new($Url)
Write-Host "[i] Connecting $Url"; $ws.ConnectAsync($uri, [Threading.CancellationToken]::None).Wait()

function Send-Json($obj) {
  $json = ($obj | ConvertTo-Json -Compress)
  $bytes = [Text.Encoding]::UTF8.GetBytes($json)
  $seg = [ArraySegment[byte]]::new($bytes)
  $ws.SendAsync($seg, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [Threading.CancellationToken]::None).Wait()
}

Send-Json @{cmd='arm'; on=$true}
Start-Sleep -Milliseconds 200
Send-Json @{cmd='cfg'; mode='single'; width=10; spacing=20; repeat=1}
Start-Sleep -Milliseconds 200
Send-Json @{cmd='fire'}
Write-Host "[i] Commands sent. Receiving telemetry for 3s..."

$buf = New-Object byte[] 2048
$seg = [ArraySegment[byte]]::new($buf)
$sw = [Diagnostics.Stopwatch]::StartNew()
while ($sw.ElapsedMilliseconds -lt 3000) {
  if ($ws.State -ne 'Open') { break }
  $res = $ws.ReceiveAsync($seg, [Threading.CancellationToken]::None).Result
  if ($res.Count -gt 0) {
    $text = [Text.Encoding]::UTF8.GetString($buf,0,$res.Count)
    Write-Host $text
  }
}
$ws.Dispose()


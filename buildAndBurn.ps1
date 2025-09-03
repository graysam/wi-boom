# PowerShell Build + Burn helper for HV Trigger Async (ESP32-S3)
# Usage: pwsh -ExecutionPolicy Bypass -File .\buildAndBurn.ps1

param(
  [string]$SketchDefault = 'hv_trigger_async.ino',
  [string]$FqbnDefault   = 'esp32:esp32:esp32s3',
  [string]$BuildDir      = 'build'
)

function Write-Section($t){"`n==========================================================`n$($t)`n=========================================================="}

function Require-ArduinoCLI {
  if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    Write-Error 'arduino-cli not found on PATH. Install: https://arduino.github.io/arduino-cli/latest/'
    exit 1
  }
}

function Ensure-CoreEsp32 {
  $hasCore = arduino-cli core list | Select-String -Pattern '^esp32:esp32' -Quiet
  if (-not $hasCore) {
    Write-Host 'Installing esp32 core ...'
    arduino-cli core update-index | Out-Null
    arduino-cli core install esp32:esp32 | Out-Null
  }
}

function Prompt-YesNo($Prompt, $Default='Y'){
  $d = if ($Default -match '^[Yy]') { 'Y/n' } else { 'y/N' }
  $ans = Read-Host "$Prompt [$d]"
  if ([string]::IsNullOrWhiteSpace($ans)) { $ans = $Default }
  return ($ans -match '^(?i:y|yes)$')
}

function Prompt-String($Prompt, $Default){
  $ans = Read-Host "$Prompt [$Default]"
  if ([string]::IsNullOrWhiteSpace($ans)) { return $Default }
  return $ans
}

function Validate-Fqbn($fqbn){
  & arduino-cli board details -b $fqbn *> $null
  return $LASTEXITCODE -eq 0
}

function Select-Port {
  Write-Host 'Detecting serial ports...'
  $ports = @()
  try {
    $list = arduino-cli board list 2>$null
    foreach($line in $list){
      if ($line -match '^(COM\d+)') { $ports += $matches[1] }
      elseif ($line -match '^(/dev/(tty|cu)\.[^\s]+)') { $ports += $matches[1] }
    }
  } catch {}
  if ($ports.Count -eq 0) {
    try {
      $cims = Get-CimInstance Win32_SerialPort 2>$null | Select-Object -ExpandProperty DeviceID
      foreach($c in $cims){ if ($c -match '^COM\d+$') { $ports += $c } }
    } catch {}
  }
  $ports = $ports | Select-Object -Unique
  if ($ports.Count -eq 0) {
    return (Read-Host 'No ports detected. Enter a port (e.g., COM9 or /dev/tty.usbserial-XXXX)')
  }
  Write-Host 'Available ports:'
  for($i=0; $i -lt $ports.Count; $i++){ Write-Host ("  {0}) {1}" -f ($i+1), $ports[$i]) }
  $sel = Read-Host ("Select port [1-{0}]" -f $ports.Count)
  if (-not ($sel -as [int]) -or $sel -lt 1 -or $sel -gt $ports.Count) {
    throw 'Invalid selection'
  }
  return $ports[$sel-1]
}

Require-ArduinoCLI
Ensure-CoreEsp32

Write-Host (Write-Section 'Build + Burn (Windows PowerShell)')

$sketch = if (Prompt-YesNo "Use sketch $SketchDefault?" 'Y') { $SketchDefault } else { Read-Host 'Enter path to .ino' }
if (-not (Test-Path -Path $sketch -PathType Leaf)) { Write-Error "Sketch not found: $sketch"; exit 1 }

do {
  $fqbn = Prompt-String 'Enter FQBN' $FqbnDefault
  if (-not (Validate-Fqbn $fqbn)) { Write-Warning "Invalid FQBN: $fqbn" }
} until (Validate-Fqbn $fqbn)

if (Test-Path $BuildDir) {
  if (Prompt-YesNo "Build folder '$BuildDir' exists. Overwrite/clean?" 'Y') { Remove-Item -Recurse -Force $BuildDir } else { Write-Host 'Aborting per user choice.'; exit 1 }
}

Write-Host (Write-Section "Compiling: $sketch")
$compileArgs = @('compile','--fqbn', $fqbn,'--output-dir', $BuildDir, $sketch)
& arduino-cli @compileArgs
if ($LASTEXITCODE -ne 0) {
  Write-Warning 'Compile failed. Retrying once...'
  Start-Sleep -Seconds 1
  & arduino-cli @compileArgs; if ($LASTEXITCODE -ne 0) { Write-Error 'Compile failed again.'; exit 1 }
}
Write-Host "Build artifacts in: $BuildDir"

if (Prompt-YesNo 'Upload to device now?' 'Y') {
  try {
    $port = Select-Port
  } catch { Write-Error $_; exit 1 }
  Write-Host (Write-Section "Uploading to $port")
  $uploadArgs = @('upload','-p', $port,'--fqbn', $fqbn, $sketch)
  & arduino-cli @uploadArgs
  if ($LASTEXITCODE -ne 0) {
    Write-Warning 'Upload failed. Retrying once...'
    Start-Sleep -Seconds 1
    & arduino-cli @uploadArgs; if ($LASTEXITCODE -ne 0) { Write-Error 'Upload failed again.'; exit 1 }
  }
  Write-Host 'Upload completed.'
} else {
  Write-Host 'Skipping upload.'
}

# Optional serial monitor
if (Prompt-YesNo 'Open serial monitor now?' 'Y') {
  if (-not $port) {
    try { $port = Select-Port } catch { Write-Error $_; exit 1 }
  }
  $baud = Prompt-String 'Baudrate' '115200'
  Write-Host (Write-Section "Serial monitor (^C to exit) on $port @ $baud")
  $monArgs = @('monitor','-p', $port,'-c', "baudrate=$baud")
  & arduino-cli @monArgs
  if ($LASTEXITCODE -ne 0) {
    Write-Warning 'Monitor failed. Retry once?'
    if (Prompt-YesNo 'Retry monitor?' 'Y') { & arduino-cli @monArgs }
  }
}

Write-Host (Write-Section 'Done')

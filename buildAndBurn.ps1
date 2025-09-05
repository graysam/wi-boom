# PowerShell Build + Burn helper for HV Trigger Async (ESP32-S3)
# Usage: pwsh -ExecutionPolicy Bypass -File .\buildAndBurn.ps1

param(
  [string]$SketchDefault = 'hv_trigger_async.ino',
  [string]$FqbnDefault   = 'esp32:esp32:esp32cam:PartitionScheme=default',
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

function Scan-SketchesAndBinaries {
  $root = Split-Path -Parent $MyInvocation.MyCommand.Path
  Push-Location $root
  $sketches = Get-ChildItem -Filter *.ino -Name | Sort-Object
  $binDirs = @()
  foreach($d in Get-ChildItem -Directory){
    $bins = Get-ChildItem -Path $d.FullName -Filter *.bin -ErrorAction SilentlyContinue
    if ($bins -and $bins.Count -ge 2) { $binDirs += $d.Name }
  }
  Pop-Location
  return @{ Root=$root; Sketches=$sketches; BinDirs=$binDirs }
}

function Select-FromList($Title, [string[]]$Items){
  if ($Items.Count -eq 0) { return $null }
  Write-Host $Title
  for($i=0;$i -lt $Items.Count;$i++){ Write-Host ("  {0}) {1}" -f ($i+1), $Items[$i]) }
  $sel = Read-Host ("Select [1-{0}]" -f $Items.Count)
  if (-not ($sel -as [int]) -or $sel -lt 1 -or $sel -gt $Items.Count) { throw 'Invalid selection' }
  return $Items[$sel-1]
}

function Ensure-Port-Ready($port){
  while ($true) {
    try {
      $sp = New-Object System.IO.Ports.SerialPort $port,115200,'None',8,'One'
      $sp.ReadTimeout=500; $sp.WriteTimeout=500
      $sp.Open(); Start-Sleep -Milliseconds 50; $sp.Close()
      return
    } catch {
      Write-Warning "Port $port appears busy or unavailable. Close other apps and press Enter to retry, or Ctrl+C to abort."
      Read-Host | Out-Null
    }
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

Write-Host (Write-Section 'Build + Burn (PowerShell)')

# Scan environment
$scan = Scan-SketchesAndBinaries
$root = $scan.Root
$sketch = $SketchDefault
if ($scan.Sketches.Count -gt 0) {
  try { $sel = Select-FromList 'Found sketches:' $scan.Sketches; if ($sel) { $sketch = $sel } } catch {}
}
if (-not (Test-Path -Path (Join-Path $root $sketch) -PathType Leaf)) { Write-Error "Sketch not found: $sketch"; exit 1 }

do {
  $fqbn = Prompt-String 'Enter FQBN' $FqbnDefault
  if (-not (Validate-Fqbn $fqbn)) { Write-Warning "Invalid FQBN: $fqbn" }
} until (Validate-Fqbn $fqbn)

# Choose action
Write-Host 'Actions:'
Write-Host '  1) Build only'
Write-Host '  2) Upload only (existing build)'
Write-Host '  3) Build and upload'
$action = Read-Host 'Select [1-3]'
if ($action -notin '1','2','3') { Write-Error 'Invalid selection'; exit 1 }

# If uploading, choose upload method and port
$port = $null; $uploadVia = 'serial'
if ($action -in '2','3') {
  $uploadVia = if (Prompt-YesNo 'Upload via OTA (network)?' 'N') { 'ota' } else { 'serial' }
  if ($uploadVia -eq 'serial') {
    try { $port = Select-Port } catch { Write-Error $_; exit 1 }
    Ensure-Port-Ready $port
  } else {
    $port = Prompt-String 'Enter device IP (or host:port)' '10.11.12.1'
  }
}

# If uploading existing, select build artifacts directory
$inputDir = $null
if ($action -eq '2') {
  if ($scan.BinDirs.Count -gt 0) {
    try { $binSel = Select-FromList 'Found build directories:' $scan.BinDirs; if ($binSel) { $inputDir = (Join-Path $root $binSel) } } catch {}
  }
  if (-not $inputDir) { $inputDir = Prompt-String 'Enter path to directory with compiled artifacts (*.bin, *.elf)' (Join-Path $root $BuildDir) }
}

# Build step (if chosen)
if ($action -in '1','3') {
  if (Test-Path $BuildDir) {
    if (Prompt-YesNo "Build folder '$BuildDir' exists. Overwrite/clean?" 'Y') { Remove-Item -Recurse -Force $BuildDir } else { Write-Host 'Aborting per user choice.'; exit 1 }
  }
  Write-Host (Write-Section "Compiling: $sketch")
  $compileArgs = @('compile','--fqbn', $fqbn,'--output-dir', $BuildDir, (Join-Path $root $sketch))
  & arduino-cli @compileArgs
  if ($LASTEXITCODE -ne 0) {
    Write-Warning 'Compile failed. Retrying once...'
    Start-Sleep -Seconds 1
    & arduino-cli @compileArgs; if ($LASTEXITCODE -ne 0) { Write-Error 'Compile failed again.'; exit 1 }
  }
  $inputDir = (Resolve-Path $BuildDir).Path
  Write-Host "Build artifacts in: $inputDir"
}

# Upload step (if chosen)
if ($action -in '2','3') {
  Write-Host (Write-Section ("Uploading via {0} to {1}" -f $uploadVia,$port))
  $args = @('upload','--fqbn', $fqbn)
  if ($uploadVia -eq 'serial') { $args += @('-p', $port) } else { $args += @('-p', $port) }
  if ($inputDir) { $args += @('--input-dir', $inputDir) } else { $args += (Join-Path $root $sketch) }
  & arduino-cli @args
  if ($LASTEXITCODE -ne 0) {
    Write-Warning 'Upload failed. Close conflicting apps and retry?'
    if (Prompt-YesNo 'Retry upload?' 'Y') {
      & arduino-cli @args; if ($LASTEXITCODE -ne 0) { Write-Error 'Upload failed again.'; exit 1 }
    } else { exit 1 }
  }
  Write-Host 'Upload completed.'
}

Write-Host (Write-Section 'Done')

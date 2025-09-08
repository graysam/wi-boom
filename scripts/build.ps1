Param(
  [string]$Port,
  [switch]$Upload
)

$fqbn = "esp8266:esp8266:nodemcuv2"
Write-Host "[i] Using FQBN: $fqbn"

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
  Write-Host "[!] arduino-cli not found. Install from https://arduino.github.io/arduino-cli/"
  exit 1
}

arduino-cli core update-index | Out-Null
arduino-cli core install esp8266:esp8266

Write-Host "[i] Compiling..."
arduino-cli compile --fqbn $fqbn .
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Upload) {
  if (-not $Port) {
    $Port = Read-Host "Enter serial port (e.g., COM3)"
  }
  Write-Host "[i] Uploading to $Port"
  arduino-cli upload -p $Port --fqbn $fqbn .
}


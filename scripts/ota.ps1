Param(
  [string]$Host = "10.11.12.1",
  [int]$Port = 3232
)
$fqbn = "esp8266:esp8266:nodemcuv2"
if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) { Write-Host "arduino-cli not found"; exit 1 }
arduino-cli compile --fqbn $fqbn .
arduino-cli upload --fqbn $fqbn --port $Host --protocol network .


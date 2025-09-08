#!/usr/bin/env bash
set -euo pipefail
FQBN="esp8266:esp8266:nodemcuv2"
echo "[i] Using FQBN: $FQBN"
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "[!] arduino-cli not found. Install https://arduino.github.io/arduino-cli/" >&2
  exit 1
fi
arduino-cli core update-index >/dev/null
arduino-cli core install esp8266:esp8266
echo "[i] Compiling..."
arduino-cli compile --fqbn "$FQBN" .
if [[ "${1:-}" == "upload" ]]; then
  PORT="${2:-}"
  if [[ -z "$PORT" ]]; then read -rp "Enter serial port (e.g., /dev/ttyUSB0): " PORT; fi
  echo "[i] Uploading to $PORT"
  arduino-cli upload -p "$PORT" --fqbn "$FQBN" .
fi


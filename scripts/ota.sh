#!/usr/bin/env bash
set -euo pipefail
FQBN="esp8266:esp8266:nodemcuv2"
HOST="${1:-10.11.12.1}"
PORT=3232
arduino-cli compile --fqbn "$FQBN" .
arduino-cli upload --fqbn "$FQBN" --port "$HOST" --protocol network .


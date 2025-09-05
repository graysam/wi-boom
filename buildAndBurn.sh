#!/usr/bin/env bash
set -euo pipefail

# Build + Burn helper for HV Trigger Async (ESP32-S3)
# Requires: arduino-cli on PATH

SKETCH_DEFAULT="hv_trigger_async.ino"
FQBN_DEFAULT="esp32:esp32:esp32cam:PartitionScheme=default"
BUILD_DIR="build"

banner() { echo "=========================================================="; }

prompt_yes_no() {
  local prompt="$1"; local default="$2"; local ans
  read -r -p "$prompt [$default] " ans || true
  ans=${ans:-$default}
  case "${ans,,}" in y|yes) return 0;; n|no) return 1;; *) return 1;; esac
}

require_arduino_cli() {
  if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "arduino-cli not found on PATH. Install from https://arduino.github.io/arduino-cli/latest/"
    exit 1
  fi
}

ensure_core() {
  if ! arduino-cli core list | grep -q "^esp32:esp32\b"; then
    echo "Installing esp32 core ..."
    arduino-cli core update-index
    arduino-cli core install esp32:esp32
  fi
}

select_port() {
  echo "Detecting serial ports..."
  local ports=()
  # Prefer arduino-cli listing
  while IFS= read -r line; do
    # First field should be a port path
    local p
    p=$(awk '{print $1}' <<<"$line")
    case "$p" in 
      /dev/tty*|/dev/cu*|COM[0-9]*) ports+=("$p");;
    esac
  done < <(arduino-cli board list | tail -n +2)

  # Fallback to disk scan on *nix if none
  if [[ ${#ports[@]} -eq 0 && "$(uname -s)" != MINGW* && "$(uname -s)" != CYGWIN* ]]; then
    mapfile -t ports < <(ls /dev/tty.* /dev/cu.* 2>/dev/null || true)
  fi

  if [[ ${#ports[@]} -eq 0 ]]; then
    echo "No serial ports detected. Enter a port manually (e.g., COM9 or /dev/tty.usbserial-XXXX):"
    read -r sel
    echo "$sel"
    return 0
  fi

  echo "Available ports:"
  local i=0
  for p in "${ports[@]}"; do
    i=$((i+1)); echo "  $i) $p"
  done
  local choice
  read -r -p "Select port [1-$i]: " choice
  if [[ -z "$choice" || "$choice" -lt 1 || "$choice" -gt $i ]]; then
    echo "Invalid selection"; exit 1
  fi
  echo "${ports[$((choice-1))]}"
}

open_serial_monitor() {
  local default_port="$1"
  local port="$default_port"
  if [[ -z "$port" ]]; then
    port=$(select_port)
  else
    read -r -p "Serial port [$port]: " ans; port=${ans:-$port}
  fi
  local baud
  read -r -p "Baudrate [115200]: " baud; baud=${baud:-115200}
  banner; echo "Serial monitor (Ctrl+C to exit) on $port @ $baud"
  set +e
  arduino-cli monitor -p "$port" -c baudrate="$baud"
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "Monitor failed (rc=$rc). Retry? [Y/n]"; read -r ans; ans=${ans:-Y}
    if [[ ${ans,,} =~ ^y ]]; then
      arduino-cli monitor -p "$port" -c baudrate="$baud"
    fi
  fi
  set -e
}

main() {
  banner
  echo "Build + Burn (macOS/Linux)"
  require_arduino_cli
  ensure_core

  # Scan sketches in script dir
  local root
  root="$(cd -- "$(dirname -- "$0")" && pwd)"
  mapfile -t sketches < <(find "$root" -maxdepth 1 -type f -name "*.ino" -printf "%f\n" | sort)
  local sketch="$SKETCH_DEFAULT"
  if [[ ${#sketches[@]} -gt 0 ]]; then
    echo "Found sketches:"; local i=0; for s in "${sketches[@]}"; do i=$((i+1)); echo "  $i) $s"; done
    read -r -p "Select sketch [1-${#sketches[@]}] or Enter for default ($SKETCH_DEFAULT): " sel
    if [[ -n "$sel" ]]; then sketch="${sketches[$((sel-1))]}"; fi
  fi
  [[ -f "$sketch" ]] || { echo "Sketch not found: $sketch"; exit 1; }

  local fqbn="$FQBN_DEFAULT"
  read -r -p "Enter FQBN [$FQBN_DEFAULT]: " ans || true
  fqbn=${ans:-$FQBN_DEFAULT}

  echo "Actions:"; echo "  1) Build only"; echo "  2) Upload only (existing build)"; echo "  3) Build and upload"
  read -r -p "Select [1-3]: " action
  [[ "$action" =~ ^[123]$ ]] || { echo "Invalid selection"; exit 1; }

  local input_dir=""; local port=""; local method="serial"
  if [[ "$action" == "2" || "$action" == "3" ]]; then
    read -r -p "Upload via OTA (network)? [y/N]: " uans; uans=${uans:-N}
    if [[ ${uans,,} =~ ^y ]]; then method="ota"; read -r -p "Enter device IP (or host:port) [10.11.12.1]: " port; port=${port:-10.11.12.1}; else
      port=$(select_port)
    fi
  fi

  if [[ "$action" == "2" ]]; then
    echo "Scanning for build artifact directories..."
    mapfile -t bindirs < <(find "$root" -maxdepth 2 -type d -name build)
    if [[ ${#bindirs[@]} -gt 0 ]]; then
      echo "Found:"; local i=0; for d in "${bindirs[@]}"; do i=$((i+1)); echo "  $i) $d"; done
      read -r -p "Select input dir [1-${#bindirs[@]}] or enter path: " sel
      if [[ "$sel" =~ ^[0-9]+$ ]]; then input_dir="${bindirs[$((sel-1))]}"; else input_dir="$sel"; fi
    else
      read -r -p "Enter path to directory with compiled artifacts: " input_dir
    fi
  fi

  if [[ "$action" == "1" || "$action" == "3" ]]; then
    if [[ -d "$BUILD_DIR" ]]; then
      if prompt_yes_no "Build folder '$BUILD_DIR' exists. Overwrite/clean?" "y"; then rm -rf "$BUILD_DIR"; else echo "Aborting."; exit 1; fi
    fi
    banner; echo "Compiling: $sketch"
    set +e; arduino-cli compile --fqbn "$fqbn" --output-dir "$BUILD_DIR" "$sketch"; rc=$?; if [[ $rc -ne 0 ]]; then echo "Retry compile..."; arduino-cli compile --fqbn "$fqbn" --output-dir "$BUILD_DIR" "$sketch" || { echo "Compile failed."; exit 1; }; fi; set -e
    input_dir="$BUILD_DIR"
    echo "Build artifacts in: $input_dir"
  fi

  if [[ "$action" == "2" || "$action" == "3" ]]; then
    banner; echo "Uploading via $method to $port"
    set +e
    arduino-cli upload -p "$port" --fqbn "$fqbn" --input-dir "$input_dir"; rc=$?
    if [[ $rc -ne 0 ]]; then echo "Upload failed (rc=$rc). Close conflicting apps and press Enter to retry..."; read -r _; arduino-cli upload -p "$port" --fqbn "$fqbn" --input-dir "$input_dir" || { echo "Upload failed again."; exit 1; }; fi
    set -e; echo "Upload completed."
  fi

  banner
  if prompt_yes_no "Open serial monitor now?" "Y"; then open_serial_monitor "$port"; fi
}

main "$@"

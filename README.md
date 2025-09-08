# HV Trigger Async (ESP8266)

A small, self-hosted controller that safely arms and triggers a high-voltage discharge device via a networked HTML UI and WebSocket API. Behavior and timing follow PROJECT_SPEC.txt.

## Features
- SoftAP at 10.11.12.1/24 with minimal UI and /ws control.
- State machine with ARMED gating and auto-disarm.
- Modes: single pulse or buzz (10 sub-pulses), runtime-configurable.
- Non-blocking fire engine and periodic telemetry (~250 ms).
- OTA updates on port 3232.

## Pins (ESP8266)
- TRIGGER_OUT: GPIO16 (D0)
- LED_WAIT_AMBER: GPIO14 (D5)
- LED_READY_GREEN: GPIO15 (D8)
- LED_PULSE_BLUE: GPIO13 (D7)
- LED_ARMED_RED: GPIO12 (D6)

## Build & Upload
- Arduino CLI:
  - `scripts/build.ps1 -Upload -Port COM3` (Windows) or `scripts/build.sh upload /dev/ttyUSB0` (macOS/Linux)
  - OTA: `scripts/ota.ps1` or `scripts/ota.sh 10.11.12.1`
- PlatformIO:
  - `pio run -t upload` (firmware), `pio run -t uploadfs` (LittleFS UI)

## WebSocket API
- Telemetry (push): `{ type:"state", armed, pulseActive, pageCount, wifiClients, wifiConnected, staConnected, staIP, adc, cfg:{mode,width,spacing,repeat} }`
- Commands:
  - Arm: `{ "cmd":"arm", "on": true|false }`
  - Configure (not while armed): `{ "cmd":"cfg", "mode":"single|buzz", "width":5..100, "spacing":10..100, "repeat":1..4 }`
  - Fire (armed only): `{ "cmd":"fire" }`

## Notes
- GPIO15 is a boot-strap pin; ensure external circuitry preserves LOW at boot.
- Keep firing engine independent of networking for safety and timing integrity.


# HV Trigger Async (ESP32‑S3)

Firmware for an ESP32‑S3 that hosts a minimal web UI to safely arm and trigger a high‑voltage discharge device. Uses AsyncTCP + ESPAsyncWebServer with a WebSocket control channel and persists configuration via Preferences.

## Features
- SoftAP: SSID `Trigger-Remote` (default pass `lollipop`), IP `10.11.12.1`.
- Inline UI served from PROGMEM; no filesystem needed.
- WebSocket at `/ws` for live telemetry and control (250 ms updates).
- Configurable pulse: mode `single` or `buzz`, width, spacing, repeat.
- LED indicators: amber/green (network/WS), red (armed), blue (pulse).

## Quickstart
1. Install ESP32 boards (3.x) and libraries: AsyncTCP, ESPAsyncWebServer, ArduinoJson.
2. In Arduino IDE select board: ESP32S3 Dev Module (enable USB CDC).
3. Open `hv_trigger_async.ino` and Upload.
4. Join Wi‑Fi AP `Trigger-Remote` and browse `http://10.11.12.1/`.

## Controls & Ranges
- Mode: `single` or `buzz`.
- Width: 5–100 ms. Spacing: 10–100 ms. Repeat: 1–4.
- Arm/Disarm gates firing; config changes are locked while armed.

## Code Map
- `hv_trigger_async.ino`: setup/loop, telemetry tick, indicators.
- `web_server.cpp/.h`: Wi‑Fi AP, async server, WebSocket, inline UI, actions.
- `config.h`: pins, SoftAP settings, defaults.

See `README.build.md` for detailed build/flash notes and `library-notes.txt` for dependencies and protocol.

## Safety
Change the default SoftAP password before field use. Treat the trigger output as live hardware—validate with a dummy load first.

# HV Trigger Async (ESP32 / ESP32‑S3)

Firmware for ESP32 family boards hosting a minimal web UI to safely arm and trigger a high‑voltage initiator. AsyncTCP + ESPAsyncWebServer power a WebSocket control channel; configuration persists via Preferences.

## Features
- SoftAP: `Trigger-Remote` / `lollipop`, IP `10.11.12.1`.
- Inline UI from PROGMEM; no filesystem required.
- WebSocket at `/ws` for telemetry (~250 ms) and commands.
- Pulse controls: mode `single` or `buzz`, width, spacing, repeat.
- Hardware LEDs: amber/green (network/WS), red (armed), blue (pulse).
- Status bar: WS and Armed LEDs, mode label (BUZZ/SINGLE‑SHOT), compact `W/S/R` values (e.g., `31/38/3`), AP name; reconnect overlay while WS is down.
- OTA updates via ArduinoOTA (TCP/3232) — update while the app is running.
- Verbose Serial logs for boot, prefs, HTTP, WS, and actions.

## Quickstart
1. Dependencies: ESP32 core (recommended 2.0.14), libraries in `library-notes.txt`.
2. Build & Burn scripts (recommended):
   - Windows: `buildAndBurn.ps1` (or `buildAndBurn.bat` wrapper)
   - macOS/Linux: `chmod +x buildAndBurn.sh && ./buildAndBurn.sh`
   Scripts prompt for sketch, FQBN, clean, port selection, upload, and can open a serial monitor.
3. Manual (Arduino IDE):
   - ESP32 Dev Module (ESP32‑WROOM‑32): select Board "ESP32 Dev Module".
   - ESP32‑S3 Dev Module: select Board "ESP32S3 Dev Module" and enable USB CDC on boot.
   Then open `hv_trigger_async.ino` and Upload.
4. Connect to AP `Trigger-Remote` and open `http://10.11.12.1/`.
5. OTA Update (optional):
   - Ensure your PC and the device are on the same network (AP or STA).
   - Arduino IDE: use the Network Port entry if available.
   - Arduino CLI: `arduino-cli upload -p 10.11.12.1:3232 --fqbn esp32:esp32:esp32 hv_trigger_async.ino` (replace FQBN for ESP32‑S3).

## Controls & Ranges
- Mode: `single` or `buzz`.
- Width: 5–100 ms; Spacing: 10–100 ms; Repeat: 1–4.
- Arm/Disarm gates firing; cfg changes are ignored while armed.

## Build & Tools
- Scripts: see above. CLI fallback in `README.build.md`.
- Serial monitor: `arduino-cli monitor -p COMX -c baudrate=115200`.
- WebSocket test (PowerShell): `powershell -ExecutionPolicy Bypass -File tools\test_ws.ps1`

## API Reference
See `docs/WS_API.md` for WebSocket message formats, examples, and flows.

## Code Map
- `hv_trigger_async.ino`: setup/loop, telemetry tick, indicators.
- `web_server.cpp/.h`: AP setup, async server, WebSocket, inline UI, actions.
- `config.h`: pins, SoftAP settings, defaults; edit pins here if needed.

## Safety
Change the default SoftAP password before field use. Treat the trigger output as live hardware — validate with a dummy load first.


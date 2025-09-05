# HV Trigger Async (ESP32-S3)

Firmware for an ESP32S3 hosting a minimal web UI to safely arm and trigger a high-voltage initiator. AsyncTCP + ESPAsyncWebServer power a WebSocket control channel; configuration persists via Preferences.

## Features
- SoftAP: `Trigger-Remote` / `lollipop`, IP `10.11.12.1`.
- Inline UI from PROGMEM; no filesystem required.
- WebSocket at `/ws` for telemetry (~250 ms) and commands.
- Pulse controls: mode `single` or `buzz`, width, spacing, repeat.
- LEDs: amber/green (network/WS), red (armed), blue (pulse).
- Bottom status bar shows AP clients, WS status, armed, and config.
- Verbose Serial logs for boot, prefs, HTTP, WS, and actions.

## Quickstart
1. Dependencies: ESP32 core (recommended 2.0.14), libraries in `library-notes.txt`.
2. Build & Burn scripts (recommended):
   - Windows: `buildAndBurn.ps1` (or `buildAndBurn.bat` wrapper)
   - macOS/Linux: `chmod +x buildAndBurn.sh && ./buildAndBurn.sh`
   Scripts prompt for sketch, FQBN, clean, port selection, upload, and can open a serial monitor.
3. Manual (Arduino IDE): select Board "ESP32S3 Dev Module", enable USB CDC; open `hv_trigger_async.ino` and Upload.
4. Connect to AP `Trigger-Remote` and open `http://10.11.12.1/`.

## Controls & Ranges
- Mode: `single` or `buzz`.
- Width: 5-100 ms; Spacing: 10-100 ms; Repeat: 1-4.
- Arm/Disarm gates firing; cfg changes ignored while armed.

## Build & Tools
- Scripts: see above. CLI fallback in `README.build.md`.
- Serial monitor: `arduino-cli monitor -p COMX -c baudrate=115200`.

## API Reference
See `docs/WS_API.md` for full WebSocket message formats, examples, and flows.

## Code Map
- `hv_trigger_async.ino`: setup/loop, telemetry tick, indicators.
- `web_server.cpp/.h`: AP config, async server, WebSocket, inline UI, actions.
- `config.h`: pins, SoftAP settings, defaults.

## Safety
Change the default SoftAP password before field use. Treat the trigger output as live hardwareâ€”validate with a dummy load first.

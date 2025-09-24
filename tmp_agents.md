# Repository Guidelines

## Project Structure & Module Organization
- `perci.ino`: Arduino entrypoint (setup/loop, periodic telemetry).
- `web_server.cpp/.h`: Async web server, WebSocket, routes, and UI assets (served from PROGMEM).
- `config.h`: Pins, SoftAP settings, and default timing values.
- `README.build.md`, `library-notes.txt`: Board, library, and flashing notes.

## Build, Test, and Development Commands
- Arduino IDE:
  - ESP32 Dev Module (ESP32â€‘WROOMâ€‘32): select Board `ESP32 Dev Module`.
  - ESP32â€‘S3 Dev Module: select Board `ESP32S3 Dev Module` and enable USB CDC.
  - Install AsyncTCP, ESPAsyncWebServer, ArduinoJson. Open `perci.ino` and Upload.
- Arduino CLI (examples):
  - Compile (ESP32): `arduino-cli compile --fqbn esp32:esp32:esp32 perci.ino`
  - Compile (ESP32â€‘S3): `arduino-cli compile --fqbn esp32:esp32:esp32s3 perci.ino`
  - Upload: `arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32 perci.ino`
- Run locally: Not applicable (firmware). After flashing, join AP `Trigger-Remote` and browse `http://10.11.12.1/`.

## Coding Style & Naming Conventions
- Indentation: 2 spaces; braces on same line. Keep lines â‰² 100 chars.
- Naming: functions `lowerCamelCase` (e.g., `setupWiFiAP`); constants/macros `UPPER_SNAKE_CASE` (e.g., `PIN_LED_GREEN`); files `snake_case`.
- Includes: prefer local headers first (`"config.h"`) then libraries.
- Formatting tools: none enforced; match existing style and avoid trailing whitespace.

## Testing Guidelines
- No unit test harness. Perform device tests:
  - Join AP, load UI, verify telemetry updates, arm/disarm, and FIRE actions.
  - Observe LEDs: amber/green network state, red armed, blue pulse.
  - Monitor `Serial` at 115200 for diagnostics.
- Changes impacting timing: validate `TELEMETRY_PERIOD_MS`, pulse width, and buzz spacing on hardware.

## Commit & Pull Request Guidelines
- Commits: concise, imperative mood; group related changes. Prefer Conventional Commits (e.g., `feat: add buzz spacing control`).
- PRs: include summary, rationale, affected files, board/USB settings used, and test evidence (Serial logs, screenshots of UI). Reference related issues.

## Security & Configuration Tips
- Change default SoftAP password in `config.h` before field use.
- Review pin mappings for your ESP32â€‘S3 variant; avoid driving unintended lines.
- Treat the HV trigger output as live hardwareâ€”test with dummy loads first.


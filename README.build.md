# Build / Flash Notes

Recommended Stack
- ESP32 core: 2.0.14 (stable with current Async libs). Newer 3.x cores may require updated ESP32Async libraries.
- Libraries: Async TCP (ESP32Async), ESP Async WebServer, ArduinoJson (see `library-notes.txt`).

Scripts (preferred)
- Windows: `buildAndBurn.ps1` (or the `buildAndBurn.bat` wrapper)
- macOS/Linux: `chmod +x buildAndBurn.sh && ./buildAndBurn.sh`
  - Prompts for sketch, FQBN (default `esp32:esp32:esp32s3`), cleans build dir, compiles, lists ports for upload, retries once on errors, and can open a serial monitor.

Arduino IDE
- ESP32 Dev Module (ESP32‑WROOM‑32):
  - Tools > Board > ESP32 Arduino > ESP32 Dev Module
  - Open `hv_trigger_async.ino` and Upload.
- ESP32‑S3 Dev Module:
  - Tools > Board > ESP32 Arduino > ESP32S3 Dev Module
  - USB CDC On Boot: Enabled
  - Upload Mode: UART0 / Hardware CDC
  - Flash Size: per module (e.g., 8MB)
  - Open `hv_trigger_async.ino` and Upload.

Arduino CLI
- ESP32 Dev Module: `arduino-cli compile --fqbn esp32:esp32:esp32 hv_trigger_async.ino`
- ESP32‑S3 Dev Module: `arduino-cli compile --fqbn esp32:esp32:esp32s3 hv_trigger_async.ino`
- Upload (serial example): `arduino-cli upload -p COM9 --fqbn esp32:esp32:esp32 hv_trigger_async.ino`
- Upload (OTA example): `arduino-cli upload -p 10.11.12.1:3232 --fqbn esp32:esp32:esp32 hv_trigger_async.ino`
- Monitor: `arduino-cli monitor -p COM9 -c baudrate=115200`

Connect & Use
1. Join AP `Trigger-Remote` / pass `lollipop` (change in `config.h`).
2. Open `http://10.11.12.1/`. Use UI to set Mode/Width/Spacing/Repeat; Arm then FIRE.

Notes
- UI is served from PROGMEM; no filesystem needed.
- WebSocket at `/ws` sends telemetry ~every 250 ms and after commands. See `docs/WS_API.md`.
- OTA updates: device listens on TCP/3232; ensure your host and device share a network (AP or STA). In IDE, a Network Port may appear.

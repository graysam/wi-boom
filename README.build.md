# Build / Flash Notes

Recommended Stack
- ESP32 core: 3.3.x (latest stable). Validated with AsyncTCP 3.4.8, ESPAsyncWebServer 3.8.1, ArduinoJson 7.4.x.
- Libraries: AsyncTCP, ESPAsyncWebServer, ArduinoJson (see `library-notes.txt`).

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
- ESP32 Dev Module: `arduino-cli compile --fqbn esp32:esp32:esp32 perci.ino`
- ESP32‑S3 Dev Module: `arduino-cli compile --fqbn esp32:esp32:esp32s3 perci.ino`
- Upload (serial example): `arduino-cli upload -p COM9 --fqbn esp32:esp32:esp32 perci.ino`
- Upload (OTA example): `arduino-cli upload -p 10.11.12.1:3232 --fqbn esp32:esp32:esp32 perci.ino`
- Monitor: `arduino-cli monitor -p COM9 -c baudrate=115200`

Make (production‑oriented)
1. Install makeEspArduino: `git clone https://github.com/plerup/makeEspArduino.git ~/makeEspArduino`
2. From repo root:
   - ESP32: `make BOARD=esp32:esp32:esp32 CHIP=esp32` (build)
   - ESP32‑S3: `make BOARD=esp32:esp32:esp32s3 CHIP=esp32`
   - Flash: add `UPLOAD_PORT` (e.g., COM9) and target `flash`
     - Example: `make BOARD=esp32:esp32:esp32 CHIP=esp32 UPLOAD_PORT=COM9 flash`
   Notes:
   - The top‑level `Makefile` auto‑locates makeEspArduino at `tools/makeEspArduino/` or `~/makeEspArduino`. Override with `MAKEESPARDUINO=/path/to/makeEspArduino.mk`.
   - Use `make help` for available targets.

Connect & Use
1. Join AP `PeRci-Remote` / pass `lollipop` (change in `config.h`).
2. Open `http://10.11.12.1/`. On first boot a Setup page guides STA config and UI update; afterwards the main UI loads by default. Arm then FIRE.

Notes
- UI is served from SPIFFS (`/webroot/`), updated via Admin. Telemetry/WebSocket at `/ws` ~every 250 ms. OTA available on TCP/3232.

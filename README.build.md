# Build / Flash Notes

## Arduino IDE
- Tools > Board > ESP32 Arduino > ESP32S3 Dev Module
  - USB CDC On Boot: Enabled
  - Upload Mode: UART0 / CDC (match your wiring)
  - Flash Size: per your module (e.g., 8MB)
- Libraries (Library Manager): AsyncTCP, ESPAsyncWebServer, ArduinoJson
- Open `hv_trigger_async.ino` and Upload

## Arduino CLI (optional)
- Compile:
  `arduino-cli compile --fqbn esp32:esp32:esp32s3 hv_trigger_async.ino`
- Upload (replace `COM5` with your port):
  `arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32s3 hv_trigger_async.ino`

## Connect & Use
1. Join Wi‑Fi AP: SSID `Trigger-Remote` / pass `lollipop` (change in `config.h`).
2. Browse to `http://10.11.12.1/`.
3. Use the UI to set Mode, Width, Spacing, Repeat; Arm then FIRE.

Notes
- The UI is served from PROGMEM; no LittleFS required.
- Telemetry and commands use a WebSocket at `/ws` (~250 ms updates).

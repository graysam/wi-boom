# Build / Flash Notes

1. Arduino IDE: Tools → Board → ESP32 → ESP32S3 Dev Module
   - USB CDC On Boot: Enabled
   - Upload Mode: UART0 / CDC (match your setup)
   - Flash Size: per your module (e.g., 8MB)
2. Libraries: install AsyncTCP, ESPAsyncWebServer, ArduinoJson
3. Open `hv_trigger_async.ino` and **Upload**.
4. Join Wi‑Fi AP: SSID `Trigger-Remote` / pass `lollipop` (change in `config.h`).
5. Browse to `http://10.11.12.1/`.

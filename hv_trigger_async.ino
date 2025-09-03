// ============================================================================
// file: hv_trigger_async.ino
// ESP32-S3 (Arduino) — Wi-Fi HV Trigger (AsyncTCP + ESPAsyncWebServer + WS)
// Minimal UI: a single “FIRE” button + LED controls; websocket telemetry ready.
// ============================================================================

#include <Arduino.h>
#include "config.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);
  delay(100);

  // GPIO init
  pinMode(PIN_PULSE_OUT, OUTPUT);
  digitalWrite(PIN_PULSE_OUT, LOW);

  pinMode(PIN_LED_AMBER, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_PULSE, OUTPUT);
  pinMode(PIN_LED_ARMED, OUTPUT);

  pinMode(PIN_ADC_RESERVED, INPUT_PULLDOWN); // wired but currently unused

  setStatusLED(LED_OFF);

  // Bring up AP + web stack
  setupWiFiAP();
  initWeb(); // mounts handlers + websocket + static page

  Serial.println(F("System ready."));
}

void loop() {
  // Async server requires no busy loop; use millis for periodic state broadcast
  static uint32_t last = 0;
  const uint32_t now = millis();
  if (now - last >= TELEMETRY_PERIOD_MS) {
    last = now;
    broadcastState();  // push inputs/telemetry to any connected WS clients
  }
}

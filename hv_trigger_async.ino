// ============================================================================
// file: hv_trigger_async.ino
// ESP32-S3 (Arduino) â€” Wi-Fi HV Trigger with Async Web UI and persistent config.
// ============================================================================

#include <Arduino.h>
#include "config.h"
#include "web_server.h"
#include <ArduinoOTA.h>

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.setDebugOutput(true);
  Serial.println(F("Boot: HV Trigger Async starting"));

  // GPIO init
  pinMode(PIN_PULSE_OUT, OUTPUT);
  digitalWrite(PIN_PULSE_OUT, LOW);

  pinMode(PIN_LED_AMBER, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_PULSE, OUTPUT);
  pinMode(PIN_LED_ARMED, OUTPUT);
  Serial.println(F("GPIO: pins configured"));

  // ADC reserved pin removed on ESP32-CAM

  // Bring up AP/AP+STA + web stack
  setupWiFiAP();
  initWeb(); // mounts handlers + websocket + static page

  // OTA setup (TCP/3232)
  ArduinoOTA.onStart([](){ Serial.println(F("OTA: start")); });
  ArduinoOTA.onEnd([](){ Serial.println(F("OTA: end")); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t){
    Serial.printf("OTA: %u%%\n", (p*100)/t);
  });
  ArduinoOTA.onError([](ota_error_t e){ Serial.printf("OTA: error %u\n", e); });
  ArduinoOTA.begin();

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
  updateIndicators();
  ArduinoOTA.handle();
}

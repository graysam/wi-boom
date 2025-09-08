#include <Arduino.h>

#include "config.h"
#include "pins.h"
#include "state.h"
#include "storage.h"
#include "fire_engine.h"
#include "net.h"
#include "ota.h"
#include "web_ui.h"

static uint32_t last_telemetry_ms = 0;

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println(F("Boot: hv_trigger_async_ESP8266"));

  pins_setup();
  storage_init();
  state_init();
  state_load_or_defaults();

  net_setup();
  ota_setup();
  fire_engine_setup();

  Serial.println(F("Init complete."));
}

void loop() {
  // Non-blocking timing/state machines.
  fire_engine_loop();
  ota_loop();

  const uint32_t now = millis();
  if (now - last_telemetry_ms >= TELEMETRY_INTERVAL_MS) {
    last_telemetry_ms = now;
    net_broadcast_telemetry();
  }
}


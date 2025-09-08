#include "ota.h"
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include "pins.h"

void ota_setup() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.onStart([](){
    // Indicate OTA via LEDs
    digitalWrite(PIN_LED_WAIT_AMBER, HIGH);
  });
  ArduinoOTA.onEnd([](){
    digitalWrite(PIN_LED_WAIT_AMBER, LOW);
  });
  ArduinoOTA.onError([](ota_error_t){
    // Leave WAIT LED on to signal problem
  });
  ArduinoOTA.begin();
}

void ota_loop() { ArduinoOTA.handle(); }


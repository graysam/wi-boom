#pragma once

#include <Arduino.h>

// Pin mapping derived from pin_assignments.txt
// GPIO numbers are for ESP8266 (NodeMCU/Wemos D1 mini class boards)

// Trigger output
static const uint8_t PIN_TRIGGER_OUT   = 16; // D0

// LEDs
static const uint8_t PIN_LED_WAIT_AMBER  = 14; // D5
static const uint8_t PIN_LED_READY_GREEN = 15; // D8 (note: boot strap pin)
static const uint8_t PIN_LED_PULSE_BLUE  = 13; // D7
static const uint8_t PIN_LED_ARMED_RED   = 12; // D6

inline void pins_setup() {
  pinMode(PIN_TRIGGER_OUT, OUTPUT);
  digitalWrite(PIN_TRIGGER_OUT, LOW);

  pinMode(PIN_LED_WAIT_AMBER, OUTPUT);
  pinMode(PIN_LED_READY_GREEN, OUTPUT);
  pinMode(PIN_LED_PULSE_BLUE, OUTPUT);
  pinMode(PIN_LED_ARMED_RED, OUTPUT);

  digitalWrite(PIN_LED_WAIT_AMBER, LOW);
  digitalWrite(PIN_LED_READY_GREEN, LOW);
  digitalWrite(PIN_LED_PULSE_BLUE, LOW);
  digitalWrite(PIN_LED_ARMED_RED, LOW);
}


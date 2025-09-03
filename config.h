// ============================================================================
// file: config.h
// Shared configuration, pin map, and small helpers.
// ============================================================================

#pragma once
#include <Arduino.h>

// -------------------- Pin Assignments (ESP32-S3-WROOM-1 dev board) --------------------
static constexpr uint8_t PIN_PULSE_OUT   = 12; // HV trigger pulse output (active HIGH)
static constexpr uint8_t PIN_LED_AMBER   = 16; // Bi-colour LED half: AMBER = WiFi not connected
static constexpr uint8_t PIN_LED_GREEN   = 17; // Bi-colour LED half: GREEN = WiFi connected/ready
static constexpr uint8_t PIN_LED_PULSE   = 8;  // Blue LED: pulse active indicator
static constexpr uint8_t PIN_LED_ARMED   = 9;  // Red LED: armed & ready to fire
static constexpr uint8_t PIN_ADC_RESERVED= 11; // Reserved for future ADC input

// -------------------- Wi-Fi (SoftAP) --------------------
static constexpr char WIFI_AP_SSID[]     = "Trigger-Remote";
static constexpr char WIFI_AP_PASS[]     = "lollipop"; // change for field use

// SoftAP IPv4
static constexpr uint8_t AP_IP[4]        = {10,11,12,1};
static constexpr uint8_t AP_GW[4]        = {10,11,12,1};
static constexpr uint8_t AP_MASK[4]      = {255,255,255,0};

// -------------------- Behaviour --------------------
static constexpr uint32_t PULSE_WIDTH_MS      = 50;   // Adjust to suit your HV gate/driver
static constexpr uint32_t TELEMETRY_PERIOD_MS = 250;  // State push interval

// -------------------- LED helpers --------------------
enum LedStatus : uint8_t { LED_OFF=0, LED_GREEN=1, LED_AMBER=2 };

inline void setStatusLED(LedStatus s) {
  switch (s) {
    case LED_OFF:
      digitalWrite(PIN_LED_AMBER, LOW);
      digitalWrite(PIN_LED_GREEN, LOW);
      break;
    case LED_GREEN:
      digitalWrite(PIN_LED_AMBER, LOW);
      digitalWrite(PIN_LED_GREEN, HIGH);
      break;
    case LED_AMBER:
      digitalWrite(PIN_LED_AMBER, HIGH);
      digitalWrite(PIN_LED_GREEN, LOW);
      break;
  }
}

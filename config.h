// file: config.h
// Shared configuration, pin map, and small helpers.
// ============================================================================

#pragma once
#include <Arduino.h>

// -------------------- Pin Assignments (ESP32-CAM per pin_assignments.txt) --------------------
// Ribbon order mapping and colours:
//  - PULSE Signal:        GPIO16
//  - AMBER (WiFi wait):   GPIO14
//  - GREEN (WiFi ready):  GPIO15
//  - BLUE (Pulse active): GPIO13
//  - RED (Armed):         GPIO12
static constexpr uint8_t PIN_PULSE_OUT   = 16; // HV trigger pulse output (active HIGH)
static constexpr uint8_t PIN_LED_AMBER   = 14; // AMBER = WiFi not connected
static constexpr uint8_t PIN_LED_GREEN   = 15; // GREEN = WiFi connected/ready
static constexpr uint8_t PIN_LED_PULSE   = 13; // BLUE = pulse active indicator
static constexpr uint8_t PIN_LED_ARMED   = 12; // RED  = armed & ready to fire
// No ADC reserved on ESP32-CAM mapping

// -------------------- Wi-Fi (SoftAP) --------------------
static constexpr char WIFI_AP_SSID[]     = "Trigger-Remote";
static constexpr char WIFI_AP_PASS[]     = "lollipop"; // change for field use

// SoftAP IPv4
static constexpr uint8_t AP_IP[4]        = {10,11,12,1};
static constexpr uint8_t AP_GW[4]        = {10,11,12,1};
static constexpr uint8_t AP_MASK[4]      = {255,255,255,0};

// -------------------- Behaviour --------------------
// Default timing values; runtime configurable and persisted via Preferences
static constexpr uint32_t DEFAULT_PULSE_WIDTH_MS = 10;   // single pulse width
static constexpr uint32_t DEFAULT_BUZZ_SPACING_MS = 20;  // inter-pulse gap for buzz mode
static constexpr uint8_t  DEFAULT_BUZZ_REPEAT    = 1;    // number of buzz repetitions
static constexpr uint32_t TELEMETRY_PERIOD_MS    = 250;  // State push interval

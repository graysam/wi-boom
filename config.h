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
// Default timing values; runtime configurable and persisted via Preferences
static constexpr uint32_t DEFAULT_PULSE_WIDTH_MS = 10;   // single pulse width
static constexpr uint32_t DEFAULT_BUZZ_SPACING_MS = 20;  // inter-pulse gap for buzz mode
static constexpr uint8_t  DEFAULT_BUZZ_REPEAT    = 1;    // number of buzz repetitions
static constexpr uint32_t TELEMETRY_PERIOD_MS    = 250;  // State push interval

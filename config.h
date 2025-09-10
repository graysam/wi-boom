// file: config.h
// Shared configuration, pin map, and small helpers.
// ============================================================================

#pragma once
#include <Arduino.h>

// -------------------- Wi-Fi Mode --------------------
// When true, device runs in AP+STA mode and attempts to join STA_SSID/STA_PASS
// while also broadcasting its own SoftAP. When false, AP-only.
static constexpr bool WIFI_APSTA = true;
static constexpr char STA_SSID[] = "TimeBecomesALoop";     // set to join infrastructure Wi‑Fi (optional)
static constexpr char STA_PASS[] = "lollipop";

// -------------------- Pin Assignments --------------------
// Default mappings are chosen to be safe on the common dev kits.
// - For standard ESP32 Dev Module (ESP32-WROOM-32), use the provided GPIO set.
// - For ESP32-S3 (and other variants), use a conservative default compatible with prior builds.
// Pins are easily changed here as needed for your hardware.

#if defined(CONFIG_IDF_TARGET_ESP32)
// ESP32 Dev Module (ESP32-WROOM-32)
// Available pins requested: GPIO27, GPIO26, GPIO25, GPIO33, GPIO32
//  - PULSE Signal:        GPIO27
//  - AMBER (WiFi wait):   GPIO26
//  - GREEN (WiFi ready):  GPIO25
//  - BLUE (Pulse active): GPIO33
//  - RED (Armed):         GPIO32
static constexpr uint8_t PIN_PULSE_OUT   = 27; // HV trigger pulse output (active HIGH)
static constexpr uint8_t PIN_LED_AMBER   = 26; // AMBER = WiFi not connected
static constexpr uint8_t PIN_LED_GREEN   = 25; // GREEN = WiFi connected/ready
static constexpr uint8_t PIN_LED_PULSE   = 33; // BLUE = pulse active indicator
static constexpr uint8_t PIN_LED_ARMED   = 32; // RED  = armed & ready to fire

#else
// Fallback / prior mapping (ESP32-S3 dev kits and ESP32-CAM harness used previously)
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
#endif
// No ADC reserved in current mappings

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

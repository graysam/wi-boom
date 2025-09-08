#pragma once

#include <Arduino.h>

enum class FireMode : uint8_t { Single = 0, Buzz = 1 };

struct Config {
  FireMode mode;
  uint8_t width_ms;           // 5..100
  uint8_t spacing_ms;         // 10..100
  uint8_t repeat;             // 1..4
  uint16_t repeat_interval_ms;// 50..1000
};

struct DeviceState {
  bool armed;
  bool firing;
  bool ws_connected;
  uint8_t wifi_clients;
  bool sta_connected;
  IPAddress sta_ip;
  uint32_t page_count;
  uint16_t adc_value; // reserved (0 if unused)
  Config cfg;
};

void state_init();
void state_load_or_defaults();
const DeviceState& state_get();
bool state_set_cfg(const Config& cfg); // ignored when armed
void state_set_ws_connected(bool on);
void state_set_wifi_clients(uint8_t n);
void state_set_sta(bool connected, IPAddress ip);
bool state_arm(bool on); // on=true arm, false disarm
void state_set_firing(bool on);


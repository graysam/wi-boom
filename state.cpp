#include "state.h"
#include "config.h"

static DeviceState S;

void state_init() {
  memset(&S, 0, sizeof(S));
}

void state_load_or_defaults() {
  S.armed = false;
  S.firing = false;
  S.ws_connected = false;
  S.wifi_clients = 0;
  S.sta_connected = false;
  S.page_count = 0;
  S.adc_value = 0;

  S.cfg.mode = DEFAULT_MODE_BUZZ ? FireMode::Buzz : FireMode::Single;
  S.cfg.width_ms = DEFAULT_WIDTH_MS;
  S.cfg.spacing_ms = DEFAULT_SPACING_MS;
  S.cfg.repeat = DEFAULT_REPEAT;
  S.cfg.repeat_interval_ms = DEFAULT_REPEAT_INTERVAL;
}

const DeviceState& state_get() { return S; }

bool state_set_cfg(const Config& cfg) {
  if (S.armed) return false;
  S.cfg = cfg;
  return true;
}

void state_set_ws_connected(bool on) { S.ws_connected = on; }
void state_set_wifi_clients(uint8_t n) { S.wifi_clients = n; }
void state_set_sta(bool connected, IPAddress ip) {
  S.sta_connected = connected; S.sta_ip = ip;
}

bool state_arm(bool on) {
  if (on) {
    if (S.armed) return true;
    // Snapshot occurs implicitly via S.cfg immutability while armed.
    S.armed = true;
    return true;
  } else {
    S.armed = false;
    return true;
  }
}

void state_set_firing(bool on) { S.firing = on; }


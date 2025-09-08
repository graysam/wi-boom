#include "storage.h"
#include <Arduino.h>
#include <EEPROM.h>

// Minimal EEPROM-backed KV for cfg. Layout:
// [0]=magic(0x42) [1]=mode [2]=width [3]=spacing [4]=repeat [5..6]=repeat_interval

static const uint8_t MAGIC = 0x42;

void storage_init() {
  EEPROM.begin(32);
}

bool storage_save_cfg(const Config& c) {
  EEPROM.write(0, MAGIC);
  EEPROM.write(1, (uint8_t)c.mode);
  EEPROM.write(2, c.width_ms);
  EEPROM.write(3, c.spacing_ms);
  EEPROM.write(4, c.repeat);
  EEPROM.write(5, (uint8_t)(c.repeat_interval_ms & 0xFF));
  EEPROM.write(6, (uint8_t)(c.repeat_interval_ms >> 8));
  return EEPROM.commit();
}

bool storage_load_cfg(Config& out) {
  if (EEPROM.read(0) != MAGIC) return false;
  out.mode = (EEPROM.read(1) == 0) ? FireMode::Single : FireMode::Buzz;
  out.width_ms = EEPROM.read(2);
  out.spacing_ms = EEPROM.read(3);
  out.repeat = EEPROM.read(4);
  out.repeat_interval_ms = EEPROM.read(5) | (uint16_t(EEPROM.read(6)) << 8);
  return true;
}


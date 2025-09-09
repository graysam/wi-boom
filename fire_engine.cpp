#include "fire_engine.h"
#include "pins.h"

// Simple non-blocking state machine covering Single/Buzz sequences.
// Enforces guard timing for visibility/power integrity.

enum class FEState : uint8_t { Idle, SingleHigh, SingleLow, BuzzHigh, BuzzLow, BetweenReps };
static FEState F = FEState::Idle;
static uint32_t t_deadline = 0;
static uint8_t buzz_i = 0;      // sub-pulses (1..10)
static uint8_t rep_i = 0;       // repetitions

static inline void setPulse(bool on) {
  digitalWrite(PIN_TRIGGER_OUT, on ? HIGH : LOW);
  digitalWrite(PIN_LED_PULSE_BLUE, on ? HIGH : LOW);
}

void fire_engine_setup() {
  setPulse(false);
}

bool fire_engine_trigger() {
  const auto& S = state_get();
  if (!S.armed || S.firing) return false;
  state_set_firing(true);

  if (S.cfg.mode == FireMode::Single) {
    F = FEState::SingleHigh;
    setPulse(true);
    t_deadline = millis() + S.cfg.width_ms;
  } else { // Buzz
    F = FEState::BuzzHigh;
    rep_i = 1; buzz_i = 1;
    setPulse(true);
    t_deadline = millis() + S.cfg.width_ms;
  }
  return true;
}

void fire_engine_loop() {
  auto& S = const_cast<DeviceState&>(state_get());
  const uint32_t now = millis();
  const uint8_t width = S.cfg.width_ms;
  const uint8_t spacing = S.cfg.spacing_ms;
  const uint16_t rep_gap = S.cfg.repeat_interval_ms;

  switch (F) {
    case FEState::Idle: return;
    case FEState::SingleHigh:
      if (int32_t(now - t_deadline) >= 0) {
        setPulse(false);
        F = FEState::SingleLow;
        // enforce ~50ms spacing if width < 50ms
        const uint16_t guard = (width < 50) ? (50 - width) : 0;
        t_deadline = now + guard;
      }
      break;
    case FEState::SingleLow:
      if (int32_t(now - t_deadline) >= 0) {
        F = FEState::Idle;
        S.armed = false; // auto-disarm
        S.firing = false;
      }
      break;
    case FEState::BuzzHigh:
      if (int32_t(now - t_deadline) >= 0) {
        setPulse(false);
        // Determine required LOW time between successive HIGH edges
        const uint16_t guard = (width < 50) ? (50 - width) : 0;
        const uint16_t low_time = (buzz_i < 10) ? max<uint16_t>(spacing, guard) : 0;
        if (buzz_i < 10) {
          F = FEState::BuzzLow;
          t_deadline = now + low_time;
        } else {
          // Completed 10 sub-pulses in this repetition
          if (rep_i < S.cfg.repeat) {
            F = FEState::BetweenReps;
            t_deadline = now + rep_gap;
          } else {
            F = FEState::Idle;
            S.armed = false; // auto-disarm
            S.firing = false;
          }
        }
      }
      break;
    case FEState::BuzzLow:
      if (int32_t(now - t_deadline) >= 0) {
        buzz_i++;
        if (buzz_i <= 10) {
          F = FEState::BuzzHigh;
          setPulse(true);
          t_deadline = now + width;
        }
      }
      break;
    case FEState::BetweenReps:
      if (int32_t(now - t_deadline) >= 0) {
        rep_i++;
        buzz_i = 1;
        F = FEState::BuzzHigh;
        setPulse(true);
        t_deadline = now + width;
      }
      break;
  }
}


#pragma once

#include <Arduino.h>
#include "state.h"

void fire_engine_setup();
void fire_engine_loop();
bool fire_engine_trigger(); // returns false if not allowed


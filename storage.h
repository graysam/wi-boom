#pragma once

#include "state.h"

void storage_init();
bool storage_save_cfg(const Config& cfg);
bool storage_load_cfg(Config& cfg_out);


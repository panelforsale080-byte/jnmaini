#pragma once
#include <cstddef>

const char *attach_status_text();
void attach_log_copy(char *dst, size_t cap);
void attach_tick();
int attach_state();

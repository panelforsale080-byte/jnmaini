#pragma once
#include <cstddef>

const char *attach_status_text();
void attach_log_copy(char *dst, size_t cap);
void attach_log_line(const char *line);
void attach_hint_pid(int pid, const char *cmd);
void attach_tick();
int attach_state();

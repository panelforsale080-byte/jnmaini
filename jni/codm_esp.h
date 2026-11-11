#pragma once
#include <sys/types.h>

struct ImDrawList;

void codm_esp_reset();
void codm_esp_resolve(pid_t pid);
void codm_esp_draw(ImDrawList *draw, int width, int height, bool enable, bool line, bool box,
                   bool name, bool health, bool distance, bool counter);

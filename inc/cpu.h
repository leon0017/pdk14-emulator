#pragma once

#include <main.h>

#define CLOCK_MONOTONIC      1

#define DEFAULT_TARGET_FREQ  (8 * 1000000) // 8 MHZ
#define SECOND_IN_NANOS      1000000000.0 

void cpu_set_freq(u32 hz);
void cpu_clock_loop(void);

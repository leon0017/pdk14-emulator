#pragma once

#include <types.h>

#define PROGRAM_PATH "test_program/.output/TestProgram_PFS154.bin"

#define ROM_MAX 8192
#define RAM_MAX 512

typedef struct {
    u16 *rom;
    u16 *ram;
} CPU;

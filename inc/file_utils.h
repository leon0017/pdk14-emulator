#pragma once

#include <main.h>

typedef struct {
    u8 *program; // Program buffer
    i64 program_len; // Program buffer length in bytes
} Program;

Program fu_read_program(); // Read program from "PROGRAM_PATH"

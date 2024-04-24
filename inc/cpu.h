#pragma once

#include <main.h>

#define CLOCK_MONOTONIC      1

#define DEFAULT_TARGET_FREQ  (8 * 1000000) // 8 MHZ
#define SECOND_IN_NANOS      1000000000.0 

#define ROM_MAX_WORDS 2048
#define RAM_MAX_BYTES 128
#define IO_MAX_BYTES  64

// The definitions and the CPU struct is mainly from: https://github.com/free-pdk/fppa-pdk-tools
//
// MIT License
//
// Copyright (c) 2018 free-pdk
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define eF  (*((volatile uint8_t*)&cpu->io[0x00]))
#define eSP (*((volatile uint8_t*)&cpu->io[0x02]))
#define eA  cpu->a
#define ePC cpu->pc
#define eGINTEnabled       cpu->global_interrupts_enabled
#define eInterruptActive   cpu->interrupts_active
#define eT16               cpu->t16
#define eCycle             cpu->cpu_cycle

typedef struct {
    u16 *rom;
    u8 *ram;
    u8  *io;

    u16 rom_size_words;
    u16 ram_size_bytes;
    u8  io_size_bytes;

    int freq_pause_nanos;
    int freq_hz;

    u64 cpu_cycle; // Current CPU cycle

    u32 pc;  // Program counter
    u16 a;   // A register
    u16 t16; // Timer T16 value
    bool global_interrupts_enabled; // Are global interrupts enabled
    bool interrupts_active; // Internal status that cpu started interrupt
} CPU;

void cpu_set_freq(CPU *cpu, u32 hz);
void cpu_clock_loop(CPU *cpu);
u16 cpu_get_instruction(CPU *cpu, u16 addr);

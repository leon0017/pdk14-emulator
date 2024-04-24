#include <stdio.h>
#include <main.h>
#include <file_utils.h>
#include <cpu.h>
#include <stdlib.h>
#include <string.h>

void print_program_hex8(Program *program) {
    for (int64_t i = 0; i < program->program_len; i++) {
        u8 val = program->program[i];

        if (i % 16 == 0) {
            if (i == 0) {
                printf("|");
            } else {
                printf("|\n|");
            }
        }

        printf("%02X ", val);

        if (i == program->program_len - 1) {
            printf("|\n");
        }                
    }
}

// View intruction set here:
// https://free-pdk.github.io/instruction-sets/PDK14
int main() {
    Program program = fu_read_program();
    print_program_hex8(&program);

    CPU cpu = {
        .rom = (u16 *)malloc(ROM_MAX * sizeof(u16)),
        .ram = (u16 *)malloc(RAM_MAX * sizeof(u16))
    };

    memcpy(cpu.rom, program.program, program.program_len);

    cpu_set_freq(DEFAULT_TARGET_FREQ);
    cpu_clock_loop();
}
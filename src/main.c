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

void error(char *s) {
    printf("%s\n", s);
    exit(1);
}

// View intruction set here:
// https://free-pdk.github.io/instruction-sets/PDK14
int main() {
    Program program = fu_read_program();
    print_program_hex8(&program);

    CPU cpu = {
        .rom = (u16 *)malloc(ROM_MAX_WORDS * sizeof(u16)),
        .ram = (u8 *)malloc(RAM_MAX_BYTES  * sizeof(u8)),
        .io =  (u8  *)malloc(IO_MAX_BYTES  * sizeof(u8)),
        .rom_size_words = ROM_MAX_WORDS,
        .ram_size_bytes = RAM_MAX_BYTES,
        .io_size_bytes  = IO_MAX_BYTES,
    };

    memcpy(cpu.rom, program.program, program.program_len);

    // https://github.com/free-pdk/fppa-pdk-tools/blob/670e7bc9c1b33469fe8c80cd4c1eeb053d20c191/cpuvariant/pmx154.c#L167
    // Code fixups, TODO: implement for more chips.
    cpu.rom[0x07F6] = 0x0200; // RET 0
    cpu.rom[0x07FE] = 0x0200; // RET 0xXY - calibration code is NOT executed / RET 0xFF - calibration code is executed
    cpu.rom[0x07ED] = 0x0283; // IHRCR factory calibration
    cpu.rom[0x07EE] = 0x024a; // BGTR factory calibration

    // 1 instruction a second:
    // cpu_set_freq(&cpu, 1);

    cpu_set_freq(&cpu, DEFAULT_TARGET_FREQ);
    cpu_clock_loop(&cpu);
}
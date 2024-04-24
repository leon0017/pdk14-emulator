#include <stdio.h>
#include <main.h>
#include <file_utils.h>
#include <time.h>
#include <sys/select.h>

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

void print_program_b14(Program *program) {
    for (int64_t i = 0; i < program->program_len; i++) {
        //u8 val = program->program[i];

        // TODO: Print 14 bit binary number on each line
    }
}

#define CLOCK_MONOTONIC  1

#define TARGET_FREQ      (8 * 1000000) // 8 MHZ
#define SECOND_IN_NANOS  1000000000.0 
#define FREQ_PAUSE_NANOS (int)((1.0 / TARGET_FREQ) * SECOND_IN_NANOS)

// View intruction set here:
// https://free-pdk.github.io/instruction-sets/PDK14
int main() {
    Program program = fu_read_program();
    print_program_hex8(&program);

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    register u64 i = 0;
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        i64 elapsed_ns = (now.tv_sec - start.tv_sec) * 1000000000 + now.tv_nsec - start.tv_nsec;
        if (elapsed_ns >= FREQ_PAUSE_NANOS) {
            if (i % 8000000 == 0) {
                printf("Clock has cycled: %ld times\n", i);
            }
            i++;
            start = now;
        }
    }
}
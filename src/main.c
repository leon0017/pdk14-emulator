#include <stdio.h>
#include <main.h>
#include <file_utils.h>
#include <time.h>

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

#define CLOCK_MONOTONIC      1

#define DEFAULT_TARGET_FREQ  (8 * 1000000) // 8 MHZ
#define SECOND_IN_NANOS      1000000000.0 

int freq_pause_nanos = 0;
int freq_hz = 0;

void set_cpu_freq(u32 hz) {
    float freq_pause_nanos_f = (1.0 / hz) * SECOND_IN_NANOS;
    freq_pause_nanos = (int)freq_pause_nanos_f;
    freq_hz = hz;
}

// View intruction set here:
// https://free-pdk.github.io/instruction-sets/PDK14
int main() {
    Program program = fu_read_program();
    print_program_hex8(&program);

    set_cpu_freq(DEFAULT_TARGET_FREQ);

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    register u64 i = 0;
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        i64 elapsed_ns = (now.tv_sec - start.tv_sec) * 1000000000 + now.tv_nsec - start.tv_nsec;
        if (elapsed_ns >= freq_pause_nanos) {
            if (i % freq_hz == 0) {
                printf("Clock has cycled: %ld times\n", i);
            }
            i++;
            start = now;
        }
    }
}
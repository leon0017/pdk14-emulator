#include <cpu.h>
#include <time.h>
#include <stdio.h>

int freq_pause_nanos = 0;
int freq_hz = 0;

void cpu_set_freq(u32 hz) {
    float freq_pause_nanos_f = (1.0 / hz) * SECOND_IN_NANOS;
    freq_pause_nanos = (int)freq_pause_nanos_f;
    freq_hz = hz;
}

void cpu_clock_loop(void) {
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

#include <peripherals.h>
#include <stdio.h>

// Assumes that addr is within range
u8 peripheral_read(CPU *cpu, u8 addr) {
    // TODO: Actually read from peripheral
    return cpu->io[addr];
}

// Assumes that addr is within range
void peripheral_write(CPU *cpu, u8 addr, u8 data) {
    // TODO: Actually write to peripheral
    printf("WROTE TO PERIPHERAL: %d\n", data);
    cpu->io[addr] = data;
}

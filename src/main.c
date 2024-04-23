#include <stdio.h>
#include <main.h>
#include <file_utils.h>

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

// View intruction set here:
// https://free-pdk.github.io/instruction-sets/PDK14
int main() {
    Program program = fu_read_program();
    print_program_hex8(&program); 
    return 0;
}
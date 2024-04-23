#include <file_utils.h>
#include <stdio.h>
#include <stdlib.h>

Program fu_read_program() {
    FILE *file_ptr = fopen(PROGRAM_PATH, "rb");
    fseek(file_ptr, 0, SEEK_END);
    int64_t file_len = ftell(file_ptr);
    rewind(file_ptr);

    u8 *program_buffer = malloc(file_len * sizeof(u8));
    fread(program_buffer, file_len, 1, file_ptr);
    fclose(file_ptr);

    Program program = {
        .program = program_buffer,
        .program_len = file_len
    };

    return program;
}

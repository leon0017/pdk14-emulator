
SRC_DIR := src
OBJ_DIR := obj
INC_DIR := inc
OUTPUT  := $(OBJ_DIR)/pdk14-emulator

C_FILES := $(shell find $(SRC_DIR) -type f -name '*.c')

.PHONY: all clean link run test_program

all: run

run: link
	chmod +x $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -rf $(OBJ_DIR)
	make -C test_program clean

build_test_program:
	$(MAKE) -C test_program

link: build_test_program
	@mkdir -p $(OBJ_DIR)
	@echo " CC $(C_FILES)"
	@cc $(C_FILES) -I $(INC_DIR) -std=gnu99 -Wpedantic -Werror -Wall -o $(OUTPUT)

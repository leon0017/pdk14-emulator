
SRC_DIR := src
OBJ_DIR := obj
INC_DIR := inc
OUTPUT  := $(OBJ_DIR)/pdk14-emulator

C_FILES := $(shell find $(SRC_DIR) -type f -name '*.c')

.PHONY: all clean link run

all: run

run: link
	chmod +x $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -rf $(OBJ_DIR)

link:
	@echo " CC $(C_FILES)"
	@cc $(C_FILES) -I $(INC_DIR) -Wpedantic -Werror -Wall -o $(OUTPUT)

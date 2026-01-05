# Makefile for LVM4 Project
# Targets: build, assemble, test, clean

# Compiler and Flags
CC = gcc
CFLAGS = -m64 -Wall -Wextra -g
PYTHON = python3

# Files
VM_SRC = main.c
ASM_SRC = lvm.s
EXE = lvm_vm
ASSEMBLER = asm.py

# Directories
PROG_DIR = programs
BIN_DIR = bin

.PHONY: all build assemble test clean

all: build assemble test

# 1. Build the VM (Links C harness and Assembly engine)
build: $(EXE)

$(EXE): $(VM_SRC) $(ASM_SRC)
	$(CC) $(CFLAGS) -no-pie $(VM_SRC) $(ASM_SRC) -o $(EXE)

# 2. Assemble Demo Programs
# This assumes you have .lavm files in a programs/ directory
assemble:
	@mkdir -p $(BIN_DIR)
	@echo "Assembling LVM programs..."
	$(PYTHON) $(ASSEMBLER) $(PROG_DIR)/fib.lavm -o $(BIN_DIR)/fib.bin
	$(PYTHON) $(ASSEMBLER) $(PROG_DIR)/interrupt_test.lavm -o $(BIN_DIR)/int.bin
	$(PYTHON) $(ASSEMBLER) $(PROG_DIR)/alignment_test.lavm -o $(BIN_DIR)/align.bin
	@cp $(PROG_DIR)/illegal_test.bin $(BIN_DIR)/illegal.bin
	@cp $(PROG_DIR)/unaligned_test.bin $(BIN_DIR)/unaligned.bin

# 3. Run the Test Suite and Generate Coverage Report
test: build assemble
	@echo "Running Test Suite..."
	./$(EXE) $(BIN_DIR)/fib.bin
	./$(EXE) $(BIN_DIR)/int.bin
	./$(EXE) $(BIN_DIR)/align.bin
	./$(EXE) $(BIN_DIR)/illegal.bin
	./$(EXE) $(BIN_DIR)/unaligned.bin

# Cleanup
clean:
	rm -rf $(EXE) $(BIN_DIR)

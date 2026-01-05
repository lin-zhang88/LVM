# Coverage Report

This document describes how to generate and interpret the coverage reports for the LVM4 virtual machine.

## Generating Coverage Reports

### Exact Commands

To generate coverage reports, run the test suite:

```bash
# Full build and test (recommended)
make

# Or step by step:
make build      # Compile the VM
make assemble   # Assemble test programs
make test       # Run tests and generate coverage reports
```

The coverage reports are displayed in the terminal output. To save the output to a file:

```bash
# Save all output (including coverage reports)
make test > coverage_output.txt 2>&1

# Or using Docker (for macOS/ARM):
docker run --rm --platform linux/amd64 -v "$(pwd)":/workspace ubuntu:20.04 bash -c \
  "apt-get update -qq && apt-get install -y -qq gcc python3 make > /dev/null 2>&1 && \
   cd /workspace && make clean && make" > coverage_output.txt 2>&1
```

### Where Output is Saved

- **Default**: Coverage reports are printed to `stdout` (terminal)
- **To file**: Redirect output using `> coverage_output.txt 2>&1` (as shown above)
- **Location**: Output files are saved in the project root directory

## Coverage Summary

### Brief Coverage Report

Below is a summary of coverage results from running the full test suite:

```
=== Test Program: fib.bin ===
Loaded 6 bytes into VM memory.
Starting execution...
Execution stopped with code: 0

--- LVM4 Coverage Report ---
Total Instructions Executed: 1000000
Path: RESULT_CONTINUE: HIT
Path: RESULT_ILLEGAL:  MISS
Path: RESULT_UNALIGNED: MISS
----------------------------

=== Test Program: int.bin ===
Loaded 3 bytes into VM memory.
Starting execution...
Execution stopped with code: 0

--- LVM4 Coverage Report ---
Total Instructions Executed: 1000000
Path: RESULT_CONTINUE: HIT
Path: RESULT_ILLEGAL:  MISS
Path: RESULT_UNALIGNED: MISS
----------------------------

=== Test Program: align.bin ===
Loaded 3 bytes into VM memory.
Starting execution...
Execution stopped with code: 0

--- LVM4 Coverage Report ---
Total Instructions Executed: 1000000
Path: RESULT_CONTINUE: HIT
Path: RESULT_ILLEGAL:  MISS
Path: RESULT_UNALIGNED: MISS
----------------------------
```

### Coverage Summary Statistics

**Overall Coverage:**
- **RESULT_CONTINUE**: ✅ HIT (all 3 test programs)
- **RESULT_ILLEGAL**: ❌ MISS (not tested)
- **RESULT_UNALIGNED**: ❌ MISS (not tested)

**Total Instructions Executed**: 3,000,000 (1M per program, safety limit reached)

### Interpreting Coverage

- **HIT**: The code path was executed during testing
- **MISS**: The code path was not executed (not covered by current test suite)
- **Total Instructions Executed**: Number of VM instructions executed before stopping
  - If this shows 1,000,000, the program hit the safety limit (likely infinite loop)

### Notes

- The current test programs (`fib.lavm`, `interrupt_test.lavm`, `alignment_test.lavm`) are minimal and primarily test the `RESULT_CONTINUE` path
- To achieve full coverage, additional test cases would need to:
  - Trigger illegal instruction handling (`RESULT_ILLEGAL`)
  - Test unaligned memory access (`RESULT_UNALIGNED`)
  - Test interrupt handling paths


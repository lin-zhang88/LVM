***LVM4: Lightweight Virtual Machine (Milestone 4)***
*Project Overview*

This repository contains a complete implementation of the LVM4 specification. It features a 16-bit virtual machine engine written in x86 assembly (AT&T syntax), a custom assembler, and a test suite designed for full code-path coverage.
*Components*

    lvm.s: The VM core. It implements the step() function, handling 16-bit ALU operations, complex memory addressing, and interrupt logic.

    asm.py: A Python-based assembler that translates LVM assembly syntax into binary machine code, including the specific reg/mod byte encoding.

    main.c: A C test harness that loads binaries, executes the VM, and generates a coverage report.

    Makefile: Automates the build, assembly, and testing process end-to-end.

*Design Notes*
1. Memory Access & Addressing

The VM implements the full reg/mod table for Effective Address (EA) calculation:

    Modes: Supports Register, BP-relative, SP-relative, and Immediate-relative addressing.

    Side Effects: Includes logic for Pre-decrement (D bit) and Post-increment (I bit) based on the operation size (W bit).

2. Interrupts and IRET

The step() function checks for pending interrupts before fetching instructions. If an interrupt is triggered:

    The current Flags and IP are pushed onto the stack (R7).

    The VM jumps to the address specified in the Interrupt Vector Table at 0xFFF0.

    The RET instruction acts as an IRET when IE=0, restoring the saved flags from the stack.

**How to Build and Run**
*Prerequisites*

    Linux x86-64 environment

    gcc (GNU Compiler Collection)

    Python 3

    make

*Instructions*

    Clone the repository and navigate to the project root.

    Build and Test everything:
    ```bash
    make
    ```

    Or step by step:
    ```bash
    make build      # Compile the VM
    make assemble   # Assemble test programs
    make test       # Run all tests
    ```

*## Test Programs*

The project includes three test programs:

- `fib.lavm`: Fibonacci sequence calculation
- `interrupt_test.lavm`: Tests interrupt handling
- `alignment_test.lavm`: Tests memory alignment checks

*## Output*

The test harness (`main.c`) will:
- Load the binary into VM memory
- Execute instructions until completion or error
- Display final register state
- Generate a coverage report showing which code paths were executed

*## Notes*

- The VM uses x86-64 assembly (AT&T syntax), so you need a Linux x86-64 environment
- The assembler (`asm.py`) supports a subset of LVM instructions
- Coverage tracking helps verify that all code paths in `lvm.s` are tested

*## Architecture Requirements*

**Important**: This project requires a Linux x86-64 environment. It will not compile on:
- ARM64 systems (Apple Silicon, ARM servers)
- macOS (native compilation)

### Using an x86-64 Emulator (Recommended for macOS/ARM)

If you're on macOS or ARM, you can use an x86-64 emulator:

**Using Docker with x86-64 emulation**:
```bash
# Build a Docker image with x86-64 Linux
docker run --rm --platform linux/amd64 -v "$(pwd)":/workspace ubuntu:20.04 bash -c \
  "apt-get update && apt-get install -y gcc python3 make && cd /workspace && make"
```

**Other Options**:
- **Colima**: Lightweight Docker alternative for macOS
- **UTM**: GUI-based QEMU frontend (easiest for full VMs)
- **QEMU**: Full system emulation
- **Native Linux x86-64**: Best performance, no emulation needed

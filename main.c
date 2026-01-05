#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Result codes from lvm.s */
#define RESULT_CONTINUE 0
#define RESULT_ILLEGAL  1
#define RESULT_UNALIGNED 2

/* VM state defined in lvm.s */
extern uint16_t vm_regs[8];
extern uint16_t vm_flags;
extern uint8_t vm_memory[65536];

/* The step function in lvm.s */
extern int step(int int_num);

/* Simple Coverage Tracker */
typedef struct {
    int hit_continue;
    int hit_illegal;
    int hit_unaligned;
    uint32_t total_cycles;
} coverage_t;

void print_coverage(coverage_t *cov) {
    printf("\n--- LVM4 Coverage Report ---\n");
    printf("Total Instructions Executed: %u\n", cov->total_cycles);
    printf("Path: RESULT_CONTINUE: %s\n", cov->hit_continue ? "HIT" : "MISS");
    printf("Path: RESULT_ILLEGAL:  %s\n", cov->hit_illegal  ? "HIT" : "MISS");
    printf("Path: RESULT_UNALIGNED: %s\n", cov->hit_unaligned ? "HIT" : "MISS");
    printf("----------------------------\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <binary_file>\n", argv[0]);
        return 1;
    }

    /* 1. Load the Binary into vm_memory */
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Failed to open binary");
        return 1;
    }
    size_t bytes_read = fread(vm_memory, 1, 65536, f);
    fclose(f);
    printf("Loaded %zu bytes into VM memory.\n", bytes_read);

    /* 2. Setup IVT (Optional: for LVM4 interrupt tests) */
    // Example: vm_memory[0xFFF0] = 0x00; vm_memory[0xFFF1] = 0x10; (Jump to 0x1000)

    /* 3. Execution Loop */
    coverage_t cov = {0};
    int res = RESULT_CONTINUE;
    
    printf("Starting execution...\n");
    while (res == RESULT_CONTINUE && cov.total_cycles < 1000000) {
        /* Calling step with INT_NONE (-1) */
        res = step(-1); 
        
        cov.total_cycles++;
        if (res == RESULT_CONTINUE) cov.hit_continue = 1;
        if (res == RESULT_ILLEGAL)  cov.hit_illegal = 1;
        if (res == RESULT_UNALIGNED) cov.hit_unaligned = 1;
    }

    /* 4. Dump Final State */
    printf("\nExecution stopped with code: %d\n", res);
    printf("Final Register State:\n");
    for(int i=0; i<8; i++) {
        printf("R%d: 0x%04X  ", i, vm_regs[i]);
        if (i == 3) printf("\n");
    }
    printf("\nFlags: 0x%04X\n", vm_flags);

    print_coverage(&cov);

    return 0;
}
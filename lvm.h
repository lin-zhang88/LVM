/* Minimal LVM header - auto-generated */
#ifndef LVM_H
#define LVM_H

#include <stdint.h>

typedef struct {
    uint16_t reg[8];
    uint16_t flags;
    uint8_t memory[65536];
} vm_t;

#define RESULT_CONTINUE    0
#define RESULT_ILLEGAL     1
#define RESULT_UNALIGNED   2

#define FLAG_POSITIVE     1
#define FLAG_ZERO         2
#define FLAG_NEGATIVE     4
#define FLAG_INT_ENABLE   8

#define INT_NONE          -1

int step(vm_t *v, int int_num);

#endif /* LVM_H */

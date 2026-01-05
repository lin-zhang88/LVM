#include <stdint.h>
#include "lvm.h"

/* instruction stream word fetch: UNALIGNED OK */
static inline uint16_t fetch16(vm_t *v) {
    uint16_t ip = v->reg[0];
    uint16_t x  = (uint16_t)(v->memory[ip] | (v->memory[(uint16_t)(ip + 1)] << 8));
    v->reg[0]   = (uint16_t)(ip + 2);
    return x;
}

/* raw mem word helpers */
static inline uint16_t rd16_raw(vm_t *v, uint16_t a) {
    return (uint16_t)(v->memory[a] | (v->memory[(uint16_t)(a + 1)] << 8));
}
static inline void wr16_raw(vm_t *v, uint16_t a, uint16_t x) {
    v->memory[a] = (uint8_t)(x & 0xFF);
    v->memory[(uint16_t)(a + 1)] = (uint8_t)(x >> 8);
}

/* stack/data word access: MUST be aligned (used for data memory + IVT) */
static inline int rd16_align(vm_t *v, uint16_t a, uint16_t *out) {
    if (a & 1) return RESULT_UNALIGNED;
    *out = rd16_raw(v, a);
    return RESULT_CONTINUE;
}
static inline int wr16_align(vm_t *v, uint16_t a, uint16_t x) {
    if (a & 1) return RESULT_UNALIGNED;
    wr16_raw(v, a, x);
    return RESULT_CONTINUE;
}

static inline void set_nzp(vm_t *v, uint16_t r) {
    uint16_t ie = (uint16_t)(v->flags & FLAG_INT_ENABLE);
    uint16_t nzp = (r == 0) ? FLAG_ZERO : (r & 0x8000) ? FLAG_NEGATIVE : FLAG_POSITIVE;
    v->flags = (uint16_t)(ie | nzp);
}

static inline int16_t sext5(uint8_t x) {
    x &= 0x1F;
    return (int16_t)((x & 0x10) ? (0xFFE0 | x) : x);
}

/* reg/mod EA (W=bit5, I=bit4, D=bit3, NNN=bits2..0) */
static inline uint16_t ea(vm_t *v, uint8_t rm, uint16_t sz) {
    uint16_t *R = v->reg;
    uint8_t mode = (uint8_t)(rm >> 6);
    uint8_t n    = (uint8_t)(rm & 7);

    if (rm & 0x08) R[n] = (uint16_t)(R[n] - sz); /* pre-dec */

    uint16_t base = 0;
    if (mode == 1) base = R[6];            /* BP */
    else if (mode == 2) base = R[7];       /* SP */
    else if (mode == 3) base = fetch16(v); /* imm word (unaligned OK) */

    uint16_t addr = (uint16_t)(base + R[n]);

    if (rm & 0x10) R[n] = (uint16_t)(R[n] + sz); /* post-inc */

    return addr;
}

int step(vm_t *v, int int_num) {
    uint16_t *R = v->reg;
    uint8_t  *M = v->memory;

    /* spec: SP starts at FFF0 */
    if (R[7] == 0) R[7] = 0xFFF0;

    /* ---------- interrupts ---------- */
    if (int_num != INT_NONE && (v->flags & FLAG_INT_ENABLE)) {

        /* FIX: precise exception — if SP is odd, fail without changing SP */
        if (R[7] & 1) return RESULT_UNALIGNED;

        /* push FLAGS then IP */
        R[7] = (uint16_t)(R[7] - 2);
        wr16_raw(v, R[7], v->flags);

        R[7] = (uint16_t)(R[7] - 2);
        wr16_raw(v, R[7], R[0]);

        v->flags = (uint16_t)(v->flags & ~FLAG_INT_ENABLE);

        /* IVT at end of memory; first entry at FFF0 */
        {
            uint16_t isr;
            int rc = rd16_align(v, (uint16_t)(0xFFF0 + ((uint16_t)int_num << 1)), &isr);
            if (rc) return rc;
            R[0] = isr;
        }
        return RESULT_CONTINUE;
    }

    /* ---------- fetch/decode ---------- */
    uint8_t opb = M[R[0]++];
    uint8_t op  = (uint8_t)(opb >> 6);
    uint8_t d   = (uint8_t)((opb >> 3) & 7);
    uint8_t s   = (uint8_t)(opb & 7);

    /* ---------- DDD==000 : control/memory ---------- */
    if (d == 0) {

        /* xx 000 000: ret / cjmp / jmp abs / call abs */
        if (s == 0) {

            if (op == 0) { /* ret */

                /* FIX: if SP odd, fail without changing SP */
                if (R[7] & 1) return RESULT_UNALIGNED;

                uint16_t ip = rd16_raw(v, R[7]);
                R[7] = (uint16_t)(R[7] + 2);
                R[0] = ip;

                /* return-from-interrupt restores flags too */
                if ((v->flags & FLAG_INT_ENABLE) == 0) {
                    /* still aligned because +2 preserves parity */
                    v->flags = rd16_raw(v, R[7]);
                    R[7] = (uint16_t)(R[7] + 2);
                }
                return RESULT_CONTINUE;
            }

            if (op == 1) { /* conditional jump relative */
                uint8_t co = M[R[0]++];
                uint8_t cond = (uint8_t)((co >> 5) & 7);
                if (cond & (uint8_t)(v->flags & 7))
                    R[0] = (uint16_t)((int16_t)R[0] + sext5(co));
                return RESULT_CONTINUE;
            }

            if (op == 2) { /* jmp absolute */
                R[0] = fetch16(v);
                return RESULT_CONTINUE;
            }

            /* op==3 call absolute */
            {
                uint16_t tgt = fetch16(v);

                /* FIX: if SP odd, fail without changing SP */
                if (R[7] & 1) return RESULT_UNALIGNED;

                R[7] = (uint16_t)(R[7] - 2);
                wr16_raw(v, R[7], R[0]); /* push return addr */
                R[0] = tgt;
                return RESULT_CONTINUE;
            }
        }

        /* memory ops: 00/01/10 000 SSS ; 11 illegal */
        if (op == 3) return RESULT_ILLEGAL;

        uint8_t rm = M[R[0]++];
        uint16_t sz = (rm & 0x20) ? 2u : 1u;
        uint16_t addr = ea(v, rm, sz);

        /* unaligned only for WORD data load/store (not lea) */
        if (sz == 2 && (addr & 1) && op != 2) return RESULT_UNALIGNED;

        if (op == 0) { /* load */
            if (sz == 2) {
                uint16_t x;
                int rc = rd16_align(v, addr, &x);
                if (rc) return rc;
                R[s] = x;
            } else {
                R[s] = (uint16_t)M[addr];
            }
            return RESULT_CONTINUE;
        }

        if (op == 1) { /* store */
            if (sz == 2) {
                int rc = wr16_align(v, addr, R[s]);
                if (rc) return rc;
            } else {
                M[addr] = (uint8_t)(R[s] & 0xFF);
            }
            return RESULT_CONTINUE;
        }

        /* lea */
        R[s] = addr;
        return RESULT_CONTINUE;
    }

    /* ---------- SSS==000 : imm/shift/not/neg ---------- */
    if (s == 0) {
        if (op == 0) { /* imm */
            R[d] = fetch16(v);
            set_nzp(v, R[d]);
            return RESULT_CONTINUE;
        }

        if (op == 1) { /* shift */
            uint8_t dc = M[R[0]++];
            uint8_t Dbit = (uint8_t)((dc >> 7) & 1);
            uint8_t Abit = (uint8_t)((dc >> 6) & 1);
            uint8_t Rbit = (uint8_t)((dc >> 5) & 1);
            uint8_t cnt  = (uint8_t)(dc & 0x1F);

            if (Rbit) {
                if (cnt > 7) return RESULT_ILLEGAL;
                cnt = (uint8_t)(R[cnt] & 0x1F);
            }

            uint16_t x = R[d];
            if (Dbit) x = Abit ? (uint16_t)((int16_t)x >> cnt) : (uint16_t)(x >> cnt);
            else      x = (uint16_t)(x << cnt);

            R[d] = x;
            set_nzp(v, x);
            return RESULT_CONTINUE;
        }

        if (op == 2) { /* not */
            R[d] = (uint16_t)(~R[d]);
            set_nzp(v, R[d]);
            return RESULT_CONTINUE;
        }

        /* neg */
        R[d] = (uint16_t)(0 - R[d]);
        set_nzp(v, R[d]);
        return RESULT_CONTINUE;
    }

    /* ---------- reg-reg ALU ---------- */
    if (op == 0) R[d] = R[s];
    else if (op == 1) R[d] = (uint16_t)(R[d] + R[s]);
    else if (op == 2) R[d] = (uint16_t)(R[d] & R[s]);
    else              R[d] = (uint16_t)(R[d] ^ R[s]);

    set_nzp(v, R[d]);
    return RESULT_CONTINUE;
}

/*
 * native.h: for C written as C rather than instruction by instruction.
 * The routine computes with C values and reads and writes memory through
 * rd8/wr8; the registers and flags the original leaves where they can be
 * seen (its exit, a yield, a callee that takes them) are set explicitly,
 * with these.
 */
#ifndef ALADDIN_NATIVE_H
#define ALADDIN_NATIVE_H

#include "machine.h"
#include "cpu_ops.h"

/* A byte of the direct page (D + off, bank 0) and of the data bank. */
static inline uint32_t dpa(uint16_t off) { return (uint16_t)(cpu.d + off); }
static inline uint32_t dba(uint16_t addr) { return (uint32_t)cpu.db << 16 | addr; }

/* A word of the direct page or the data bank. */
static inline uint16_t dp16(uint16_t off) { return rd16(dpa(off)); }
static inline void dp16_set(uint16_t off, uint16_t v) { wr16(dpa(off), v); }

/* The 24-bit pointer at a direct-page offset, and [dp],Y through it. */
static inline uint32_t dp_long(uint16_t off)
{
    return rd16(dpa(off)) | (uint32_t)rd8(dpa((uint16_t)(off + 2))) << 16;
}
static inline uint32_t dp_long_y(uint16_t off, uint16_t y)
{
    return (dp_long(off) + y) & 0xFFFFFFu;
}

/* A as an 8-bit load leaves it (the high byte kept), N and Z from it. */
static inline void a8_load(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* A as a 16-bit load leaves it, N and Z from it. */
static inline void a16_load(uint16_t v)
{
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
}

/* X or Y as an 8-bit load leaves it (index registers 8-bit: no high
   byte), N and Z from it. */
static inline void x8_load(uint8_t v) { cpu.x = v; cpu.p = nz8(cpu.p, v); }
static inline void y8_load(uint8_t v) { cpu.y = v; cpu.p = nz8(cpu.p, v); }

/* N and Z as a test of an 8-bit or 16-bit value leaves them. */
static inline void nz8_of(uint8_t v) { cpu.p = nz8(cpu.p, v); }
static inline void nz16_of(uint16_t v) { cpu.p = nz16(cpu.p, v); }

/* One flag set or cleared. */
static inline void flag_set(uint8_t bit, int on)
{
    cpu.p = on ? (uint8_t)(cpu.p | bit) : (uint8_t)(cpu.p & ~bit);
}

/* A compare's flags: N and Z from a - m, C set when a >= m. */
static inline void cmp8_of(uint8_t a, uint8_t m)
{
    cpu.p = nz8(cpu.p, (uint8_t)(a - m));
    flag_set(P_C, a >= m);
}

static inline void cmp16_of(uint16_t a, uint16_t m)
{
    cpu.p = nz16(cpu.p, (uint16_t)(a - m));
    flag_set(P_C, a >= m);
}

/* TSB and TRB of a byte with the bits in A: Z from A AND the byte, then
   the bits set or cleared. */
static inline void tsb_at(uint32_t addr, uint8_t bits)
{
    uint8_t v = rd8(addr);

    flag_set(P_Z, !(v & bits));
    wr8(addr, (uint8_t)(v | bits));
}

static inline void trb_at(uint32_t addr, uint8_t bits)
{
    uint8_t v = rd8(addr);

    flag_set(P_Z, !(v & bits));
    wr8(addr, (uint8_t)(v & ~bits));
}

/* BIT of A with a byte or word: N and V from the memory, Z from A AND it. */
static inline void bit8_of(uint8_t a, uint8_t m)
{
    flag_set(P_N, m & 0x80);
    flag_set(P_V, m & 0x40);
    flag_set(P_Z, !(a & m));
}

static inline void bit16_of(uint16_t a, uint16_t m)
{
    flag_set(P_N, m & 0x8000);
    flag_set(P_V, m & 0x4000);
    flag_set(P_Z, !(a & m));
}

/* INC and DEC of a byte or word in memory, N and Z from the result. */
static inline uint8_t inc8_at(uint32_t addr)
{
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    wr8(addr, v);
    nz8_of(v);
    return v;
}

static inline uint8_t dec8_at(uint32_t addr)
{
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    wr8(addr, v);
    nz8_of(v);
    return v;
}

static inline uint16_t inc16_at(uint32_t addr)
{
    uint16_t v = (uint16_t)(rd16(addr) + 1);

    wr16(addr, v);
    nz16_of(v);
    return v;
}

static inline uint16_t dec16_at(uint32_t addr)
{
    uint16_t v = (uint16_t)(rd16(addr) - 1);

    wr16(addr, v);
    nz16_of(v);
    return v;
}

/* A JSR to a routine that can wait (a task's): the return address `ret`
   (the JSR's last byte) on the stack while it runs, as the original's. */
static inline void call_frame(uint16_t ret, void (*fn)(void))
{
    op_push8((uint8_t)(ret >> 8));
    op_push8((uint8_t)ret);
    fn();
    cpu.s = (uint16_t)(cpu.s + 2);
}

#endif

/*
 * ops65.h: every 65816 instruction (native mode) as a C step on the
 * register file, for C drafted from the assembly (tools/aladdin/cdraft.py).
 *
 * Each step does exactly what its instruction does to the registers, the
 * flags and memory, at the width the name says (8 or 16: the accumulator's
 * for A's instructions, the index registers' for X's and Y's), so a draft
 * is behaviour exact before a person names anything. Effective addresses
 * are computed as the CPU does: direct page from D (bank 0), absolute from
 * DB, long as given; indexed modes carry into the bank.
 */
#ifndef ALADDIN_OPS65_H
#define ALADDIN_OPS65_H

#include "machine.h"
#include "cpu_ops.h"

/* ---- effective addresses ---- */

static inline uint32_t ea_dp(uint8_t o) { return (uint16_t)(cpu.d + o); }
static inline uint32_t ea_dpx(uint8_t o) { return (uint16_t)(cpu.d + o + cpu.x); }
static inline uint32_t ea_dpy(uint8_t o) { return (uint16_t)(cpu.d + o + cpu.y); }
static inline uint32_t ea_abs(uint16_t a) { return (uint32_t)cpu.db << 16 | a; }
static inline uint32_t ea_absx(uint16_t a) { return ((uint32_t)cpu.db << 16 | a) + cpu.x & 0xFFFFFF; }
static inline uint32_t ea_absy(uint16_t a) { return ((uint32_t)cpu.db << 16 | a) + cpu.y & 0xFFFFFF; }
static inline uint32_t ea_long(uint32_t a) { return a; }
static inline uint32_t ea_longx(uint32_t a) { return (a + cpu.x) & 0xFFFFFF; }
static inline uint32_t ea_dpi(uint8_t o) { return (uint32_t)cpu.db << 16 | rd16(ea_dp(o)); }
static inline uint32_t ea_dpiy(uint8_t o) { return ((uint32_t)cpu.db << 16 | rd16(ea_dp(o))) + cpu.y & 0xFFFFFF; }
static inline uint32_t ea_dpxi(uint8_t o) { return (uint32_t)cpu.db << 16 | rd16(ea_dpx(o)); }
static inline uint32_t ea_dpil(uint8_t o) { return rd24(ea_dp(o)); }
static inline uint32_t ea_dpily(uint8_t o) { return (rd24(ea_dp(o)) + cpu.y) & 0xFFFFFF; }
static inline uint32_t ea_sr(uint8_t o) { return (uint16_t)(cpu.s + o); }
static inline uint32_t ea_sriy(uint8_t o) { return ((uint32_t)cpu.db << 16 | rd16(ea_sr(o))) + cpu.y & 0xFFFFFF; }

/* ---- small pieces ---- */

static inline void set_al(uint8_t v) { cpu.a = (uint16_t)((cpu.a & 0xFF00) | v); }
static inline uint8_t al(void) { return (uint8_t)cpu.a; }
static inline void fl8(uint8_t v) { cpu.p = nz8(cpu.p, v); }
static inline void fl16(uint16_t v) { cpu.p = nz16(cpu.p, v); }
static inline uint8_t cmpf8(uint8_t r, uint8_t m)
{
    return (uint8_t)((nz8(cpu.p, (uint8_t)(r - m)) & ~P_C) | (r >= m ? P_C : 0));
}
static inline uint8_t cmpf16(uint16_t r, uint16_t m)
{
    return (uint8_t)((nz16(cpu.p, (uint16_t)(r - m)) & ~P_C) | (r >= m ? P_C : 0));
}

/* ---- loads and stores ---- */

static inline void lda8_v(uint8_t v) { set_al(v); fl8(v); }
static inline void lda16_v(uint16_t v) { cpu.a = v; fl16(v); }
static inline void lda8(uint32_t ea) { lda8_v(rd8(ea)); }
static inline void lda16(uint32_t ea) { lda16_v(rd16(ea)); }
static inline void ldx8_v(uint8_t v) { cpu.x = v; fl8(v); }
static inline void ldx16_v(uint16_t v) { cpu.x = v; fl16(v); }
static inline void ldx8(uint32_t ea) { ldx8_v(rd8(ea)); }
static inline void ldx16(uint32_t ea) { ldx16_v(rd16(ea)); }
static inline void ldy8_v(uint8_t v) { cpu.y = v; fl8(v); }
static inline void ldy16_v(uint16_t v) { cpu.y = v; fl16(v); }
static inline void ldy8(uint32_t ea) { ldy8_v(rd8(ea)); }
static inline void ldy16(uint32_t ea) { ldy16_v(rd16(ea)); }
static inline void sta8(uint32_t ea) { wr8(ea, al()); }
static inline void sta16(uint32_t ea) { wr16(ea, cpu.a); }
static inline void stx8(uint32_t ea) { wr8(ea, (uint8_t)cpu.x); }
static inline void stx16(uint32_t ea) { wr16(ea, cpu.x); }
static inline void sty8(uint32_t ea) { wr8(ea, (uint8_t)cpu.y); }
static inline void sty16(uint32_t ea) { wr16(ea, cpu.y); }
static inline void stz8(uint32_t ea) { wr8(ea, 0); }
static inline void stz16(uint32_t ea) { wr16(ea, 0); }

/* ---- arithmetic and logic on A (value given) ---- */

static inline void adc8_v(uint8_t m) { set_al(op_adc8(al(), m, &cpu.p)); }
static inline void adc16_v(uint16_t m) { cpu.a = op_adc16(cpu.a, m, &cpu.p); }
static inline void sbc8_v(uint8_t m) { set_al(op_sbc8(al(), m, &cpu.p)); }
static inline void sbc16_v(uint16_t m) { cpu.a = op_sbc16(cpu.a, m, &cpu.p); }
static inline void and8_v(uint8_t m) { set_al(al() & m); fl8(al()); }
static inline void and16_v(uint16_t m) { cpu.a &= m; fl16(cpu.a); }
static inline void ora8_v(uint8_t m) { set_al(al() | m); fl8(al()); }
static inline void ora16_v(uint16_t m) { cpu.a |= m; fl16(cpu.a); }
static inline void eor8_v(uint8_t m) { set_al(al() ^ m); fl8(al()); }
static inline void eor16_v(uint16_t m) { cpu.a ^= m; fl16(cpu.a); }
static inline void cmp8_v(uint8_t m) { cpu.p = cmpf8(al(), m); }
static inline void cmp16_v(uint16_t m) { cpu.p = cmpf16(cpu.a, m); }
static inline void cpx8_v(uint8_t m) { cpu.p = cmpf8((uint8_t)cpu.x, m); }
static inline void cpx16_v(uint16_t m) { cpu.p = cmpf16(cpu.x, m); }
static inline void cpy8_v(uint8_t m) { cpu.p = cmpf8((uint8_t)cpu.y, m); }
static inline void cpy16_v(uint16_t m) { cpu.p = cmpf16(cpu.y, m); }
/* BIT memory: N and V from it, Z from A AND it; BIT #: Z only */
static inline void bit8_v(uint8_t m)
{
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | (m & 0xC0) | ((al() & m) ? 0 : P_Z));
}
static inline void bit16_v(uint16_t m)
{
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | ((m >> 8) & 0xC0) | ((cpu.a & m) ? 0 : P_Z));
}
static inline void bit8_i(uint8_t m) { cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((al() & m) ? 0 : P_Z)); }
static inline void bit16_i(uint16_t m) { cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & m) ? 0 : P_Z)); }

/* ---- shifts and steps on a value: return the result, set the flags ---- */

static inline uint8_t asl8_r(uint8_t v) { cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7)); v = (uint8_t)(v << 1); fl8(v); return v; }
static inline uint16_t asl16_r(uint16_t v) { cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 15)); v = (uint16_t)(v << 1); fl16(v); return v; }
static inline uint8_t lsr8_r(uint8_t v) { cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 1)); v >>= 1; fl8(v); return v; }
static inline uint16_t lsr16_r(uint16_t v) { cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 1)); v >>= 1; fl16(v); return v; }
static inline uint8_t rol8_r(uint8_t v)
{
    uint8_t c = (uint8_t)(v >> 7);
    v = (uint8_t)(v << 1 | (cpu.p & P_C));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | c); fl8(v); return v;
}
static inline uint16_t rol16_r(uint16_t v)
{
    uint8_t c = (uint8_t)(v >> 15);
    v = (uint16_t)(v << 1 | (cpu.p & P_C));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | c); fl16(v); return v;
}
static inline uint8_t ror8_r(uint8_t v)
{
    uint8_t c = (uint8_t)(v & 1);
    v = (uint8_t)(v >> 1 | (cpu.p & P_C) << 7);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | c); fl8(v); return v;
}
static inline uint16_t ror16_r(uint16_t v)
{
    uint8_t c = (uint8_t)(v & 1);
    v = (uint16_t)(v >> 1 | (cpu.p & P_C) << 15);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | c); fl16(v); return v;
}
static inline uint8_t inc8_r(uint8_t v) { v++; fl8(v); return v; }
static inline uint16_t inc16_r(uint16_t v) { v++; fl16(v); return v; }
static inline uint8_t dec8_r(uint8_t v) { v--; fl8(v); return v; }
static inline uint16_t dec16_r(uint16_t v) { v--; fl16(v); return v; }

/* The same on the accumulator, and on memory (one read-modify-write) */
#define ACC8(op) set_al(op##8_r(al()))
#define ACC16(op) (cpu.a = op##16_r(cpu.a))
#define MEM8(op, ea) do { uint32_t e_ = (ea); wr8(e_, op##8_r(rd8_rmw(e_))); } while (0)
#define MEM16(op, ea) do { uint32_t e_ = (ea); wr16_rmw(e_, op##16_r(rd16_rmw(e_))); } while (0)

/* TSB and TRB: Z from A AND memory, then the bits set or cleared */
static inline void tsb8(uint32_t ea)
{
    uint8_t m = rd8_rmw(ea);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((al() & m) ? 0 : P_Z));
    wr8(ea, (uint8_t)(m | al()));
}
static inline void tsb16(uint32_t ea)
{
    uint16_t m = rd16_rmw(ea);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & m) ? 0 : P_Z));
    wr16_rmw(ea, (uint16_t)(m | cpu.a));
}
static inline void trb8(uint32_t ea)
{
    uint8_t m = rd8_rmw(ea);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((al() & m) ? 0 : P_Z));
    wr8(ea, (uint8_t)(m & ~al()));
}
static inline void trb16(uint32_t ea)
{
    uint16_t m = rd16_rmw(ea);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & m) ? 0 : P_Z));
    wr16_rmw(ea, (uint16_t)(m & ~cpu.a));
}

/* Index steps */
static inline void inx8(void) { cpu.x = (uint8_t)(cpu.x + 1); fl8((uint8_t)cpu.x); }
static inline void inx16(void) { cpu.x = (uint16_t)(cpu.x + 1); fl16(cpu.x); }
static inline void dex8(void) { cpu.x = (uint8_t)(cpu.x - 1); fl8((uint8_t)cpu.x); }
static inline void dex16(void) { cpu.x = (uint16_t)(cpu.x - 1); fl16(cpu.x); }
static inline void iny8(void) { cpu.y = (uint8_t)(cpu.y + 1); fl8((uint8_t)cpu.y); }
static inline void iny16(void) { cpu.y = (uint16_t)(cpu.y + 1); fl16(cpu.y); }
static inline void dey8(void) { cpu.y = (uint8_t)(cpu.y - 1); fl8((uint8_t)cpu.y); }
static inline void dey16(void) { cpu.y = (uint16_t)(cpu.y - 1); fl16(cpu.y); }

/* ---- transfers: the destination's width (A by M, X and Y by X) ---- */

static inline void tax8(void) { ldx8_v(al()); }
static inline void tax16(void) { ldx16_v(cpu.a); }
static inline void tay8(void) { ldy8_v(al()); }
static inline void tay16(void) { ldy16_v(cpu.a); }
static inline void txa8(void) { lda8_v((uint8_t)cpu.x); }
static inline void txa16(void) { lda16_v(cpu.x); }
static inline void tya8(void) { lda8_v((uint8_t)cpu.y); }
static inline void tya16(void) { lda16_v(cpu.y); }
static inline void txy8(void) { ldy8_v((uint8_t)cpu.x); }
static inline void txy16(void) { ldy16_v(cpu.x); }
static inline void tyx8(void) { ldx8_v((uint8_t)cpu.y); }
static inline void tyx16(void) { ldx16_v(cpu.y); }
static inline void tsx8(void) { ldx8_v((uint8_t)cpu.s); }
static inline void tsx16(void) { ldx16_v(cpu.s); }
static inline void txs(void) { cpu.s = cpu.x; }
static inline void tcd(void) { cpu.d = cpu.a; fl16(cpu.a); }
static inline void tdc(void) { lda16_v(cpu.d); }
static inline void tcs(void) { cpu.s = cpu.a; }
static inline void tsc(void) { lda16_v(cpu.s); }
static inline void xba(void) { cpu.a = (uint16_t)(cpu.a << 8 | cpu.a >> 8); fl8(al()); }

/* ---- stack ---- */

static inline void push16(uint16_t v) { op_push8((uint8_t)(v >> 8)); op_push8((uint8_t)v); }
static inline uint16_t pull16(void) { uint16_t lo = op_pull8(); return (uint16_t)(lo | op_pull8() << 8); }
static inline void pha8(void) { op_push8(al()); }
static inline void pha16(void) { push16(cpu.a); }
static inline void phx8(void) { op_push8((uint8_t)cpu.x); }
static inline void phx16(void) { push16(cpu.x); }
static inline void phy8(void) { op_push8((uint8_t)cpu.y); }
static inline void phy16(void) { push16(cpu.y); }
static inline void php(void) { op_push8(cpu.p); }
static inline void phb(void) { op_push8(cpu.db); }
static inline void phd(void) { push16(cpu.d); }
static inline void phk(uint8_t bank) { op_push8(bank); }
static inline void pea(uint16_t v) { push16(v); }
static inline void pla8(void) { lda8_v(op_pull8()); }
static inline void pla16(void) { lda16_v(pull16()); }
static inline void plx8(void) { ldx8_v(op_pull8()); }
static inline void plx16(void) { ldx16_v(pull16()); }
static inline void ply8(void) { ldy8_v(op_pull8()); }
static inline void ply16(void) { ldy16_v(pull16()); }
static inline void plp(void) { op_plp(op_pull8()); }
static inline void plb(void) { cpu.db = op_pull8(); fl8(cpu.db); }
static inline void pld(void) { cpu.d = pull16(); fl16(cpu.d); }

/* ---- flags ---- */

static inline void clc(void) { cpu.p &= (uint8_t)~P_C; }
static inline void sec(void) { cpu.p |= P_C; }
static inline void cli(void) { cpu.p &= (uint8_t)~P_I; }
static inline void sei(void) { cpu.p |= P_I; }
static inline void cld(void) { cpu.p &= (uint8_t)~P_D; }
static inline void sed(void) { cpu.p |= P_D; }
static inline void clv(void) { cpu.p &= (uint8_t)~P_V; }

/* ---- block moves: MVN and MVP, A+1 bytes, DB the destination bank ---- */

static inline void mvn(uint8_t dst, uint8_t src)
{
    cpu.db = dst;
    do {
        wr8((uint32_t)dst << 16 | cpu.y, rd8((uint32_t)src << 16 | cpu.x));
        if (cpu.p & P_X) { cpu.x = (uint8_t)(cpu.x + 1); cpu.y = (uint8_t)(cpu.y + 1); }
        else { cpu.x++; cpu.y++; }
    } while (cpu.a-- != 0);
}
static inline void mvp(uint8_t dst, uint8_t src)
{
    cpu.db = dst;
    do {
        wr8((uint32_t)dst << 16 | cpu.y, rd8((uint32_t)src << 16 | cpu.x));
        if (cpu.p & P_X) { cpu.x = (uint8_t)(cpu.x - 1); cpu.y = (uint8_t)(cpu.y - 1); }
        else { cpu.x--; cpu.y--; }
    } while (cpu.a-- != 0);
}

/* ---- branches ---- */

#define IF_N (cpu.p & P_N)
#define IF_V (cpu.p & P_V)
#define IF_Z (cpu.p & P_Z)
#define IF_C (cpu.p & P_C)

#endif

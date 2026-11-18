/*
 * power_on.c: the game's start at power-on and reset: the hardware and
 * work RAM cleared, the task slots laid out, the kept options checked, and
 * the first task started under the scheduler.
 */
#include "machine.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"
#include "wram_state.h"

void abi_PpuRegistersZero(void);
void abi_KeptOptionsSum(void);
void abi_NmiEnable(void);
void abi_TaskCreate(void);

#define SIGNATURE 0x0000u        /* word: $0123 once the game has started (kept over a reset) */
#define V_000A 0x000Au           /* cleared at the start; purpose not yet established */
#define SLOT_DATA 0x0074u        /* by task slot ($18 bytes each), the byte after the stack top */
#define V_0004 0x0004u           /* set to $80 on a cold start; purpose not yet established */
#define V_0005 0x0005u           /* cleared on a cold start; purpose not yet established */
#define DP_5F 0x5Fu              /* direct page: set to $88; purpose not yet established */
#define SLOT_TOPS 0xB60Bu        /* ROM, bank $80: each slot's stack top and the next byte */
#define TITLE 0xB600u            /* ROM, bank $80: what the header's title must start with */
#define HEADER 0xFFC0u           /* ROM: the cartridge header's title */
#define SCHEDULER 0x81BC6Du      /* the task scheduler */

/* A JSR to fn whose last byte is at ret: the return pushed (high, low),
   fn, and the RTS's pulls of it read back (low, high), as the original's
   stack accesses go: this code clears and reuses that stack. */
static void jsr_call(uint16_t ret, void (*fn)(void))
{
    wr8(cpu.s, (uint8_t)(ret >> 8)); cpu.s--;
    wr8(cpu.s, (uint8_t)ret); cpu.s--;
    fn();
    (void)rd8((uint16_t)(cpu.s + 1));
    (void)rd8((uint16_t)(cpu.s + 2));
    cpu.s = (uint16_t)(cpu.s + 2);
}

/* The same for a JSL at site: its frame, fn, the RTL's pulls. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    (void)rd8((uint16_t)(cpu.s + 1));
    (void)rd8((uint16_t)(cpu.s + 2));
    (void)rd8((uint16_t)(cpu.s + 3));
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 80:8000 PowerOn
 * @alias 00:8000
 * WHAT IT DOES: The game's start, from the reset vector: the S-CPU into
 * native mode in the fast bank, the I/O registers, the PPU's and work RAM
 * cleared, each task slot's stack laid out; on a cold start (the RAM's
 * signature or the header's title not as left by the game, or the kept
 * options' sum wrong) the signature and the kept state set afresh; then
 * the NMI on, the first task created (slot 0 at $81:8000) and the
 * scheduler entered for good.
 * HOW: SEI; native mode (CLC, XCE); JML into bank $80. NMITIMEN, MDMAEN,
 * HDMAEN 0; INIDISP $8F (forced blank); $000A 0; MEMSEL 1; then
 * $4202-$420D 0 (X $0D down to 2); WRIO $FF. S = $0326; DB = $80.
 * PpuRegistersZero for $2101-$210C, $2115-$211A and $2123-$2133. DB $7E:
 * $7E:2000-$7E:FFFF cleared; $000C-$1FFF cleared; the palette buffer
 * ($200 bytes) cleared. The 8 slots' stack tops and the next byte from
 * $80:B60B. The title's first 8 bytes compared with $80:B600 (16-bit, X
 * 0-6), then SIGNATURE with $0123: both equal and KeptOptionsSum equal
 * to KEPT_OPTIONS_SUM: a warm start, the kept state left as it is. Else
 * SIGNATURE $0123, KEPT_OPTIONS_SUM $80, KEPT_OPTIONS and $0005 0, $0004
 * $80, CHEAT_0009 0. NMITIMEN's shadow 1, NmiEnable, CLI; dp $5F $88;
 * TASK_SLOT_COUNT 8; TaskCreate (slot 0, parameter 0, $81:8000); JML to
 * the scheduler ($81:BC6D).
 * Entry: the reset vector (emulation mode, at $00:8000), or a unit call.
 * Exit: never returns: in the scheduler with S $0326 (cpu.unwound).
 */
void abi_PowerOn(void)
{
    uint8_t c;

    sei();
    clc();
    /* XCE: the carry and the emulation flag swapped; emulation mode
       holds M and X at 8 bits and S in page 1 */
    c = cpu.p & P_C;
    cpu.p = cpu.e ? (uint8_t)(cpu.p | P_C) : (uint8_t)(cpu.p & ~P_C);
    cpu.e = c ? 1 : 0;
    if (cpu.e) {
        cpu.p |= P_M | P_X;
        cpu.x &= 0xFF;
        cpu.y &= 0xFF;
        cpu.s = (uint16_t)(0x0100 | (cpu.s & 0xFF));
    }
    /* JML $80:8007: on in the fast bank */
    stz8(ea_abs(NMITIMEN));
    stz8(ea_abs(MDMAEN));
    stz8(ea_abs(HDMAEN));
    lda8_v(0x8F);
    sta8(ea_abs(INIDISP));
    stz8(ea_abs(V_000A));
    lda8_v(0x01);
    sta8(ea_abs(MEMSEL));
    ldx8_v(0x0D);
    for (;;) {
        cpx8_v(0x01);
        if (IF_Z) break;
        stz8(ea_absx(NMITIMEN));
        dex8();
        if (IF_N) break;
    }
    lda8_v(0xFF);
    sta8(ea_abs(WRIO));
    op_rep(0x10);
    ldx16_v(0x0326);
    txs();
    op_sep(0x10);
    lda8_v(0x80);
    pha8();
    plb();
    ldx8_v(0x01);
    ldy8_v(0x0B);
    jsr_call(0x8040, abi_PpuRegistersZero);   /* JSR at 80:803E */
    ldx8_v(0x15);
    ldy8_v(0x05);
    jsr_call(0x8047, abi_PpuRegistersZero);   /* JSR at 80:8045 */
    ldx8_v(0x23);
    ldy8_v(0x10);
    jsr_call(0x804E, abi_PpuRegistersZero);   /* JSR at 80:804C */
    phb();
    lda8_v(0x7E);
    pha8();
    plb();
    op_rep(0x10);
    ldx16_v(0xE000);
    do {
        stz8(ea_absx(0x1FFF));
        dex16();
    } while (!IF_Z);
    plb();
    ldx16_v(0x1FFF);
    do {
        stz8(ea_absx(0x0000));
        dex16();
        cpx16_v(0x000B);
    } while (!IF_Z);
    ldx16_v(0x01FF);
    lda8_v(0x00);
    do {
        sta8(ea_longx(PALETTE_BUFFER));
        dex16();
    } while (!IF_N);
    op_sep(0x30);
    ldy8_v(0xA8);
    ldx8_v(0x0E);
    do {
        lda8(ea_absx(SLOT_TOPS));
        sta8(ea_absy(TASK_STACK_TOP));
        lda8(ea_absx(SLOT_TOPS + 1));
        sta8(ea_absy(SLOT_DATA));
        sec();
        tya8();
        sbc8_v(0x18);
        tay8();
        dex8();
        dex8();
    } while (!IF_N);
    op_rep(0x20);
    ldx8_v(0xFE);
    for (;;) {
        inx8();
        inx8();
        lda16(ea_absx(TITLE));
        cmp16_v(rd16(ea_absx(HEADER)));
        if (!IF_Z) goto cold;
        cpx8_v(0x06);
        if (IF_Z) break;
    }
    lda16_v(0x0123);
    cmp16_v(rd16(ea_abs(SIGNATURE)));
    op_sep(0x20);
    if (!IF_Z) goto cold;
    jsr_call(0x80B1, abi_KeptOptionsSum);      /* JSR at 80:80AF */
    cmp8_v(rd8(ea_abs(KEPT_OPTIONS_SUM)));
    if (IF_Z) goto started;
cold:
    lda8_v(0x23);
    sta8(ea_abs(SIGNATURE));
    lda8_v(0x01);
    sta8(ea_abs(SIGNATURE + 1));
    lda8_v(0x80);
    sta8(ea_abs(KEPT_OPTIONS_SUM));
    stz8(ea_abs(KEPT_OPTIONS));
    stz8(ea_abs(V_0005));
    lda8_v(0x80);
    sta8(ea_abs(V_0004));
    stz8(ea_abs(CHEAT_0009));
started:
    op_sep(0x20);
    lda8_v(0x01);
    sta8(ea_abs(NMITIMEN_SHADOW));
    jsl_call(0x8080DBu, abi_NmiEnable);
    cli();
    lda8_v(0x88);
    sta8(ea_dp(DP_5F));
    lda8_v(0x08);
    sta8(ea_abs(TASK_SLOT_COUNT));
    jsl_frame(0x8080E9u);
    abi_TaskCreate();
    cpu.unwound = 0;
    /* JML to the scheduler, which has no C: the machine goes on there */
    cpu.pc = SCHEDULER;
    cpu.unwound = 1;
}

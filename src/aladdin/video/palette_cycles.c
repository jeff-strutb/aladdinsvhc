/*
 * palette_cycles.c: the level's palette cycles: six records of 14 bytes
 * from $15F1, each playing a script of colour frames from the ROM's table
 * at $84:E000 into the palette buffer, round and round.
 *
 * A record, through the direct page (D = the record): $00 nonzero while it
 * runs; $01 its cycle (a word index into the table); $02 the frames in the
 * script and $03 the colours in a frame; $05 the wait before the next
 * frame; $06 the script's first frame and $08 the next one (offsets from
 * $84:E004); $0A the first colour's offset in the palette buffer; $0C the
 * step (0 load, 1 restart, 2 frame, 3 wait); $0D the frames left.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"

#define PALETTE_CYCLES 0x15F1u       /* the six records, 14 bytes apart */
#define PALETTE_CYCLES_END 0x1645u
#define PALETTE_CYCLE_SIZE 0x0E
#define PCYC_ACTIVE 0x00
#define PCYC_ID 0x01
#define PCYC_FRAMES 0x02
#define PCYC_COLOURS 0x03
#define PCYC_WAIT 0x05
#define PCYC_START 0x06
#define PCYC_NEXT 0x08
#define PCYC_AT 0x0A
#define PCYC_STEP 0x0C
#define PCYC_LEFT 0x0D
#define PALETTE_CYCLE_TABLE 0x84E000u /* by cycle: a script's offset (words); a script: frames, colours (bytes) */
#define PALETTE_CYCLE_DATA 0x84E004u  /* a frame: its wait (byte), then its colours (words) */
#define PALETTE_CYCLE_STEPS 0xECBBu   /* the steps' code, by step (words, bank $81) */

/*
 * @routine 81:EC8E PaletteCycleStart
 * WHAT IT DOES: Starts palette cycle A in record X.
 * HOW: Its step 0 (load), its cycle A, and its active byte counted up.
 * Entry: M 8-bit; A low = the cycle; X = the record's offset from $15F1;
 * DB a bank whose $0000-$1FFF is work RAM; called by JSR. Exit: N, Z
 * from the active byte; the registers unchanged.
 */
void abi_PaletteCycleStart(void)
{
    stz8(ea_absx(PALETTE_CYCLES + PCYC_STEP));
    sta8(ea_absx(PALETTE_CYCLES + PCYC_ID));
    MEM8(inc, ea_absx(PALETTE_CYCLES + PCYC_ACTIVE));
}

/*
 * @routine 81:EC8A PaletteCycleStartLong
 * WHAT IT DOES: PaletteCycleStart for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as PaletteCycleStart; called by JSL. Exit: as PaletteCycleStart.
 */
void abi_PaletteCycleStartLong(void)
{
    abi_PaletteCycleStart();
}

/*
 * @routine 81:EC98 PaletteCyclesStep
 * WHAT IT DOES: Steps every running palette cycle, once a frame.
 * HOW: With D at each of the six records in turn, one with its active
 * byte nonzero runs the code its step names (the table at $81:ECBB).
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M and X 8-bit; D unchanged; N, Z from it (PLD); A = $1645 (16
 * bits); C set, V unchanged; X as the last step leaves it (low byte).
 */
void abi_PaletteCyclesStep(void)
{
    phd();
    lda8_v(PALETTE_CYCLES >> 8);
    xba();
    lda8_v(PALETTE_CYCLES & 0xFF);
    tcd();
    do {
        lda8(ea_dp(PCYC_ACTIVE));
        if (!IF_Z) {
            lda8(ea_dp(PCYC_STEP));
            ACC8(asl);
            tax8();
            call_routine(0x810000u | rd16(0x810000u | (uint16_t)(PALETTE_CYCLE_STEPS + cpu.x)), 0);
        }
        op_rep(0x20);
        tdc();
        clc();
        adc16_v(PALETTE_CYCLE_SIZE);
        tcd();
        cmp16_v(PALETTE_CYCLES_END);
        op_sep(0x30);
    } while (!IF_Z);
    pld();
}

/*
 * @routine 81:ECC3 PaletteCycleLoad
 * WHAT IT DOES: A palette cycle's first step: finds its script.
 * HOW: The cycle's entry in the table at $84:E000 is the script's offset,
 * kept as its first frame; the script's first word is its frames and
 * colours, its second the palette buffer offset; the step goes on.
 * Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep.
 * Exit: M and X 16-bit; A = the palette offset; X = the script's offset;
 * N, Z from the step's increment; C, V, Y unchanged.
 */
void abi_PaletteCycleLoad(void)
{
    lda8(ea_dp(PCYC_ID));
    ACC8(asl);
    tax8();
    op_rep(0x30);
    lda16(ea_longx(PALETTE_CYCLE_TABLE));
    sta16(ea_dp(PCYC_START));
    tax16();
    lda16(ea_longx(PALETTE_CYCLE_TABLE));
    sta16(ea_dp(PCYC_FRAMES));
    lda16(ea_longx(PALETTE_CYCLE_TABLE + 2));
    sta16(ea_dp(PCYC_AT));
    MEM16(inc, ea_dp(PCYC_STEP));
}

/*
 * @routine 81:ECEF PaletteCycleFrame
 * WHAT IT DOES: Puts a palette cycle's next frame in the palette buffer.
 * HOW: The frame's wait byte, then its colours (the script's count of
 * them) copied as words from $84:E004 plus the next frame's offset to the
 * buffer from the cycle's offset; the next frame's offset kept, the step
 * goes on (to the wait) and the buffer is marked for upload. Scratch
 * $000C holds the colours left.
 * Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep,
 * or reached from PaletteCycleRestart. Exit: M 8-bit, X 16-bit; X = the
 * next frame's offset; Y = past the last colour's offset; A = the last
 * colour (16 bits); N, Z from the upload flag's increment; C, V from the
 * last offset step; scratch $000C = 0.
 */
void abi_PaletteCycleFrame(void)
{
    op_rep(0x10);
    lda8(ea_dp(PCYC_COLOURS));
    sta8(ea_abs(SCRATCH));
    stz8(ea_abs(SCRATCH + 1));
    ldx16(ea_dp(PCYC_NEXT));
    lda8(ea_longx(PALETTE_CYCLE_DATA));
    sta8(ea_dp(PCYC_WAIT));
    inx16();
    op_rep(0x20);
    ldy16(ea_dp(PCYC_AT));
    do {
        lda16(ea_longx(PALETTE_CYCLE_DATA));
        phx16();
        tyx16();
        sta16(ea_longx(PALETTE_BUFFER));
        plx16();
        inx16();
        inx16();
        iny16();
        iny16();
        MEM16(dec, ea_abs(SCRATCH));
    } while (!IF_Z);
    stx16(ea_dp(PCYC_NEXT));
    op_sep(0x20);
    MEM8(inc, ea_dp(PCYC_STEP));
    MEM8(inc, ea_abs(PALETTE_UPLOAD));
}

/*
 * @routine 81:ECDF PaletteCycleRestart
 * WHAT IT DOES: Starts a palette cycle's script again from its first
 * frame, and puts that frame in the buffer.
 * HOW: Its frames left = the script's frames, its next frame = its first;
 * the step goes on; then PaletteCycleFrame (the code that follows).
 * Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep.
 * Exit: as PaletteCycleFrame.
 */
void abi_PaletteCycleRestart(void)
{
    op_rep(0x10);
    lda8(ea_dp(PCYC_FRAMES));
    sta8(ea_dp(PCYC_LEFT));
    lda8(ea_dp(PCYC_START));
    sta8(ea_dp(PCYC_NEXT));
    lda8(ea_dp(PCYC_START + 1));
    sta8(ea_dp(PCYC_NEXT + 1));
    MEM8(inc, ea_dp(PCYC_STEP));
    abi_PaletteCycleFrame();
}

/*
 * @routine 81:ED24 PaletteCycleWait
 * WHAT IT DOES: Waits out a palette cycle's frame, then goes on to its
 * next frame, or back to its first once every frame has played.
 * HOW: The wait counted down; at 0 the frames left counted down: step 2
 * (frame) while some are left, else step 1 (restart).
 * Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep.
 * Exit: N, Z from the last decrement or A; A low = the step when the
 * wait ran out; C, V, X, Y unchanged.
 */
void abi_PaletteCycleWait(void)
{
    MEM8(dec, ea_dp(PCYC_WAIT));
    if (!IF_Z)
        return;
    lda8_v(0x02);
    MEM8(dec, ea_dp(PCYC_LEFT));
    if (IF_Z)
        ACC8(dec);
    sta8(ea_dp(PCYC_STEP));
}

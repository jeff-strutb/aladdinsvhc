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
#include "native.h"
#include "cpu_ops.h"
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
    uint32_t cycle = dba(PALETTE_CYCLES) + cpu.x;

    wr8(cycle + PCYC_STEP, 0);
    wr8(cycle + PCYC_ID, (uint8_t)cpu.a);
    inc8_at(cycle + PCYC_ACTIVE);
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
    cpu.a = cpu.d = PALETTE_CYCLES;
    nz16_of(PALETTE_CYCLES);
    do {                                  /* each active cycle: its step's code */
        uint8_t active = rd8(dpa(PCYC_ACTIVE)), step;

        a8_load(active);
        if (active) {
            step = rd8(dpa(PCYC_STEP));
            flag_set(P_C, step & 0x80);
            a8_load((uint8_t)(step << 1));
            cpu.x = (uint8_t)cpu.a;
            call_routine(0x810000u | rd16(0x810000u | (uint16_t)(PALETTE_CYCLE_STEPS + cpu.x)), 0);
        }
        op_rep(P_M);
        flag_set(P_C, 0);
        cpu.a = op_adc16(cpu.d, PALETTE_CYCLE_SIZE, &cpu.p);
        cpu.d = cpu.a;
        cmp16_of(cpu.a, PALETTE_CYCLES_END);
        op_sep(0x30);
    } while (cpu.d != PALETTE_CYCLES_END);
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
    uint16_t script;

    cpu.x = (uint8_t)(rd8(dpa(PCYC_ID)) << 1);
    op_rep(0x30);
    script = rd16(PALETTE_CYCLE_TABLE + cpu.x);
    dp16_set(PCYC_START, script);
    cpu.x = script;
    dp16_set(PCYC_FRAMES, rd16(PALETTE_CYCLE_TABLE + script));   /* frames, colours */
    cpu.a = rd16(PALETTE_CYCLE_TABLE + 2 + script);              /* the palette offset */
    dp16_set(PCYC_AT, cpu.a);
    inc16_at(dpa(PCYC_STEP));
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
    uint8_t colours = rd8(dpa(PCYC_COLOURS));

    op_rep(P_X);
    a8_load(colours);
    wr8(dba(SCRATCH), colours);
    wr8(dba(SCRATCH + 1), 0);
    /* the frame: its wait, then its colours into the palette buffer */
    cpu.x = dp16(PCYC_NEXT);
    a8_load(rd8(PALETTE_CYCLE_DATA + cpu.x));
    wr8(dpa(PCYC_WAIT), (uint8_t)cpu.a);
    cpu.x = (uint16_t)(cpu.x + 1);
    op_rep(P_M);
    cpu.y = dp16(PCYC_AT);
    do {
        cpu.a = rd16(PALETTE_CYCLE_DATA + cpu.x);
        phx16();
        cpu.x = cpu.y;
        wr16((PALETTE_BUFFER + cpu.x) & 0xFFFFFF, cpu.a);
        plx16();
        cpu.x = (uint16_t)(cpu.x + 2);
        cpu.y = (uint16_t)(cpu.y + 2);
    } while (dec16_at(dba(SCRATCH)));
    dp16_set(PCYC_NEXT, cpu.x);
    op_sep(P_M);
    inc8_at(dpa(PCYC_STEP));
    inc8_at(dba(PALETTE_UPLOAD));
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
    op_rep(P_X);
    a8_load(rd8(dpa(PCYC_FRAMES)));
    wr8(dpa(PCYC_LEFT), (uint8_t)cpu.a);
    wr8(dpa(PCYC_NEXT), rd8(dpa(PCYC_START)));
    a8_load(rd8(dpa(PCYC_START + 1)));
    wr8(dpa(PCYC_NEXT + 1), (uint8_t)cpu.a);
    inc8_at(dpa(PCYC_STEP));
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
    if (dec8_at(dpa(PCYC_WAIT)))
        return;
    /* the next frame (step 2), or after the last a restart (step 1) */
    a8_load(0x02);
    if (!dec8_at(dpa(PCYC_LEFT)))
        a8_load(0x01);
    wr8(dpa(PCYC_STEP), (uint8_t)cpu.a);
}

/*
 * input_modes.c: where a frame's buttons come from: the pads, the demo's
 * script (the title's attract play), or the pads with a recorder that
 * writes them into cartridge RAM (a development mode retail never enters).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_pad.h"
#include "wram_state.h"

void abi_JoypadRead(void);
void abi_ControlsRead(void);
void abi_InputRecord(void);

/* The demo's input: the pad read (for its Start, bit 4 of the high byte),
   the rest from the demo's script, a step of it each time its frames run
   out. */
static void demo_input(void)
{
    uint8_t before = rd8(dba(JOY_HELD + 1)), held;

    a8_load(before);
    pha8();
    abi_JoypadRead();
    wr8(dba(JOY_HELD), 0);
    wr8(dba(JOY_PRESSED), 0);
    wr8(dba(JOY_HELD + 2), 0);
    wr8(dba(JOY_PRESSED + 2), 0);
    wr8(dba(JOY_HELD + 3), 0);
    wr8(dba(JOY_PRESSED + 3), 0);
    pla8();
    a8_load((uint8_t)((uint8_t)cpu.a ^ 0xFF));
    wr8(dba(JOY_PRESSED + 1), (uint8_t)cpu.a);
    held = (uint8_t)((rd8(dba(JOY_HELD + 1)) & 0x10) | rd8(dba(DEMO_BUTTONS)));
    a8_load(held);
    wr8(dba(JOY_HELD + 1), held);
    a8_load((uint8_t)(held & rd8(dba(JOY_PRESSED + 1))));
    wr8(dba(JOY_PRESSED + 1), (uint8_t)cpu.a);
    if (!dec8_at(dba(DEMO_INPUT_FRAMES))) {
        op_rep(P_X);
        cpu.x = rd16(dba(DEMO_INPUT_AT));
        inc8_at(dba(DEMO_INPUT_AT));
        inc8_at(dba(DEMO_INPUT_AT));
        a8_load(rd8(DEMO_SCRIPT + cpu.x) & 0xEF);
        wr8(dba(DEMO_BUTTONS), (uint8_t)cpu.a);
        a8_load(rd8(DEMO_SCRIPT + 1 + cpu.x));
        wr8(dba(DEMO_INPUT_FRAMES), (uint8_t)cpu.a);
        op_sep(P_X);
    }
}

/*
 * @routine 81:BDB1 InputRead
 * WHAT IT DOES: Takes this frame's buttons and turns them into the game's
 * actions: from the pads, or in the demo from its script (only the real
 * pad's START kept, so a player can end the demo), or from the pads while
 * the recorder writes them down.
 * HOW: Play mode 2 (the demo): reads the pads, clears every held and
 * pressed byte, makes pad 1's high byte the real START plus the script's
 * buttons and its pressed byte the ones not held last frame, and steps the
 * script when its frame count runs out. Otherwise reads the pads, and in
 * mode 1 calls InputRecord. Both end in ControlsRead.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $4212-$421B are the CPU's registers; called by JSR.
 * Exit: as ControlsRead; X and Y unchanged; M 8-bit, X 8-bit.
 */
void abi_InputRead(void)
{
    uint8_t mode = rd8(dba(PLAY_MODE));

    a8_load(mode);
    cmp8_of(mode, 0x02);
    phx8();
    phy8();
    if (mode == 0x02) {
        demo_input();
    } else {
        abi_JoypadRead();
        mode = rd8(dba(PLAY_MODE));
        a8_load((uint8_t)(mode - 1));
        if (mode == 0x01)                 /* play mode 1: the input recorded */
            abi_InputRecord();
    }
    abi_ControlsRead();
    ply8();
    plx8();
}

/*
 * @routine 81:BE5A InputRecord
 * WHAT IT DOES: The development recorder: writes pad 1's buttons into
 * cartridge RAM as runs (the buttons and how many frames they were held),
 * until its buffer is full; then holds the game until pad 2's DOWN and
 * starts over. This board has no cartridge RAM: the writes go nowhere and
 * each read is the bus's last byte ($70).
 * HOW: By its step, through its table: step 0 clears the buffer's first
 * 256 bytes, its offset and pad 1's held buttons, and goes on to step 1;
 * step 1 compares the buttons with the run at the offset: the same, the
 * run's frame count goes up; else, or when the count would wrap past $FF,
 * it starts the next run, 2 bytes on (its buttons, its count up by one),
 * and at offset $FE keeps the offset, waits for pad 2's DOWN (JoypadRead
 * in a loop) and sets the step back to 0. The wrap ($81:BE97) needs a
 * read of $FF at $70:0001 up: on this board the read is $70, so it never
 * runs, and no script covers it.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR (InputRead, play mode 1).
 * Exit: M 8-bit, X 8-bit; A, X, Y, flags as its last step left them.
 */
void abi_InputRecord(void)
{
    uint16_t to;
    uint8_t step = rd8(dba(RECORD_STEP)), held, n;

    cpu.x = (uint8_t)(step << 1);
    flag_set(P_C, step & 0x80);
    a8_load((uint8_t)cpu.x);
    to = rd16(0x810000u | (uint16_t)(0xBE62 + cpu.x));   /* JMP ($BE62,X) */
    if (to == 0xBE66) {
        /* step 0: the buffer and the pad cleared, then on as step 1 */
        x8_load(0x00);
        a8_load(0x00);
        do {
            wr8(RECORD_BUF + cpu.x, 0x00);
            x8_load((uint8_t)(cpu.x - 1));
        } while (cpu.x);
        wr8(dba(RECORD_POS), 0);
        wr8(dba(RECORD_POS + 1), 0);
        wr8(dba(JOY_HELD), 0);
        wr8(dba(JOY_HELD + 1), 0);
        inc8_at(dba(RECORD_STEP));
    } else if (to != 0xBE7F) {
        jump_routine(0x810000u | to);
        return;
    }
    /* step 1: the pad's high byte kept with how many frames it held, two
       bytes an entry */
    x8_load(rd8(dba(RECORD_POS)));
    held = rd8(dba(JOY_HELD + 1));
    a8_load(held);
    n = rd8(RECORD_BUF + cpu.x);          /* (outside work RAM: each byte read once, as the original) */
    cmp8_of(held, n);
    if (held == n) {
        cpu.y = held;
        n = (uint8_t)(rd8(RECORD_BUF + 1 + cpu.x) + 1);
        a8_load(n);
        wr8(RECORD_BUF + 1 + cpu.x, n);
        if (n)
            return;
        a8_load(0xFF);                    /* the count held at 255: a new entry */
        wr8(RECORD_BUF + 1 + cpu.x, 0xFF);
        a8_load(held);
    }
    wr8(RECORD_BUF + 2 + cpu.x, (uint8_t)cpu.a);
    n = (uint8_t)(rd8(RECORD_BUF + 3 + cpu.x) + 1);
    a8_load(n);
    wr8(RECORD_BUF + 3 + cpu.x, n);
    cpu.x = (uint8_t)(cpu.x + 2);
    cmp8_of((uint8_t)cpu.x, 0xFE);
    if (cpu.x != 0xFE) {
        wr8(dba(RECORD_POS), (uint8_t)cpu.x);
        return;
    }
    /* the buffer full: wait for Select, then record again */
    wr8(dba(RECORD_END), (uint8_t)cpu.x);
    do {
        abi_JoypadRead();
        held = rd8(dba(JOY_HELD + 3));
        a8_load(held);
        flag_set(P_Z, !(held & 0x04));
    } while (!(held & 0x04));
    wr8(dba(RECORD_STEP), 0);
}

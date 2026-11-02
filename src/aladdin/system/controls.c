/*
 * controls.c: the game's actions from pad 1, through the buttons the
 * options assign them.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_pad.h"

/* The options' button for each of the four actions (a mask word each). */
#define CONTROL_MASKS 0x000348u
/* A copy of pad 1's newly pressed buttons (word), beside PAD1_HELD. */
#define PAD1_PRESSED_COPY 0x000381u
/* Per action, nonzero while its button is held (ACTION_HELD) or was just
   pressed (ACTION_PRESSED). */
#define ACTION_HELD 0x000383u
#define ACTION_PRESSED 0x000387u

/*
 * @routine 81:BDE9 ControlsRead
 * WHAT IT DOES: Turns pad 1's buttons into the game's four actions, held
 * and newly pressed, through the button chosen for each.
 * HOW: Clears the eight action flags, copies the pad's held and pressed
 * words, and counts each action whose button mask (from the options)
 * matches.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M 8-bit; A = the pressed word (16 bits); N, V, Z from the last
 * BIT; X, Y, C unchanged.
 */
void abi_ControlsRead(void)
{
    /* by action: the mask's offset in CONTROL_MASKS, its count's in
       ACTION_HELD and ACTION_PRESSED (the counts are bytes, counted up as
       words: the higher byte catches the carry) */
    static const uint8_t mask[4] = { 2, 0, 4, 6 };
    static const uint8_t count[4] = { 1, 0, 2, 3 };
    uint16_t pad;

    for (int i = 0; i < 4; i++) {
        wr8(dba(ACTION_HELD + count[i]), 0);
        wr8(dba(ACTION_PRESSED + count[i]), 0);
    }
    op_rep(P_M);
    wr16(dba(PAD1_HELD), rd16(dba(JOY_HELD)));
    wr16(dba(PAD1_PRESSED_COPY), rd16(dba(JOY_PRESSED)));
    pad = rd16(dba(PAD1_HELD));
    a16_load(pad);
    for (int i = 0; i < 4; i++) {
        bit16_of(pad, rd16(dba(CONTROL_MASKS + mask[i])));
        if (!(cpu.p & P_Z))
            inc16_at(dba(ACTION_HELD + count[i]));
    }
    pad = rd16(dba(PAD1_PRESSED_COPY));
    a16_load(pad);
    for (int i = 0; i < 2; i++) {
        bit16_of(pad, rd16(dba(CONTROL_MASKS + mask[i])));
        if (!(cpu.p & P_Z))
            inc16_at(dba(ACTION_PRESSED + count[i]));
    }
    op_sep(P_M);
    for (int i = 2; i < 4; i++) {         /* the last two by the low byte only */
        bit8_of((uint8_t)pad, rd8(dba(CONTROL_MASKS + mask[i])));
        if (!(cpu.p & P_Z))
            inc8_at(dba(ACTION_PRESSED + count[i]));
    }
}

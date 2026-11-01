/*
 * controls.c: the game's actions from pad 1, through the buttons the
 * options assign them.
 */
#include "machine.h"
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
    stz8(ea_abs((ACTION_HELD + 1)));  /* 81BDE9 stz */
    stz8(ea_abs((ACTION_PRESSED + 1)));  /* 81BDEC stz */
    stz8(ea_abs(ACTION_HELD));  /* 81BDEF stz */
    stz8(ea_abs(ACTION_PRESSED));  /* 81BDF2 stz */
    stz8(ea_abs((ACTION_HELD + 2)));  /* 81BDF5 stz */
    stz8(ea_abs((ACTION_PRESSED + 2)));  /* 81BDF8 stz */
    stz8(ea_abs((ACTION_HELD + 3)));  /* 81BDFB stz */
    stz8(ea_abs((ACTION_PRESSED + 3)));  /* 81BDFE stz */
    op_rep(0x20);  /* 81BE01 rep */
    lda16(ea_abs(JOY_HELD));  /* 81BE03 lda */
    sta16(ea_abs(PAD1_HELD));  /* 81BE06 sta */
    lda16(ea_abs(JOY_PRESSED));  /* 81BE09 lda */
    sta16(ea_abs(PAD1_PRESSED_COPY));  /* 81BE0C sta */
    lda16(ea_abs(PAD1_HELD));  /* 81BE0F lda */
    bit16_v(rd16(ea_abs((CONTROL_MASKS + 2))));  /* 81BE12 bit */
    if (IF_Z) goto L_81BE1A;  /* 81BE15 beq */
    MEM16(inc, ea_abs((ACTION_HELD + 1)));  /* 81BE17 inc */
L_81BE1A:
    bit16_v(rd16(ea_abs(CONTROL_MASKS)));  /* 81BE1A bit */
    if (IF_Z) goto L_81BE22;  /* 81BE1D beq */
    MEM16(inc, ea_abs(ACTION_HELD));  /* 81BE1F inc */
L_81BE22:
    bit16_v(rd16(ea_abs((CONTROL_MASKS + 4))));  /* 81BE22 bit */
    if (IF_Z) goto L_81BE2A;  /* 81BE25 beq */
    MEM16(inc, ea_abs((ACTION_HELD + 2)));  /* 81BE27 inc */
L_81BE2A:
    bit16_v(rd16(ea_abs((CONTROL_MASKS + 6))));  /* 81BE2A bit */
    if (IF_Z) goto L_81BE32;  /* 81BE2D beq */
    MEM16(inc, ea_abs((ACTION_HELD + 3)));  /* 81BE2F inc */
L_81BE32:
    lda16(ea_abs(PAD1_PRESSED_COPY));  /* 81BE32 lda */
    bit16_v(rd16(ea_abs((CONTROL_MASKS + 2))));  /* 81BE35 bit */
    if (IF_Z) goto L_81BE3D;  /* 81BE38 beq */
    MEM16(inc, ea_abs((ACTION_PRESSED + 1)));  /* 81BE3A inc */
L_81BE3D:
    bit16_v(rd16(ea_abs(CONTROL_MASKS)));  /* 81BE3D bit */
    if (IF_Z) goto L_81BE45;  /* 81BE40 beq */
    MEM16(inc, ea_abs(ACTION_PRESSED));  /* 81BE42 inc */
L_81BE45:
    op_sep(0x20);  /* 81BE45 sep */
    bit8_v(rd8(ea_abs((CONTROL_MASKS + 4))));  /* 81BE47 bit */
    if (IF_Z) goto L_81BE4F;  /* 81BE4A beq */
    MEM8(inc, ea_abs((ACTION_PRESSED + 2)));  /* 81BE4C inc */
L_81BE4F:
    bit8_v(rd8(ea_abs((CONTROL_MASKS + 6))));  /* 81BE4F bit */
    if (IF_Z) goto L_81BE57;  /* 81BE52 beq */
    MEM8(inc, ea_abs((ACTION_PRESSED + 3)));  /* 81BE54 inc */
L_81BE57:
    op_sep(0x20);  /* 81BE57 sep */
    return;  /* 81BE59 rts */
}

/*
 * level_flags.c: eight flags at $7E:F260 that a level sets up as it starts, from a mask
 * the level's $035D picks. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"

#define LEVEL_FLAGS 0x7EF260u       /* eight flags (bytes); 1: set up set; bit 7 set by actors later */
#define V_035D 0x00035Du            /* the second byte from the level's entry at $81:B933 */
/* In bank $80: per $035D a mask of the flags to set; and the bit of each
   flag (1, 2, 4 ... $80) */
#define LEVEL_FLAG_MASKS 0xD27Eu
#define FLAG_BITS 0xBCA1u

/*
 * @routine 83:C2F0 LevelFlagFromMask
 * WHAT IT DOES: Sets flag X of the level's eight to 1 when its bit is set
 * in the mask for the level's $035D, else to 0.
 * HOW: The mask (by $035D) ANDed with the flag's bit; nonzero becomes 1;
 * stored to the flag.
 * Entry: M 8-bit, X 8-bit; X = the flag (0-7); DB = $80; called by JSR (by
 * 83:C2E7 for all eight, from the level's set-up at 81:8BEF, and by
 * actors). Exit: M 8-bit, X 8-bit; A low = the flag set; Y = $035D. No
 * level the game plays has a mask bit set: setting a flag to 1 is reached
 * only by unit_level_flags.box.
 */
void abi_LevelFlagFromMask(void)
{
    ldy8(ea_abs(0x035D));  /* 83C2F0 ldy */
    lda8(ea_absy(0xD27E));  /* 83C2F3 lda */
    and8_v(rd8(ea_absx(0xBCA1)));  /* 83C2F6 and */
    if (IF_Z) goto L_83C2FD;  /* 83C2F9 beq */
    lda8_v(0x01);  /* 83C2FB lda */
L_83C2FD:
    sta8(ea_longx(0x7EF260u));  /* 83C2FD sta */
    return;  /* 83C301 rts */
}


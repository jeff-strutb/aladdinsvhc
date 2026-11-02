/*
 * level_flags.c: eight flags at $7E:F260 that a level sets up as it starts, from a mask
 * the level's $035D picks. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"

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

/*
 * @routine 83:C2A9 LevelFlagSpotFind
 * WHAT IT DOES: Finds a point ($000C, $000E, each to 8 pixels) in the
 * level's list of flagged spots: C clear and X its flag when found, C set
 * when not.
 * HOW: Both words rounded down to 8; the level less $0E picks the list
 * (from $80:D24A): its count, then 4 bytes a spot (x, y); compared in
 * turn, 16-bit A.
 * Entry: M 8-bit, X 8-bit; $000C, $000E = the point; DB = $80; levels $0E
 * up; called by JSR (83:C181). Exit: M 8-bit; found: C clear, X = the
 * count of spots from it to the list's end less 1; not found: C set
 * (reached only by unit_point_lookup_miss.box).
 */
void abi_LevelFlagSpotFind(void)
{
    lda8_v(0x07);  /* 83C2A9 lda */
    trb8(ea_abs(SCRATCH));  /* 83C2AB trb */
    trb8(ea_abs(0x000E));  /* 83C2AE trb */
    sec();  /* 83C2B1 sec */
    lda8(ea_abs(LEVEL));  /* 83C2B2 lda */
    sbc8_v(0x0E);  /* 83C2B5 sbc */
    ACC8(asl);  /* 83C2B7 asl */
    tax8();  /* 83C2B8 tax */
    op_rep(0x20);  /* 83C2B9 rep */
    ldy8(ea_absx(0xD24A));  /* 83C2BB ldy */
    ldx8(ea_absy(0xD24A));  /* 83C2BE ldx */
L_83C2C1:
    lda16(ea_absy(0xD24B));  /* 83C2C1 lda */
    and16_v(0xFFF8);  /* 83C2C4 and */
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 83C2C7 cmp */
    if (!IF_Z) goto L_83C2D7;  /* 83C2CA bne */
    lda16(ea_absy(0xD24D));  /* 83C2CC lda */
    and16_v(0xFFF8);  /* 83C2CF and */
    cmp16_v(rd16(ea_abs(0x000E)));  /* 83C2D2 cmp */
    if (IF_Z) goto L_83C2E2;  /* 83C2D5 beq */
L_83C2D7:
    iny8();  /* 83C2D7 iny */
    iny8();  /* 83C2D8 iny */
    iny8();  /* 83C2D9 iny */
    iny8();  /* 83C2DA iny */
    dex8();  /* 83C2DB dex */
    if (!IF_Z) goto L_83C2C1;  /* 83C2DC bne */
    op_sep(0x20);  /* 83C2DE sep */
    sec();  /* 83C2E0 sec */
    return;  /* 83C2E1 rts */
L_83C2E2:
    op_sep(0x20);  /* 83C2E2 sep */
    dex8();  /* 83C2E4 dex */
    clc();  /* 83C2E5 clc */
    return;  /* 83C2E6 rts */
}


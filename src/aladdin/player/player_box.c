/*
 * player_box.c: the box actors are tested against, set from the player's
 * box table: its offsets from the player and its half-sizes. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_PlayerBoxSet(void);

/* The test box (see actor/hit_box.c): its centre's offsets from the
   player (x, y), its half-width and half-height (words) */
#define BOX_OFFSET_X 0x00148Cu
#define BOX_OFFSET_Y 0x00148Eu
#define BOX_HALF_W 0x001488u
#define BOX_HALF_H 0x00148Au
/* In bank $80: 10 bytes a box: x offset, y offset, half-width,
   half-height, and the word the player's $16 gets */
#define PLAYER_BOXES 0xC7EBu

/*
 * @routine 81:E909 PlayerBoxUpdate
 * WHAT IT DOES: Sets the test box to the player's box wanted, when it is
 * not the one set already.
 * HOW: PLAYER_BOX_WANTED compared with PLAYER_BOX: the same, returns at
 * PlayerBoxSet's RTS; else on into PlayerBoxSet.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR. Exit: the same: A low = the box, Z set; else as PlayerBoxSet.
 */
void abi_PlayerBoxUpdate(void)
{
    lda8(ea_abs(PLAYER_BOX_WANTED));  /* 81E909 lda */
    cmp8_v(rd8(ea_abs(PLAYER_BOX)));  /* 81E90C cmp */
    if (IF_Z) goto L_81E950;  /* 81E90F beq */
    abi_PlayerBoxSet(); return;
L_81E950:
    return;  /* 81E950 rts */
}


/*
 * @routine 81:E911 PlayerBoxSet
 * WHAT IT DOES: Sets the test box to the player's box A: its offsets from
 * the player, its half-sizes, and the player's $16.
 * HOW: A kept in PLAYER_BOX; times 10 ($002C the double) indexes the box
 * table; the x offset negated when the player's $15 is nonzero; five words
 * copied.
 * Entry: M 8-bit, X 8-bit; A = the box; D = the player's record; DB = $80.
 * Exit: M 8-bit, X 8-bit; X = the box times 10; Y = the player's $15; A =
 * the $16 word.
 */
void abi_PlayerBoxSet(void)
{
    sta8(ea_abs(PLAYER_BOX));  /* 81E911 sta */
    ACC8(asl);  /* 81E914 asl */
    sta8(ea_abs(0x002C));  /* 81E915 sta */
    ACC8(asl);  /* 81E918 asl */
    ACC8(asl);  /* 81E919 asl */
    clc();  /* 81E91A clc */
    adc8_v(rd8(ea_abs(0x002C)));  /* 81E91B adc */
    tax8();  /* 81E91E tax */
    op_rep(0x20);  /* 81E91F rep */
    ldy8(ea_dp(0x15));  /* 81E921 ldy */
    if (!IF_Z) goto L_81E92D;  /* 81E923 bne */
    lda16(ea_absx(PLAYER_BOXES));  /* 81E925 lda */
    sta16(ea_abs(BOX_OFFSET_X));  /* 81E928 sta */
    goto L_81E937;  /* 81E92B bra */
L_81E92D:
    lda16(ea_absx(PLAYER_BOXES));  /* 81E92D lda */
    eor16_v(0xFFFF);  /* 81E930 eor */
    ACC16(inc);  /* 81E933 inc */
    sta16(ea_abs(BOX_OFFSET_X));  /* 81E934 sta */
L_81E937:
    lda16(ea_absx(PLAYER_BOXES + 2));  /* 81E937 lda */
    sta16(ea_abs(BOX_OFFSET_Y));  /* 81E93A sta */
    lda16(ea_absx(PLAYER_BOXES + 4));  /* 81E93D lda */
    sta16(ea_abs(BOX_HALF_W));  /* 81E940 sta */
    lda16(ea_absx(PLAYER_BOXES + 6));  /* 81E943 lda */
    sta16(ea_abs(BOX_HALF_H));  /* 81E946 sta */
    lda16(ea_absx(PLAYER_BOXES + 8));  /* 81E949 lda */
    sta16(ea_dp(0x16));  /* 81E94C sta */
    op_sep(0x20);  /* 81E94E sep */
    return;  /* 81E950 rts */
}


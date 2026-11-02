/*
 * backdrop_offset.c: the backdrop layer's scroll taken from the play
 * field's, with an offset down. Drafted by cdraft.py; named and described
 * by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram_video.h"

#define BACKDROP_OFFSET_Y 0x001A2Bu   /* added to the play field's scroll Y for the backdrop's: 4 bytes, from the fraction byte */

/*
 * @routine 81:CA6A BackdropFollowOffsetY
 * WHAT IT DOES: Moves the backdrop with the play field: the same scroll X,
 * and its scroll Y the play field's plus an offset.
 * HOW: The scroll X word copied; the scroll Y, from its fraction byte,
 * plus BACKDROP_OFFSET_Y as two 16-bit adds with carry (4 bytes).
 * Entry: M 16-bit, X 16-bit; DB = $80; called by JSR from the scroll
 * handler the level picks ($0841). Exit: M 16-bit, X 16-bit; A = the
 * result's high word; C, N, V, Z from its add.
 */
void abi_BackdropFollowOffsetY(void)
{
    lda16(ea_abs(0x1773));  /* 81CA6A lda */
    sta16(ea_abs(0x18C9));  /* 81CA6D sta */
    clc();  /* 81CA70 clc */
    lda16(ea_abs(0x1776));  /* 81CA71 lda */
    adc16_v(rd16(ea_abs(BACKDROP_OFFSET_Y)));  /* 81CA74 adc */
    sta16(ea_abs(0x18CC));  /* 81CA77 sta */
    lda16(ea_abs(0x1778));  /* 81CA7A lda */
    adc16_v(rd16(ea_abs(BACKDROP_OFFSET_Y + 2)));  /* 81CA7D adc */
    sta16(ea_abs(0x18CE));  /* 81CA80 sta */
    return;  /* 81CA83 rts */
}


/*
 * colour_bands.c: the colour bands' HDMA table: the fixed colour set
 * line by line down the screen, written into one of two buffers.
 */
#include "machine.h"
#include "ops65.h"
void abi_ColourBandsWrite(void);
#include "wram.h"

/*
 * @routine 81:A08A ColourBandsWrite
 * WHAT IT DOES: Writes the colour bands' HDMA table into the task's
 * spare buffer, the bands from the 18th on dimmed by its step, and points
 * the HDMA at it.
 * HOW: The buffer (the task's $07: 0 or 1) is at $7E:F500 + its byte at
 * $BAA1; the band count at $BA7E, then (lines, value) pairs from $BA7F,
 * are copied (counting at the task's $08), each value from the 18th pair
 * on less the step (the task's $06), not below 0; $0356 = the buffer's
 * offset; the buffer flips.
 * Entry: M 8-bit, X 8-bit; D = the colour band task's record (its slot +
 * $6F); DB = $80; called by JSR. Exit: A low byte = the new buffer; X, Y
 * past the table (low bytes); the task's $08 = 0.
 */
void abi_ColourBandsWrite(void)
{
    ldy8(ea_dp(0x07));  /* 81A08A ldy */
    phy8();  /* 81A08C phy */
    ldx8(ea_absy(0xBAA1));  /* 81A08D ldx */
    phx8();  /* 81A090 phx */
    lda8(ea_abs(0xBA7E));  /* 81A091 lda */
    sta8(ea_dp(0x08));  /* 81A094 sta */
    ldy8_v(0x00);  /* 81A096 ldy */
L_81A098:
    lda8(ea_absy(0xBA7F));  /* 81A098 lda */
    sta8(ea_longx(0x7EF500u));  /* 81A09B sta */
    lda8(ea_absy(0xBA80));  /* 81A09F lda */
    cpy8_v(0x22);  /* 81A0A2 cpy */
    if (!IF_C) goto L_81A0AD;  /* 81A0A4 bcc */
    sec();  /* 81A0A6 sec */
    sbc8_v(rd8(ea_dp(0x06)));  /* 81A0A7 sbc */
    if (!IF_N) goto L_81A0AD;  /* 81A0A9 bpl */
    lda8_v(0x00);  /* 81A0AB lda */
L_81A0AD:
    sta8(ea_longx(0x7EF501u));  /* 81A0AD sta */
    iny8();  /* 81A0B1 iny */
    iny8();  /* 81A0B2 iny */
    inx8();  /* 81A0B3 inx */
    inx8();  /* 81A0B4 inx */
    MEM8(dec, ea_dp(0x08));  /* 81A0B5 dec */
    if (!IF_Z) goto L_81A098;  /* 81A0B7 bne */
    plx8();  /* 81A0B9 plx */
    stx8(ea_abs(0x0356));  /* 81A0BA stx */
    pla8();  /* 81A0BD pla */
    eor8_v(0x01);  /* 81A0BE eor */
    sta8(ea_dp(0x07));  /* 81A0C0 sta */
    return;  /* 81A0C2 rts */
}

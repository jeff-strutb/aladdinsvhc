/*
 * hud.c: the play screen's status line in the text layer: the health
 * marks, the two counters, a digit and a 2x2 mark. Each is drawn when
 * its shown value falls behind the game's (the refresh at $81:F43C).
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"

#define HEALTH_SHOWN 0x000366u     /* the health the marks show */
#define HUD_COUNT_1_SHOWN 0x000368u /* the first counter as shown (its value at $0369) */
#define HUD_COUNT_2_SHOWN 0x00036Au /* the second counter as shown (its value at $036B) */
#define HEALTH_MARK_PLACES 0x81F58Du /* by health less one: the mark's tilemap offset */
#define HEALTH_MARK_TILE 0x6A      /* the mark's top tile; its bottom is 16 on */
#define HUD_TILE_BITS 0x2C00       /* palette and priority of the status line's tiles */
#define DIGIT_TILE_0 0x05          /* the counters' digit tiles start here; bottoms 16 on */

/*
 * @routine 81:F4D0 HudHealthMarkShow
 * WHAT IT DOES: Draws health mark A (1 up) in the status line, and keeps
 * A less one as the health shown.
 * HOW: The mark's place comes from a table by A less one; its top and
 * bottom tiles go in the text tilemap a row apart, and the layer's upload
 * is asked for.
 * Entry: M 8-bit, X 8-bit; A = the mark; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A low = 1, A high = the bottom tile's high byte; X = the
 * place; Y = $6A; N and Z clear; V and C from adding the row; M 8-bit.
 * The other flags unchanged.
 */
void abi_HudHealthMarkShow(void)
{
    ldy8_v(HEALTH_MARK_TILE);
    ACC8(dec);
    tax8();
    sta8(ea_abs(HEALTH_SHOWN));
    lda8(ea_longx(HEALTH_MARK_PLACES));
    tax8();
    op_rep(0x20);
    tya16();
    ora16_v(HUD_TILE_BITS);
    sta16(ea_longx(TEXT_TILEMAP + 74));
    clc();
    adc16_v(0x0010);
    sta16(ea_longx(TEXT_TILEMAP + 138));
    op_sep(0x20);
    lda8_v(0x01);
    sta8(ea_abs(TEXT_TILEMAP_UPLOAD));
}

/* A's two hex digits as big digits: tops at `top` and `top` + 2 in the
   text tilemap, bottoms a row below; the value kept at `shown`. */
static void counter_show(uint32_t shown, uint32_t top)
{
    sta8(ea_abs((uint16_t)shown));
    pha8();
    ACC8(lsr);
    ACC8(lsr);
    ACC8(lsr);
    ACC8(lsr);
    clc();
    adc8_v(DIGIT_TILE_0);
    sta8(top);
    ora8_v(0x10);
    sta8(top + 64);
    clc();
    pla8();
    and8_v(0x0F);
    adc8_v(DIGIT_TILE_0);
    sta8(top + 2);
    ora8_v(0x10);
    sta8(top + 66);
    lda8_v(0x01);
    sta8(ea_abs(TEXT_TILEMAP_UPLOAD));
}

/*
 * @routine 81:F4F6 HudCount1Show
 * WHAT IT DOES: Draws the first counter, A as two digits, in the status
 * line, and keeps A as the value shown.
 * HOW: Each hex digit's top tile is 5 plus the digit, its bottom tile 16
 * on, a row below; the layer's upload is asked for.
 * Entry: M 8-bit; A = the value (two decimal digits as hex); DB a bank
 * whose $0000-$1FFF is work RAM. Exit: A low = 1; N and Z clear; V and C
 * from the last addition. A high, X, Y and the other flags unchanged.
 */
void abi_HudCount1Show(void)
{
    counter_show(HUD_COUNT_1_SHOWN, TEXT_TILEMAP + 178);
}

/*
 * @routine 81:F521 HudCount2Show
 * WHAT IT DOES: Draws the second counter, A as two digits, in the status
 * line, and keeps A as the value shown.
 * HOW: As HudCount1Show, four columns to the right.
 * Entry: M 8-bit; A = the value (two decimal digits as hex); DB a bank
 * whose $0000-$1FFF is work RAM. Exit: A low = 1; N and Z clear; V and C
 * from the last addition. A high, X, Y and the other flags unchanged.
 */
void abi_HudCount2Show(void)
{
    counter_show(HUD_COUNT_2_SHOWN, TEXT_TILEMAP + 186);
}

/*
 * @routine 81:F47F HudDigitRefresh
 * WHAT IT DOES: Draws the status line's digit for the value at $0364 when
 * it differs from the one shown ($0363): the value less 1, 0 for 0 and 9
 * at most; a value above 100 is held at 99; a negative value gets a mark
 * beside the digit.
 * HOW: The value kept as shown; negative: tile $2E at $7E:D908 and its low
 * nibble drawn; else the value less 1: negative, 0; below 9, it; 100 or
 * more ($65 up), $63 kept in both; else 9. The digit's tile (low nibble
 * plus 5) at $7E:D8C6 and its lower half (OR $10) at $7E:D906; the text
 * layer's upload set.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M 8-bit; unchanged: A low = the value, Z set; drawn: A low = 1.
 */
void abi_HudDigitRefresh(void)
{
    lda8(ea_abs(0x0364));  /* 81F47F lda */
    cmp8_v(rd8(ea_abs(0x0363)));  /* 81F482 cmp */
    if (!IF_Z) goto L_81F488;  /* 81F485 bne */
    return;  /* 81F487 rts */
L_81F488:
    sta8(ea_abs(0x0363));  /* 81F488 sta */
    lda8(ea_abs(0x0363));  /* 81F48B lda */
    if (IF_N) goto L_81F4A7;  /* 81F48E bmi */
    ACC8(dec);  /* 81F490 dec */
    if (IF_N) goto L_81F4B1;  /* 81F491 bmi */
    cmp8_v(0x09);  /* 81F493 cmp */
    if (!IF_C) goto L_81F4B3;  /* 81F495 bcc */
    cmp8_v(0x64);  /* 81F497 cmp */
    if (!IF_C) goto L_81F4A3;  /* 81F499 bcc */
    lda8_v(0x63);  /* 81F49B lda */
    sta8(ea_abs(0x0364));  /* 81F49D sta */
    sta8(ea_abs(0x0363));  /* 81F4A0 sta */
L_81F4A3:
    lda8_v(0x09);  /* 81F4A3 lda */
    goto L_81F4B3;  /* 81F4A5 bra */
L_81F4A7:
    pha8();  /* 81F4A7 pha */
    lda8_v(0x2E);  /* 81F4A8 lda */
    sta8(0x7ED908u);  /* 81F4AA sta */
    pla8();  /* 81F4AE pla */
    goto L_81F4B3;  /* 81F4AF bra */
L_81F4B1:
    lda8_v(0x00);  /* 81F4B1 lda */
L_81F4B3:
    clc();  /* 81F4B3 clc */
    and8_v(0x0F);  /* 81F4B4 and */
    adc8_v(0x05);  /* 81F4B6 adc */
    sta8(0x7ED8C6u);  /* 81F4B8 sta */
    ora8_v(0x10);  /* 81F4BC ora */
    sta8(0x7ED906u);  /* 81F4BE sta */
    lda8_v(0x01);  /* 81F4C2 lda */
    sta8(ea_abs(TEXT_TILEMAP_UPLOAD));  /* 81F4C4 sta */
    return;  /* 81F4C7 rts */
}


/*
 * @routine 81:F54C HudMarkRefresh
 * WHAT IT DOES: Draws the status line's 2x2 mark for the value at $036E,
 * keeping it as shown ($036D): for 0 its blank, else its picture.
 * HOW: The value to $036D; zero: the four blank words ($7E:D8AA, $D8AC,
 * $D8EA, $D8EC); else tiles $2462, $2463, $2472, $2473 there. The text
 * layer's upload counted up.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSR (81:F45E). Exit: M 8-bit; A = the last tile word.
 */
void abi_HudMarkRefresh(void)
{
    op_rep(0x20);  /* 81F54C rep */
    ldx8(ea_abs(0x036E));  /* 81F54E ldx */
    stx8(ea_abs(0x036D));  /* 81F551 stx */
    if (!IF_Z) goto L_81F56B;  /* 81F554 bne */
    lda16_v(0x2020);  /* 81F556 lda */
    sta16(0x7ED8AAu);  /* 81F559 sta */
    sta16(0x7ED8ACu);  /* 81F55D sta */
    sta16(0x7ED8EAu);  /* 81F561 sta */
    sta16(0x7ED8ECu);  /* 81F565 sta */
    goto L_81F587;  /* 81F569 bra */
L_81F56B:
    lda16_v(0x2462);  /* 81F56B lda */
    sta16(0x7ED8AAu);  /* 81F56E sta */
    lda16_v(0x2463);  /* 81F572 lda */
    sta16(0x7ED8ACu);  /* 81F575 sta */
    lda16_v(0x2472);  /* 81F579 lda */
    sta16(0x7ED8EAu);  /* 81F57C sta */
    lda16_v(0x2473);  /* 81F580 lda */
    sta16(0x7ED8ECu);  /* 81F583 sta */
L_81F587:
    op_sep(0x20);  /* 81F587 sep */
    MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));  /* 81F589 inc */
    return;  /* 81F58C rts */
}


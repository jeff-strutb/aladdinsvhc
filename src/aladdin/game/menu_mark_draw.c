/*
 * menu_mark_draw.c: the selection mark of a menu drawn at a choice in the
 * text layer (MenuMarkErase, menu_mark.c, takes it away). Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"



/*
 * @routine 81:EF39 MenuMarkDraw
 * WHAT IT DOES: Draws the selection mark at a menu choice: its seven tiles
 * in the text layer's tilemap (one in row 0, three in rows 1 and 2).
 * HOW: With 16-bit A, tile words at the choice's offset: +2 $207E; +$40,
 * +$42, +$44 $2802, $2003, $2004; +$80, +$82, +$84 $2812, $2013, $2014; P
 * kept.
 * Entry: X 16-bit = the choice's byte offset in the tilemap; called by JSL
 * (from the menu's cursor move, 81:EE1A). Exit: P as at entry; A = $2014.
 */
void abi_MenuMarkDraw(void)
{
    php();  /* 81EF39 php */
    op_rep(0x20);  /* 81EF3A rep */
    lda16_v(0x207E);  /* 81EF3C lda */
    sta16(ea_longx(TEXT_TILEMAP + 2));  /* 81EF3F sta */
    lda16_v(0x2802);  /* 81EF43 lda */
    sta16(ea_longx(TEXT_TILEMAP + 64));  /* 81EF46 sta */
    lda16_v(0x2003);  /* 81EF4A lda */
    sta16(ea_longx(TEXT_TILEMAP + 66));  /* 81EF4D sta */
    lda16_v(0x2004);  /* 81EF51 lda */
    sta16(ea_longx(TEXT_TILEMAP + 68));  /* 81EF54 sta */
    lda16_v(0x2812);  /* 81EF58 lda */
    sta16(ea_longx(TEXT_TILEMAP + 128));  /* 81EF5B sta */
    lda16_v(0x2013);  /* 81EF5F lda */
    sta16(ea_longx(TEXT_TILEMAP + 130));  /* 81EF62 sta */
    lda16_v(0x2014);  /* 81EF66 lda */
    sta16(ea_longx(TEXT_TILEMAP + 132));  /* 81EF69 sta */
    plp();  /* 81EF6D plp */
    return;  /* 81EF6E rtl */
}

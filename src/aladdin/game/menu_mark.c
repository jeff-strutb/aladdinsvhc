/*
 * menu_mark.c: the selection mark of a menu drawn in the text layer: taken
 * away from the choice left; and the title menu's cursor moved. Drafted
 * by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"

void abi_SoundQueueAdd(void);

/*
 * @routine 81:EF15 MenuMarkErase
 * WHAT IT DOES: Takes the selection mark away from a menu choice: blanks
 * its seven tiles in the text layer's tilemap.
 * HOW: Tile word $0020 (a blank) stored at the choice's offset plus 2 (row
 * 0), $40, $42, $44 (row 1) and $80, $82, $84 (row 2), with 16-bit A; P
 * kept.
 * Entry: X 16-bit = the choice's byte offset in the tilemap; called by JSL
 * (from the menu's cursor move, 81:EE04, which draws the mark at the new
 * choice by 81:EF39). Exit: P as at entry; A = $0020.
 */
void abi_MenuMarkErase(void)
{
    php();  /* 81EF15 php */
    op_rep(0x20);  /* 81EF16 rep */
    lda16_v(0x0020);  /* 81EF18 lda */
    sta16(ea_longx(0x7ED802u));  /* 81EF1B sta */
    sta16(ea_longx(0x7ED840u));  /* 81EF1F sta */
    sta16(ea_longx(0x7ED842u));  /* 81EF23 sta */
    sta16(ea_longx(0x7ED844u));  /* 81EF27 sta */
    sta16(ea_longx(0x7ED880u));  /* 81EF2B sta */
    sta16(ea_longx(0x7ED882u));  /* 81EF2F sta */
    sta16(ea_longx(0x7ED884u));  /* 81EF33 sta */
    plp();  /* 81EF37 plp */
    return;  /* 81EF38 rtl */
}



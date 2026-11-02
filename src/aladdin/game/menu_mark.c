/*
 * menu_mark.c: the selection mark of a menu drawn in the text layer: taken
 * away from the choice left; and the title menu's cursor moved. Drafted
 * by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    static const uint16_t cells[7] = { 2, 64, 66, 68, 128, 130, 132 };

    php();
    op_rep(P_M);
    a16_load(0x0020);
    for (int i = 0; i < 7; i++)
        wr16((TEXT_TILEMAP + cells[i] + cpu.x) & 0xFFFFFF, 0x0020);
    plp();
}



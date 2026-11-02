/*
 * menu_mark_draw.c: the selection mark of a menu drawn at a choice in the
 * text layer (MenuMarkErase, menu_mark.c, takes it away). Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_MenuMarkErase(void);
void abi_MenuMarkDraw(void);

#define MENU_CHOICE 0x000849u        /* the menu's choice */
#define MENU_CHOICE_WAS 0x00084Au    /* the choice the mark was drawn at */
#define MENU_CHOICE_PLACES 0xCE28u   /* by choice: its tilemap offset (words, the DB bank) */



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
    static const uint16_t cells[7] = { 2, 64, 66, 68, 128, 130, 132 };
    static const uint16_t tiles[7] = { 0x207E, 0x2802, 0x2003, 0x2004, 0x2812, 0x2013, 0x2014 };

    php();
    op_rep(P_M);
    for (int i = 0; i < 7; i++)
        wr16((TEXT_TILEMAP + cells[i] + cpu.x) & 0xFFFFFF, tiles[i]);
    a16_load(0x2014);
    plp();
}

/* The tilemap offset of the choice at byte `index` of the table, X 16-bit:
   X, N and Z as the load leaves them. */
static void choice_place(uint16_t index)
{
    cpu.y = index;
    cpu.x = rd16(dba(MENU_CHOICE_PLACES) + index);
    nz16_of(cpu.x);
}

/*
 * @routine 81:EE04 MenuMarkMove
 * WHAT IT DOES: Moves a menu's selection mark from the choice it was at
 * to the one now chosen.
 * HOW: MenuMarkErase at the tilemap offset of choice MENU_CHOICE_WAS
 * (from the table MENU_CHOICE_PLACES, by twice the choice), MenuMarkDraw
 * at that of MENU_CHOICE, with 16-bit X.
 * Entry: M 8-bit, X 8-bit; DB = $81, the table's bank; called by JSL
 * (81:EDD1, 80:9F42). Exit: M 8-bit, X 8-bit; P otherwise as MenuMarkDraw
 * leaves it; A = $2014; Y = twice the choice.
 */
void abi_MenuMarkMove(void)
{
    uint8_t was = rd8(dba(MENU_CHOICE_WAS)), now;

    a8_load((uint8_t)(was << 1));
    flag_set(P_C, was & 0x80);
    op_rep(0x10);
    choice_place((uint8_t)(was << 1));
    abi_MenuMarkErase();
    /* the TAY is 16-bit: A's high byte, as the erase left it, comes too */
    now = rd8(dba(MENU_CHOICE));
    a8_load((uint8_t)(now << 1));
    flag_set(P_C, now & 0x80);
    choice_place(cpu.a);
    abi_MenuMarkDraw();
    op_sep(0x10);
}

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
void abi_MenuCursorMove(void);

#define MENU_CHOICE 0x000849u        /* the menu's choice */
#define MENU_CHOICE_WAS 0x00084Au    /* the choice the mark was drawn at */
#define MENU_CHOICE_PLACES 0xCE28u   /* by choice: its tilemap offset (words, the DB bank) */
#define MENU_CURSOR_PLACES 0xCE1Fu   /* by choice: the one-tile cursor's tilemap offset (words, the DB bank) */
#define MENU_CURSOR_TILE 0x248E      /* the cursor's tile word */
#define MENU_CURSOR_MOVES 0x00084Bu  /* counted up at each move of the cursor */
#define MENU_CURSOR_BLANK 0x0020     /* a blank tile word */



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

/*
 * @routine 81:EE21 MenuCursorMoveLong
 * WHAT IT DOES: MenuCursorMove for callers in other banks.
 * HOW: JSR to it, then RTL. No script runs it: written from its bytes.
 * Entry: as MenuCursorMove; called by JSL. Exit: as MenuCursorMove.
 */
void abi_MenuCursorMoveLong(void)
{
    abi_MenuCursorMove();
}

/*
 * @routine 81:EE25 MenuCursorMove
 * WHAT IT DOES: Moves a menu's one-tile cursor from the choice it was at
 * to the one now chosen, and counts the move.
 * HOW: Tile word $0020 (a blank) at the tilemap offset of choice
 * MENU_CHOICE_WAS, MENU_CURSOR_TILE at that of MENU_CHOICE (both from
 * MENU_CURSOR_PLACES, by twice the choice; 16-bit A, then X); the upload
 * marked, MENU_CURSOR_MOVES counted up. No script runs it: written from
 * its bytes.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $CE1F holds MENU_CURSOR_PLACES (bank $80: $0454, $04D4, $0554);
 * called by JSR. Exit: M 8-bit, X 8-bit; A low = $8E; X = the new
 * choice's offset's low byte; N and Z from MENU_CURSOR_MOVES.
 */
void abi_MenuCursorMove(void)
{
    lda8(ea_abs(MENU_CHOICE_WAS));
    ACC8(asl);
    tax8();
    op_rep(P_M);
    lda16(ea_absx(MENU_CURSOR_PLACES));
    op_rep(P_X);
    tax16();
    lda16_v(MENU_CURSOR_BLANK);
    sta16(ea_longx(TEXT_TILEMAP));
    lda16(ea_abs(MENU_CHOICE));
    and16_v(0x00FF);
    ACC16(asl);
    tax16();
    lda16(ea_absx(MENU_CURSOR_PLACES));
    tax16();
    lda16_v(MENU_CURSOR_TILE);
    sta16(ea_longx(TEXT_TILEMAP));
    op_sep(0x30);
    MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));
    MEM8(inc, ea_abs(MENU_CURSOR_MOVES));
}

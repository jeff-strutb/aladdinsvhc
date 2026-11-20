/*
 * menu_mark_draw.c: the selection mark of a menu drawn at a choice in the
 * text layer (MenuMarkErase, menu_mark.c, takes it away).
 */
#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"

/* the mark's tile words, cell by cell (menu_mark_cells) */
static const uint16_t mark_tiles[MENU_MARK_CELLS] = { 0x207E, 0x2802, 0x2003, 0x2004, 0x2812, 0x2013, 0x2014 };

/*
 * @routine 81:EF39 MenuMarkDraw
 * WHAT IT DOES: Draws the selection mark at a menu choice: its seven tiles
 * in the text layer's map (at word `place`).
 */
void MenuMarkDraw(uint16_t place)
{
    unsigned i;

    for (i = 0; i < MENU_MARK_CELLS; i++)
        g.text_map[place + menu_mark_cells[i]] = mark_tiles[i];
}

/*
 * @routine 81:EE04 MenuMarkMove
 * WHAT IT DOES: Moves a menu's selection mark from the choice it was at
 * (menu_choice_was) to the one now chosen (menu_choice). The text map's
 * upload is left to the caller's next TextLayoutShow, as the original's.
 */
void MenuMarkMove(void)
{
    MenuMarkErase(menu_mark_place(g.menu_choice_was));
    MenuMarkDraw(menu_mark_place(g.menu_choice));
}

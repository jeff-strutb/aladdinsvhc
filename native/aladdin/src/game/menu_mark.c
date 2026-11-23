/*
 * menu_mark.c: the selection mark of a menu drawn in the text layer: taken
 * away from the choice left; and the title menu's cursor moved, its chosen
 * line blinking.
 */
#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/unconverted.h"
#include "video/video.h"

#define MENU_BLINK_SHOWN 0x20       /* frames a line is shown before it blinks off */
#define TITLE_IDLE_LEAST 0x04EC     /* frames the title waits idle, at least, after a move */
#define SOUND_MENU_MOVE 0x0C
#define BLANK_TILE 0x0020           /* the text layer's blank tile word */
#define LAYOUT_BLANKED 0x80         /* a layout number with this bit blanks its text */

#define LINE_ATTR 0x2400            /* the menu words' tile attributes: palette 1 (the tile is the letter's character code) */
#define SAVED_GAMES_AT (20 * 32 + 10)   /* the text map word of the second line's first letter: row 20, column 10 */
#define LEVEL_SELECT_AT (22 * 32 + 10)  /* the third line's: row 22, column 10 */

/* the mark's seven cells, in words from the choice's place: one in row 0,
   three in rows 1 and 2 */
const uint16_t menu_mark_cells[MENU_MARK_CELLS] = { 1, 32, 33, 34, 64, 65, 66 };

/*
 * @routine 81:EF15 MenuMarkErase
 * WHAT IT DOES: Takes the selection mark away from a menu choice: blanks
 * its seven tiles in the text layer's map (at word `place`).
 */
void MenuMarkErase(uint16_t place)
{
    unsigned i;

    for (i = 0; i < MENU_MARK_CELLS; i++)
        g.text_map[place + menu_mark_cells[i]] = BLANK_TILE;
}

/* The two lines the native game puts in place of the original's PASS WORD and OPTIONS (docs/aladdin/native/DEPARTURES.md),
   drawn as the original's lines are, in the same tiles (the letters' character codes), or blanked as a layout with bit 7 is. */
static void line_words(unsigned at, const char *text, int blanked)
{
    for (unsigned i = 0; text[i]; i++)
        g.text_map[at + i] = blanked ? BLANK_TILE : (uint16_t)(LINE_ATTR | (uint8_t)text[i]);
    g.text_map_upload++;
}

/*
 * TitleMenuLineShow: draws title menu choice `choice`'s line, or blanks it (`blanked`): GAME START by its text layout, as the
 * original's; the second and third lines, SAVED GAMES and LEVEL SELECT, by their words.
 */
void TitleMenuLineShow(unsigned choice, int blanked)
{
    if (choice == 0)
        TextLayoutShow(menu_line_layout(0) | (blanked ? LAYOUT_BLANKED : 0));
    else if (choice == 1)
        line_words(SAVED_GAMES_AT, "SAVED GAMES", blanked);
    else
        line_words(LEVEL_SELECT_AT, "LEVEL SELECT", blanked);
}

/*
 * @routine 81:EE55 TitleMenuBlink
 * WHAT IT DOES: Blinks the title menu's chosen line: shown for
 * MENU_BLINK_SHOWN frames, then blanked until its count runs out.
 * HOW: Shown: the frames counted up; at MENU_BLINK_SHOWN the line of the
 * choice blanked (its layout with bit 7). Blanked: the frames counted down
 * (from MENU_BLINK_SHOWN); at 0 the line of the choice before the last move
 * drawn again.
 */
void TitleMenuBlink(void)
{
    if (g.menu_blink_hidden) {
        if (--g.menu_blink_frames != 0)
            return;
        g.menu_blink_hidden = 0;
        TitleMenuLineShow(g.menu_choice_was, 0);
        return;
    }
    if (++g.menu_blink_frames != MENU_BLINK_SHOWN)
        return;
    g.menu_blink_hidden = 1;
    TitleMenuLineShow(g.menu_choice, 1);
}

/*
 * @routine 81:EDA6 TitleMenuCursor
 * @routine 81:EDA2 TitleMenuCursorLong
 * WHAT IT DOES: Moves the title screen menu's cursor on Up or Down
 * (wrapping round the three choices), with its mark, line and sound;
 * otherwise blinks the chosen line.
 * HOW: Pad 1's newly pressed buttons, Up tested first. A move: the mark
 * moved, TitleMenuBlink, the old choice's line drawn again (it may have
 * been blinked off), the blink restarted, the choice kept as the one the
 * mark is at, the title's idle countdown raised to at least
 * TITLE_IDLE_LEAST, and sound SOUND_MENU_MOVE.
 */
void TitleMenuCursor(void)
{
    if (g.pad_pressed[0] & PAD_UP)
        g.menu_choice = g.menu_choice == 0 ? MENU_CHOICES - 1 : (uint8_t)(g.menu_choice - 1);
    else if (g.pad_pressed[0] & PAD_DOWN)
        g.menu_choice = g.menu_choice + 1 == MENU_CHOICES ? 0 : (uint8_t)(g.menu_choice + 1);
    else {
        TitleMenuBlink();
        return;
    }
    MenuMarkMove();
    TitleMenuBlink();
    TitleMenuLineShow(g.menu_choice_was, 0);
    g.menu_blink_hidden = 0;
    g.menu_blink_frames = 0;
    g.menu_choice_was = g.menu_choice;
    if (g.play_countdown <= TITLE_IDLE_LEAST)
        g.play_countdown = TITLE_IDLE_LEAST;
    SoundQueueAdd(SOUND_MENU_MOVE);
}

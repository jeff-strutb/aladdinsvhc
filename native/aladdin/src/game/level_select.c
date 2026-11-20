/*
 * level_select.c: the level select (a development mode of the title's
 * menu): L or R held with a direction steps the level's part and stage,
 * and the level text shows them.
 */
#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/unconverted.h"
#include "video/video.h"

#define PART_LAST 6                 /* the last part (its text a title) */
#define LAYOUT_LAST_PART 0x5E
#define LAYOUT_PART_5_STAGE_3 0x60  /* part 5 stage 3, shown as "6.4" */
#define NUMBER_AT 0x20D             /* the level's number in the text map: row 16 from column 13: a space,
                                       the part, '.', the stage, a space (words, attribute $20) */
#define TILE_DIGIT_1 0x2031         /* '1': the part and stage are shown from 1 */
#define TILE_DOT 0x202E
#define TILE_SPACE 0x2020

/*
 * @routine 80:A0B1 LevelNumberText
 * WHAT IT DOES: Writes the level's part and stage into the text layer as
 * "P.S" (or a title in place of it for the last part, and for part 5
 * stage 3), and has the NMI upload it.
 */
void LevelNumberText(void)
{
    if (g.progress_sequence == PART_LAST) {
        TextLayoutShow(LAYOUT_LAST_PART);
        return;
    }
    if (g.progress_sequence == 5 && g.progress_stage == 3) {
        TextLayoutShow(LAYOUT_PART_5_STAGE_3);
        return;
    }
    g.text_map[NUMBER_AT + 1] = (uint16_t)((g.progress_sequence & 0x0F) + TILE_DIGIT_1);
    g.text_map[NUMBER_AT + 3] = (uint16_t)((g.progress_stage & 0x0F) + TILE_DIGIT_1);
    g.text_map[NUMBER_AT + 2] = TILE_DOT;
    g.text_map[NUMBER_AT] = TILE_SPACE;
    g.text_map[NUMBER_AT + 4] = TILE_SPACE;
    g.text_map_upload++;
}

/*
 * @routine 80:A05C LevelSelect
 * WHAT IT DOES: With L or R held, steps the level's part (up and down) or
 * its stage (right and left) within their limits and shows the new level:
 * returns 1 (the original's C clear, also when a step is refused). 0 when
 * neither L nor R is held or no direction is newly pressed.
 * HOW: Up: the part up by 1 unless the last, the stage 0; down: the part
 * down by 1 unless 0, the stage 0 (both may apply). Right: the stage up by
 * 1 unless the part's last; else left: down by 1 unless 0. A step refused
 * ends there (nothing shown); else LevelNumberText.
 */
int LevelSelect(void)
{
    uint16_t dir;

    if (!(g.pad_held[0] & (PAD_L | PAD_R)))
        return 0;
    dir = g.pad_pressed[0] & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT);
    if (!dir)
        return 0;
    if (dir & PAD_UP) {
        if (g.progress_sequence == PART_LAST)
            return 1;
        g.progress_sequence++;
        g.progress_stage = 0;
    }
    if (dir & PAD_DOWN) {
        if (g.progress_sequence == 0)
            return 1;
        g.progress_sequence--;
        g.progress_stage = 0;
    }
    if (dir & PAD_RIGHT) {
        if (g.progress_stage == level_select_last_stage(g.progress_sequence))
            return 1;
        g.progress_stage++;
    } else if (dir & PAD_LEFT) {
        if (g.progress_stage == 0)
            return 1;
        g.progress_stage--;
    }
    LevelNumberText();
    return 1;
}

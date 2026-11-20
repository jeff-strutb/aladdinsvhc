/*
 * level_tables.c: where a level's play starts. (The record's level_tables.c
 * holds the levels' other tables: stage 5's.)
 */
#include "data/system_tables.h"
#include "level/level.h"
#include "system/state.h"

#define VIEW_HALF 0x0080        /* the range point: the view's corner + this, in x and y */

/* 16.8 positions: the whole pixels set, the fraction kept */
static uint32_t whole_set(uint32_t at, uint16_t pixels)
{
    return (at & 0xFFu) | (uint32_t)pixels << 8;
}

/*
 * @routine 81:84B2 LevelStartPlace
 * WHAT IT DOES: Puts the player and the camera at the level's start (its
 * restart point): the player's place and facing, both layers' scroll, the
 * point actors are kept in range of, the scroll's limits across.
 * HOW: The play field's scroll, its position before the move and its drawn
 * edges take the camera's place; the backdrop takes it halved, in x unless
 * on level $10 or with a layer set-up mode, in y unless on levels 0, 3,
 * $0D, $14 or $1A. The range point is the scroll + $80 each way; the
 * scroll x between 0 and $7F00. (The original also clears the first byte
 * of the play field's layer record; that byte has no field yet: video/'s
 * ScrollLayer does not name bytes $00-$27.)
 */
void LevelStartPlace(void)
{
    LevelStart s = level_start((uint8_t)g.level, g.level_restart);
    Actor *player = &g.actors[0];
    ScrollLayer *field = &g.layers[0], *backdrop = &g.layers[1];
    uint16_t bx = s.camera_x, by = s.camera_y;

    player->x = whole_set(player->x, s.x);
    player->facing = s.facing_left;
    player->flip = s.facing_left;
    player->y = whole_set(player->y, s.y);
    field->scroll_x = whole_set(field->scroll_x, s.camera_x);
    g.scroll_x_before = s.camera_x;
    field->drawn_x = s.camera_x;
    if (g.level != 0x10 && !g.layer_setup_mode)
        bx >>= 1;
    backdrop->scroll_x = whole_set(backdrop->scroll_x, bx);
    backdrop->drawn_x = bx;
    field->scroll_y = whole_set(field->scroll_y, s.camera_y);
    g.scroll_y_before = s.camera_y;
    field->drawn_y = s.camera_y;
    if (g.level != 0x00 && g.level != 0x03 && g.level != 0x0D && g.level != 0x14 && g.level != 0x1A)
        by >>= 1;
    backdrop->scroll_y = whole_set(backdrop->scroll_y, by);
    backdrop->drawn_y = by;
    g.range_x = (uint16_t)(s.camera_x + VIEW_HALF);
    g.range_y = (uint16_t)(s.camera_y + VIEW_HALF);
    g.scroll_x_min = 0;
    g.scroll_x_max = 0x7F00;
}

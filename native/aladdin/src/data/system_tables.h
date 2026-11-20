/*
 * system_tables.h: the level set-up's tables from the cartridge
 * (game/level_load.c, level/level_tables.c), read through data/. Owned by
 * the stage 4 system session.
 */
#ifndef ALADDIN_SYSTEM_TABLES_H
#define ALADDIN_SYSTEM_TABLES_H

#include <stdint.h>

#define LEVELS_WITH_SETS 0x1F   /* the levels below this have graphics sets of their own */

/* a level's own packed graphics sets (LevelGraphicsLoadAll); negative: none */
int8_t level_gfx_set_a(uint8_t level);  /* level < LEVELS_WITH_SETS */
int8_t level_gfx_set_b(uint8_t level);

/* where a level's play starts, by its restart point (LevelStartPlace) */
typedef struct LevelStart {
    uint16_t x, y;              /* the player's place, pixels */
    uint8_t facing_left;
    uint16_t camera_x, camera_y;    /* the play field's scroll, pixels */
} LevelStart;
LevelStart level_start(uint8_t level, uint8_t restart);

#endif

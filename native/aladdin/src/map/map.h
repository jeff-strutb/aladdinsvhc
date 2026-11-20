/*
 * map.h: the level's map: building it (level_map.c) and changing its tiles
 * while it is played (tile_changes.c). Stage 5's map routines add theirs
 * here.
 */
#ifndef ALADDIN_MAP_H
#define ALADDIN_MAP_H

#include <stdint.h>

/* level_map.c */
void LevelMapBuild(void);               /* 81:BEEF (LevelMapBuildLong 81:BEEB): for g.level */
void ScreenMapLayout(uint16_t level, unsigned layout, unsigned first_place);    /* 81:BFEE */

/* tile_changes.c */
void TileChangesApply(void);            /* 81:999A */
uint16_t MapTileAddress(uint16_t x, uint16_t y);    /* 81:9C50: the tile under the point, its index in g.level_tiles */

#endif

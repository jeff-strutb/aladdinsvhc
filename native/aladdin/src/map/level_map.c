/*
 * level_map.c: building a level's map.
 *
 * A level's map is screens of 16 x 16 tiles (16 pixels each), each built
 * from 8 x 8 blocks of 2 x 2 tiles, kept one after another in
 * g.level_tiles, and a screen map that places them (g.screen_map, 32
 * screens across). The level's map header gives the screens to build and
 * where their data is (data.h, "level maps").
 */
#include "data/data.h"
#include "map/map.h"
#include "system/state.h"
#include "video/video.h"

#define SCREEN_TILES 0x100          /* a screen's tiles (words) */
#define TILE_ROW 16                 /* tiles a screen's row */
#define BLOCKS_A_SIDE 8
#define LAYOUT_MAIN_PLACE 0x21      /* the main layout's first place: row 1, column 1 */
#define LAYOUT_SECOND_PLACE 0x221   /* the second's: row 17, column 1 */
#define SCREEN_MAP_ROW 32

/*
 * @routine 81:BEEB LevelMapBuildLong
 * @routine 81:BEEF LevelMapBuild
 * WHAT IT DOES: Builds the level's map: its screens' tiles and the screen
 * map that places them.
 * HOW: The screen map cleared (with the animation sets that share its
 * bytes, as in the original) and the block source list emptied; the
 * header's layers go to the main and sub screens; for each screen built,
 * its block list noted (BlockSourceAdd) and each of its 64 blocks' 2 x 2
 * tiles written into its 16 x 16 tiles; then the screen map laid out from
 * the level's layouts (ScreenMapLayout), the second where it has one. The
 * level is kept as the map's (g.map_level), for the readers of its
 * metatiles. The original copies each screen's block list with 4 bytes
 * more to $1A94-$1B17; the 4 ($1B14-$1B17, the last list's) are not
 * carried: no field of g holds those bytes yet.
 */
void LevelMapBuild(void)
{
    uint16_t level = g.level, at = 0;
    unsigned screens = level_map_screens(level), s;

    for (s = 0; s < SCREEN_MAP_PLACES; s++)
        g.screen_map.places[s] = 0;
    g.block_sources_end = 0;
    g.map_level = level;
    g.main_layers = level_map_layers(level);
    g.sub_layers = level_map_layers(level);
    for (s = 0; s < screens; s++) {
        uint8_t screen = level_map_screen(level, s);
        unsigned row, col;

        BlockSourceAdd((uint16_t)(screen << 7));
        for (row = 0; row < BLOCKS_A_SIDE; row++)
            for (col = 0; col < BLOCKS_A_SIDE; col++) {
                unsigned i = 2 * (row * BLOCKS_A_SIDE + col);
                uint16_t block = (uint16_t)(level_screen_list_byte(level, screen, i) |
                                            level_screen_list_byte(level, screen, i + 1) << 8);
                uint16_t t = (uint16_t)(at + row * 2 * TILE_ROW + col * 2);

                g.level_tiles[t % LEVEL_TILES_WORDS] = level_block_tile(level, block, 0);
                g.level_tiles[(t + 1) % LEVEL_TILES_WORDS] = level_block_tile(level, block, 1);
                g.level_tiles[(t + TILE_ROW) % LEVEL_TILES_WORDS] = level_block_tile(level, block, 2);
                g.level_tiles[(t + TILE_ROW + 1) % LEVEL_TILES_WORDS] = level_block_tile(level, block, 3);
            }
        at = (uint16_t)(at + SCREEN_TILES);
    }
    ScreenMapLayout(level, 0, LAYOUT_MAIN_PLACE);
    if (level_map_layout_count(level) > 1)
        ScreenMapLayout(level, 1, LAYOUT_SECOND_PLACE);
}

/*
 * @routine 81:BFEE ScreenMapLayout
 * WHAT IT DOES: Writes a layout into the screen map: which screen goes in
 * each place.
 * HOW: Row by row from `first_place`, each place's high byte becomes its
 * screen number times 2 (8 bits, as the original's ASL), so its word is the
 * byte offset of the screen's tiles; its low byte is kept.
 */
void ScreenMapLayout(uint16_t level, unsigned layout, unsigned first_place)
{
    unsigned columns = level_map_layout_columns(level, layout);
    unsigned rows = level_map_layout_rows(level, layout);
    unsigned row, col, i = 0;

    for (row = 0; row < rows; row++)
        for (col = 0; col < columns; col++) {
            uint16_t *place = &g.screen_map.places[(first_place + row * SCREEN_MAP_ROW + col) % SCREEN_MAP_PLACES];
            uint8_t high = (uint8_t)(level_map_layout_screen(level, layout, i++) << 1);

            *place = (uint16_t)((*place & 0x00FFu) | high << 8);
        }
}

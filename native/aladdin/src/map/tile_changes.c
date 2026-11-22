/*
 * tile_changes.c: changing a tile of the level's map while it is played
 * (a door opening, a block broken), in the map and on screen.
 *
 * Not converted, with their proof (no JSR, JSL, JMP or table word names
 * them; the record's own notes): TileRectQueue (81:9BAE), PointOffsetAt
 * (81:9C8D), MapTileRestore (81:9A9C), MapTileRestoreShown (81:9AAC),
 * MapTileSetShown (81:9B2A). MapTileRestoreAtOffset (81:9A92) and the
 * routines only it reaches (MapTileRestoreShownTail 81:9AB4,
 * MapTileSourceOf 81:9ABF, MapTileSourceAt 81:9AC2, MapTileBlockQueue
 * 81:9AF6): its one caller is in actor/pool.c (81:EA78); converted when
 * that caller is.
 */
#include "data/data.h"
#include "data/game_tables.h"
#include "map/map.h"
#include "system/state.h"
#include "video/level_maps.h"
#include "video/video.h"

/*
 * @routine 81:9C50 MapTileAddress
 * WHAT IT DOES: The map tile under point x, y: its index in the level's
 * tiles.
 * HOW: The point rounded to the 16-pixel tile; its screen's place in the
 * screen map (x / 256 across, y / 256 down, from row 1, column 1) gives
 * the byte offset of the screen's tiles; plus the tile's place in the
 * screen (16 across). Every sum is the original's, in 16 bits. (The
 * original also leaves the rounded point and the parts on the direct page;
 * no caller on the boot path reads them.)
 */
uint16_t MapTileAddress(uint16_t x, uint16_t y)
{
    unsigned place = 0x21u + ((y >> 2) & 0x03C0u) / 2 + ((x >> 8) & 0x1Fu);
    uint16_t screen = g.screen_map.places[place % SCREEN_MAP_PLACES];
    uint16_t in_screen = (uint16_t)(((x >> 3) & 0x001Eu) | ((y << 1) & 0x01E0u));

    return (uint16_t)((uint16_t)(screen + in_screen) / 2);
}

/*
 * @routine 81:9A64 TileChangeQueueQuarter
 * Removed: it queued one 8 x 8 quarter of a changed tile for the NMI to
 * write to the play field's ring. The play field is a whole-level map now
 * (video/level_maps.c): LevelMapsTile writes the four quarters at once.
 * Removed with it: the helper that tested the change was in the ring's
 * drawn area (with 8 pixels round it, a static helper with no routine of its
 * own); the map has every tile, so every change is made.
 */

/*
 * @routine 81:999A TileChangesApply
 * WHAT IT DOES: Applies the map tile changes queued this frame: each
 * change's new tile goes into the level's tiles and into the play field's
 * map (the four tiles of the 16 x 16 tile). Then the change queue empties.
 * HOW: The original tests the count's low byte alone before it starts, as
 * here; it wrote only the changes inside the ring's drawn area, as quarters
 * queued for the NMI (bottom right first), and asked for the NMI's write
 * when any were queued: the map is whole, so each change is written (the
 * word queue it used, g.vram_words, is no longer written here).
 */
void TileChangesApply(void)
{
    unsigned i = 0;

    if (!(g.tile_changes_count & 0xFFu))
        return;
    do {
        TileChange *c = &g.tile_changes[i++ % TILE_CHANGES_SIZE];

        g.level_tiles[MapTileAddress(c->x, c->y) % LEVEL_TILES_WORDS] = c->tile;
        LevelMapsTile(c->x, c->y, c->tile);
    } while (--g.tile_changes_count);
    g.tile_changes_end = 0;
}

/*
 * @routine 81:9978 TileChangeAdd
 * WHAT IT DOES: Queues a change of the map tile at a point to a new tile,
 * for TileChangesApply to make.
 * HOW: The record at the queue's end gets the point's x and y, each with
 * bit 3 set (the tile's middle), and the new tile; the end moves on a
 * record and the count steps up (no bound, as the original's).
 */
void TileChangeAdd(uint16_t tile, uint16_t x, uint16_t y)
{
    TileChange *c = &g.tile_changes[g.tile_changes_end % TILE_CHANGES_SIZE];

    c->tile = tile;
    c->y = (uint16_t)(y | 0x0008);
    c->x = (uint16_t)(x | 0x0008);
    g.tile_changes_end++;
    g.tile_changes_count++;
}

/*
 * @routine 81:9A84 MapTileAt
 * WHAT IT DOES: The level's map tile under point x, y.
 */
uint16_t MapTileAt(uint16_t x, uint16_t y)
{
    return g.level_tiles[MapTileAddress(x, y) % LEVEL_TILES_WORDS];
}

/*
 * @routine 81:9B6D MapTileSet
 * WHAT IT DOES: Puts tile `tile` into the level's map under point x, y
 * (not queued for the screen: TileChangeAdd does both).
 * HOW: The original drew nothing here, and the tile showed once the ring
 * next drew that column or row from the level's tiles (level 5's closing
 * scene copies a block of tiles this way and the block is on screen). The
 * play field's whole-level map is the level's tiles as they are, so it is
 * written too (LevelMapsTile).
 */
void MapTileSet(uint16_t tile, uint16_t x, uint16_t y)
{
    g.level_tiles[MapTileAddress(x, y) % LEVEL_TILES_WORDS] = tile;
    LevelMapsTile(x, y, tile);
}

/*
 * @routine 81:9B7D MapQuarterAt
 * WHAT IT DOES: The tilemap word of the 8 x 8 quarter of the map tile
 * under point x, y.
 * HOW: The tile's word plus the quarter's offset (x bit 3 gives 2, y bit 3
 * gives 4) indexes the level's metatiles. (The original also leaves the
 * offset at $12; its one caller, kind $3B, does not read it.)
 */
uint16_t MapQuarterAt(uint16_t x, uint16_t y)
{
    uint16_t quarter = (uint16_t)(((x & 0x0008) >> 2) | ((y & 0x0008) >> 1));

    return metatile_word(level_metatiles(g.map_level), (uint16_t)(MapTileAt(x, y) + quarter));
}

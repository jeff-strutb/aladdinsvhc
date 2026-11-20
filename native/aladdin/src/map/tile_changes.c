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
#include "video/video.h"

#define VIEW_WIDTH 0x0120           /* the box a change is drawn in: the drawn area with 8 pixels round it */
#define VIEW_HEIGHT 0x00F8
#define VIEW_MARGIN 8

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
 * WHAT IT DOES: Queues one 8 x 8 quarter of a changed map tile for the NMI
 * to write to video memory: at the tile's VRAM word plus the quarter's
 * offset, the quarter's word of the new tile's metatile.
 */
static void TileChangeQueueQuarter(unsigned *queue_at, uint16_t vram, uint16_t tile, unsigned quarter)
{
    TileQuarter q = tile_quarter(quarter);
    VramWord *w = &g.vram_words[*queue_at % VRAM_WORDS_SIZE];

    w->to = (uint16_t)(vram + q.vram_offset);
    w->word = metatile_word(level_metatiles(g.map_level), (uint16_t)(tile + 2u * q.place));
    g.vram_words_count++;
    ++*queue_at;
}

/* The point inside the box around the drawn area, on one axis: as the
   original's (at - middle + half) < size, in 16 bits. */
static int in_view(uint16_t at, uint16_t drawn, uint16_t size)
{
    uint16_t from = (uint16_t)(drawn - VIEW_MARGIN);
    uint16_t half = (uint16_t)(size / 2);
    uint16_t middle = (uint16_t)(from + half);

    return (uint16_t)(at - middle + half) < size;
}

/*
 * @routine 81:999A TileChangesApply
 * WHAT IT DOES: Applies the map tile changes queued this frame: each
 * change's new tile goes into the level's tiles, and one inside the drawn
 * area (with 8 pixels round it) has its four quarters queued for the NMI
 * to write to video memory, bottom right first. Then the change queue
 * empties, and the NMI's write is asked for when anything was queued.
 * HOW: The original tests the count's low byte alone before it starts (and
 * the queue count's low byte at the end), as here; its word queue starts
 * again from its first record, as here.
 */
void TileChangesApply(void)
{
    unsigned queue_at = 0, i = 0;
    uint16_t w = (uint16_t)(g.layers[0].drawn_x + VIEW_WIDTH - VIEW_MARGIN - (uint16_t)(g.layers[0].drawn_x - VIEW_MARGIN));
    uint16_t h = (uint16_t)(g.layers[0].drawn_y + VIEW_HEIGHT - VIEW_MARGIN - (uint16_t)(g.layers[0].drawn_y - VIEW_MARGIN));

    if (!(g.tile_changes_count & 0xFFu))
        return;
    g.vram_words_count = 0;
    do {
        TileChange *c = &g.tile_changes[i++ % TILE_CHANGES_SIZE];
        uint16_t vram = TilemapWord(c->x, c->y);

        g.level_tiles[MapTileAddress(c->x, c->y) % LEVEL_TILES_WORDS] = c->tile;
        if (in_view(c->x, g.layers[0].drawn_x, w) && in_view(c->y, g.layers[0].drawn_y, h)) {
            TileChangeQueueQuarter(&queue_at, vram, c->tile, 3);
            TileChangeQueueQuarter(&queue_at, vram, c->tile, 2);
            TileChangeQueueQuarter(&queue_at, vram, c->tile, 1);
            TileChangeQueueQuarter(&queue_at, vram, c->tile, 0);
        }
    } while (--g.tile_changes_count);
    g.tile_changes_end = 0;
    if (g.vram_words_count & 0xFFu)
        g.vram_words_pending++;
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
 * (not on screen: TileChangeAdd does both).
 */
void MapTileSet(uint16_t tile, uint16_t x, uint16_t y)
{
    g.level_tiles[MapTileAddress(x, y) % LEVEL_TILES_WORDS] = tile;
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

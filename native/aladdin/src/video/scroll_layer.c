/*
 * scroll_layer.c: keeping each scroll layer's video map ahead of its scroll.
 *
 * A layer's video map holds a little more than the screen. As the layer
 * scrolls, the column or row of tiles about to come into view is drawn from
 * the level's map: the screen map (g.screen_map.places) places screens, each 16 by
 * 16 metatiles kept in the level tiles (the unpack buffer from its start),
 * each metatile 2 by 2 tiles in the level's metatiles. Each layer record
 * (g.layers: 0 the play field, 1 the backdrop) keeps four edges, each a
 * place in its video map and the same place in the level's map, and steps
 * them a tile at a time as it draws.
 */
#include <stdio.h>

#include "data/data.h"
#include "data/scroll_tables.h"
#include "system/state.h"
#include "video/scroll_layer.h"
#include "video/video.h"
#include "video/video_internal.h"

enum { EDGE_LEFT, EDGE_RIGHT, EDGE_TOP, EDGE_BOTTOM };

/* each edge's bit in a layer's edge mask */
static const uint8_t edge_bit[4] = { 0x02, 0x01, 0x08, 0x04 };

/* a scroll position's whole pixels: the middle two bytes of its 16.8 */
static uint16_t pixels(uint32_t position)
{
    return (uint16_t)(position >> 8);
}

/* the word of the screen map at byte offset `at`; negative entries are
   none, list 0 */
static uint16_t screen_at(uint16_t at)
{
    uint16_t w;

    if (at >= sizeof g.screen_map.places) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: a scroll layer reads the screen map at %04X, past its $800 bytes; taken as none\n", at);
        }
        return 0;
    }
    w = g.screen_map.places[at / 2u];
    return (w & 0x8000) ? 0 : w;
}

/* the word of the level tiles (g.level_tiles, the original's bank $7F) at
   byte offset `at`: a metatile's address. An odd offset (none of the
   record's) is reported and read at the word below. */
static uint16_t level_tile_word(uint32_t at)
{
    if ((at & 1) || at / 2u >= LEVEL_TILES_WORDS) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: a scroll layer reads the level tiles at %05X, odd or past them\n", at);
        }
        if (at / 2u >= LEVEL_TILES_WORDS)
            return 0;
    }
    return g.level_tiles[at / 2u];
}

/*
 * @routine 81:C032 LevelScrollLimits
 * WHAT IT DOES: Sets how far the play field may scroll, right and down,
 * from the level's main layout: its width and height in screens, less one,
 * times 256.
 * HOW: These are the original's, for its 256 x 224 view: the last view's
 * left and top edge. ScrollUpdate takes the view's (view.h) 2 dx and 2 dy
 * off them, so the view's right and lower edges stop at the level's, and
 * the player's fall (PlayerFallenTest) stays at (rows - 1) * 256 + $E6.
 * The record reads the layout's size bytes (width less one, height
 * less one) from the layouts' table; data/'s counts are those plus one for
 * every level the table has (no size byte is $80 or more).
 */
void LevelScrollLimits(void)
{
    g.scroll_limit_x = (uint16_t)((level_map_layout_columns(g.level, 0) - 1u) << 8);
    g.scroll_limit_y = (uint16_t)((level_map_layout_rows(g.level, 0) - 1u) << 8);
}

/*
 * @routine 81:C276 LayerQueueColumn
 * WHAT IT DOES: When the layer has scrolled 6 pixels or more across from
 * where its tiles are drawn to, queues the next column of tiles to draw (on
 * the side it is scrolling to: 0 right, 1 left) and moves the drawn
 * position 8 pixels that way.
 * @routine 81:C2AB LayerQueueRow
 * WHAT IT DOES: The same up or down (0 down, 1 up), for a row.
 * HOW: Both one function here, by `rows`. The queues are rings of 8; their
 * heads and tails are the original's byte offsets (4 an entry).
 */
static void layer_queue(int layer, int rows)
{
    ScrollLayer *l = &g.layers[layer];
    uint16_t scroll = pixels(rows ? l->scroll_y : l->scroll_x), *drawn = rows ? &l->drawn_y : &l->drawn_x;
    uint16_t *tail = rows ? &g.row_queue_tail : &g.column_queue_tail;
    LayerEdge *queue = rows ? g.row_queue : g.column_queue;
    uint16_t d = (uint16_t)(scroll - *drawn);
    uint8_t side = 0;

    if (d & 0x8000) {
        side = 1;
        d = (uint16_t)(0u - d);
    }
    if (d < 6)
        return;
    queue[(*tail & 0x1F) / 4u].layer = (uint8_t)layer;
    queue[(*tail & 0x1F) / 4u].side = side;
    *tail = (uint16_t)((*tail + 4) & 0x1F);
    *drawn = (uint16_t)(*drawn + layer_draw_step(side));
}

/*
 * @routine 81:C498 EdgeStepLeft
 * WHAT IT DOES: Moves a layer's edge one tile left, when the layer draws
 * that edge: the video map column back, from 0 to 31 of the other screen;
 * the tile in the metatile between halves, and from the left half to the
 * right half of the metatile to its left: the metatile back, from the
 * screen's first column to its last of the screen to the left.
 */
static void edge_step_left(ScrollLayer *l, int e)
{
    ScrollEdge *edge = &l->edge[e];
    uint8_t lo, hi;

    if (!(l->edges & edge_bit[e]))
        return;
    lo = (uint8_t)(edge->tilemap - 1);
    hi = (uint8_t)(edge->tilemap >> 8);
    if ((lo & 0x1F) == 0x1F) {
        lo = (uint8_t)((uint8_t)(lo + 1) | 0x1F);
        hi ^= 0x04;
    }
    edge->tilemap = (uint16_t)(hi << 8 | lo);
    edge->half ^= 0x02;
    if (!(edge->half & 0x02))
        return;
    lo = (uint8_t)edge->block;
    if (lo & 0x1E) {
        edge->block = (uint16_t)((edge->block & 0xFF00) | (uint8_t)(lo - 2));
        return;
    }
    edge->block = (uint16_t)((edge->block & 0xFF00) | (uint8_t)(lo | 0x1E));
    edge->map = (uint16_t)(edge->map - 2);
}

/*
 * @routine 81:C4DA EdgeStepRight
 * WHAT IT DOES: Moves a layer's edge one tile right: the video map column
 * on, from 31 to 0 of the other screen; the tile in the metatile between
 * halves, and from the right half to the left half of the metatile to its
 * right: the metatile on, from the screen's last column to its first of
 * the screen to the right.
 */
static void edge_step_right(ScrollLayer *l, int e)
{
    ScrollEdge *edge = &l->edge[e];
    uint8_t lo = (uint8_t)(edge->tilemap + 1), hi = (uint8_t)(edge->tilemap >> 8);

    if ((lo & 0x1F) == 0) {
        lo = (uint8_t)((uint8_t)(lo - 1) & 0xE0);
        hi ^= 0x04;
    }
    edge->tilemap = (uint16_t)(hi << 8 | lo);
    edge->half ^= 0x02;
    if (edge->half & 0x02)
        return;
    lo = (uint8_t)edge->block;
    if ((lo & 0x1E) != 0x1E) {
        edge->block = (uint16_t)((edge->block & 0xFF00) | (uint8_t)(lo + 2));
        return;
    }
    edge->block = (uint16_t)((edge->block & 0xFF00) | (uint8_t)(lo & 0xE0));
    edge->map = (uint16_t)(edge->map + 2);
}

/*
 * @routine 81:C4D6 EdgeStepRightIfDrawn
 * WHAT IT DOES: EdgeStepRight, when the layer draws that edge.
 */
static void edge_step_right_if_drawn(ScrollLayer *l, int e)
{
    if (l->edges & edge_bit[e])
        edge_step_right(l, e);
}

/*
 * @routine 81:C516 EdgeStepUp
 * WHAT IT DOES: Moves a layer's edge one tile up, when the layer draws that
 * edge: the video map row back, from 0 to 31 (the layer's screen bits
 * flipped); the tile in the metatile between halves, and on to the
 * metatile above: back a row, from the screen's first row to its last of
 * the screen above.
 */
static void edge_step_up(ScrollLayer *l, int e)
{
    ScrollEdge *edge = &l->edge[e];
    uint16_t t = edge->tilemap;

    if (!(l->edges & edge_bit[e]))
        return;
    edge->tilemap = (uint16_t)(t - 0x0020);
    if ((edge->tilemap & 0x03E0) == 0x03E0)
        edge->tilemap = (uint16_t)((t ^ l->screen_flip) | 0x03E0);
    edge->half ^= 0x0004;
    if (!(edge->half & 0x0004))
        return;
    edge->block = (uint16_t)((edge->block - 0x0020) & 0x01FE);
    if (edge->block >= 0x01E0)
        edge->map = (uint16_t)(edge->map - 0x0040);
}

/*
 * @routine 81:C55C EdgeStepDown
 * WHAT IT DOES: Moves a layer's edge one tile down, when the layer draws
 * that edge: the video map row on, from 31 to 0 (the layer's screen bits
 * flipped); the tile in the metatile between halves, and on to the
 * metatile below: on a row, from the screen's last row to its first of the
 * screen below.
 */
static void edge_step_down(ScrollLayer *l, int e)
{
    ScrollEdge *edge = &l->edge[e];
    uint16_t t = edge->tilemap;

    if (!(l->edges & edge_bit[e]))
        return;
    edge->tilemap = (uint16_t)(t + 0x0020);
    if ((edge->tilemap & 0x03E0) == 0)
        edge->tilemap = (uint16_t)((t ^ l->screen_flip) & 0xFC1F);
    edge->half ^= 0x0004;
    if (edge->half & 0x0004)
        return;
    edge->block = (uint16_t)((edge->block + 0x0020) & 0x01FE);
    if (edge->block < 0x0020)
        edge->map = (uint16_t)(edge->map + 0x0040);
}

/*
 * @routine 81:C41F LayerStepAcross
 * WHAT IT DOES: After a column is drawn, moves the layer's four edges a
 * tile the way it scrolled: by the column's side (0 right)
 * EdgeStepRightIfDrawn or EdgeStepLeft, for the left, right, top and bottom
 * edges in turn.
 */
static void layer_step_across(ScrollLayer *l)
{
    for (int e = 0; e < 4; e++) {
        if (l->column_side == 0)
            edge_step_right_if_drawn(l, e);
        else
            edge_step_left(l, e);
    }
}

/*
 * @routine 81:C45C LayerStepUpDown
 * WHAT IT DOES: After a row is drawn, moves the layer's four edges a tile
 * the way it scrolled: by the row's side (0 down) EdgeStepDown or
 * EdgeStepUp, for the left, right, top and bottom edges in turn.
 */
static void layer_step_up_down(ScrollLayer *l)
{
    for (int e = 0; e < 4; e++) {
        if (l->row_side == 0)
            edge_step_down(l, e);
        else
            edge_step_up(l, e);
    }
}

/*
 * @routine 81:C0F7 LayerSetup
 * WHAT IT DOES: Sets a scroll layer up for a level: where its video map and
 * screen map are, how long its columns and rows are, where its metatiles
 * come from, and that it draws all four edges.
 * HOW: The layer's entry of the set-up table: the screen map base's,
 * video map base's and screen flip's high bytes and the column and row
 * lengths' low bytes (the other bytes kept, as the original's byte
 * stores leave them), the video map mask. The metatile address's high byte
 * $80; the metatiles' bank, the one LevelMapBuild took from g.level's map
 * header ($005C), here read again from that header.
 */
static void layer_setup(int layer)
{
    ScrollLayer *l = &g.layers[layer];
    LayerSetupEntry e = layer_setup_entry((unsigned)layer);

    l->map_base = (uint16_t)((l->map_base & 0x00FF) | e.map_base_high << 8);
    l->tilemap_base = (uint16_t)((l->tilemap_base & 0x00FF) | e.tilemap_base_high << 8);
    l->tilemap_mask = e.tilemap_mask;
    l->screen_flip = (uint16_t)((l->screen_flip & 0x00FF) | e.screen_flip_high << 8);
    l->column_length = (uint16_t)((l->column_length & 0xFF00) | e.column_length);
    l->row_length = (uint16_t)((l->row_length & 0xFF00) | e.row_length);
    l->metatile = (uint16_t)((l->metatile & 0x00FF) | 0x8000);
    l->metatiles = level_metatiles(g.map_level);   /* the bank LevelMapBuild noted ($005C), for the level it built */
    l->edges = 0x0F;
}

/*
 * @routine 81:C177 TilemapWord
 * WHAT IT DOES: Gives the video map word address, in the play field's 64 by
 * 32 map, of the tile at a pixel position: the column from x bits 3-7, the
 * row from y bits 3-7, the second screen from x bit 8, over $4800.
 * HOW: TilemapWordLong (81:C173), its call for other banks, is this.
 */
uint16_t TilemapWord(uint16_t x, uint16_t y)
{
    return (uint16_t)(0x4800 | (x & 0x0100 ? 0x0400 : 0) | (y & 0x00F8) << 2 | (x & 0x00FF) >> 3);
}

/*
 * @routine 81:C1A2 EdgeMapPlace
 * WHAT IT DOES: Puts a layer's edge at a pixel position in the level's map:
 * its screen (one over and one down: the screen map has a border, plus the
 * layer's screen map base), its metatile in the screen from bits 4-7, its
 * tile's half from bit 3.
 */
static void edge_map_place(ScrollLayer *l, int e, uint16_t x, uint16_t y)
{
    ScrollEdge *edge = &l->edge[e];
    uint16_t across = (uint16_t)(((x >> 8) + 1) << 1 & 0x00FE);
    uint16_t down = (uint16_t)(((uint16_t)(y + 0x0100) & 0xFF00) >> 2);

    edge->map = (uint16_t)(down + across + l->map_base);
    edge->block = (uint16_t)((y << 1 & 0x01E0) | (x >> 3 & 0x001E));
    edge->half = (uint16_t)((x >> 2 & 0x0002) | (y >> 1 & 0x0004));
}

/*
 * @routine 81:C163 EdgePlaceAt
 * WHAT IT DOES: Puts a layer's edge at a pixel position: its video map word
 * (TilemapWord, kept to the layer's mask, with its base bits) and its place
 * in the level's map.
 */
static void edge_place_at(ScrollLayer *l, int e, uint16_t x, uint16_t y)
{
    l->edge[e].tilemap = (uint16_t)((TilemapWord(x, y) & l->tilemap_mask) | l->tilemap_base);
    edge_map_place(l, e, x, y);
}

/*
 * @routine 81:C146 EdgePlace
 * WHAT IT DOES: Puts one of a layer's edges where it starts: its scroll plus
 * the edge's start from the table, just off the screen on its side.
 */
static void edge_place(ScrollLayer *l, int e)
{
    edge_place_at(l, e, (uint16_t)(pixels(l->scroll_x) + edge_start_x((unsigned)e)),
                  (uint16_t)(pixels(l->scroll_y) + edge_start_y((unsigned)e)));
}

/*
 * @routine 81:C128 LayerPlaceEdges
 * WHAT IT DOES: Puts a layer's four edges where they start, from its
 * scroll: left, right, top, bottom.
 */
static void layer_place_edges(ScrollLayer *l)
{
    for (int e = 0; e < 4; e++)
        edge_place(l, e);
}

/*
 * @routine 81:C0B6 LayersSetup
 * WHAT IT DOES: Sets both scroll layers up for the level and puts their
 * edges where they start; in levels 0, 2, 3, $14 and $15 the backdrop's
 * rows are 68 tiles long (the length's low byte $44).
 */
void LayersSetup(void)
{
    uint8_t level = (uint8_t)g.level;

    layer_setup(0);
    layer_setup(1);
    layer_place_edges(&g.layers[0]);
    layer_place_edges(&g.layers[1]);
    if (level == 0x00 || level == 0x15 || level == 0x14 || level == 0x02 || level == 0x03)
        g.layers[1].row_length = (uint16_t)((g.layers[1].row_length & 0xFF00) | 0x44);
}

/*
 * @routine 81:C2E0 LayerDrawColumn
 * WHAT IT DOES: Draws the column of tiles coming into view on a side of a
 * layer (0 right, 1 left), from the level's map, ready to be copied to its
 * video map: from the side's edge, down the map, each screen's metatile
 * list, each metatile's tile in the edge's column half, top and bottom in
 * turn, into the column ring from the edge's row on, for the column length.
 * Then the video map word address of its top and its parts (two when
 * longer than 32 tiles).
 */
static void column_tiles(ScrollLayer *l, const ScrollEdge *edge)
{
    uint16_t map = edge->map, block = edge->block, half = edge->half, left = l->column_length;
    uint16_t t = (uint16_t)((edge->tilemap & 0xFBFF) >> 5);
    unsigned at = (unsigned)(((t & 0x0040) ? (t | 0x0020) : t) << 1 & 0x003F);

    for (;;) {                                   /* each screen down */
        l->block = screen_at(map);
        for (;;) {                               /* each metatile down */
            uint16_t y = half;

            l->metatile = (uint16_t)(level_tile_word((uint32_t)l->block + block) | 0x8000);
            for (;;) {                           /* its two tiles */
                l->column[at / 2u] = level_metatile_word(l->metatiles, l->metatile, y);
                if (--left == 0)
                    return;
                at = (at + 2) & 0x003F;
                y ^= 0x0004;
                if (!(y & 0x0004))
                    break;
            }
            half = y;
            block = (uint16_t)((block + 0x0020) & 0x01FE);
            if (block < 0x0020)
                break;
        }
        map = (uint16_t)(map + 0x0040);
    }
}

static void layer_draw_column(ScrollLayer *l, unsigned side)
{
    ScrollEdge *edge = &l->edge[side_edge(side)];

    column_tiles(l, edge);
    l->column_to = (uint16_t)((edge->tilemap & 0x041F) | l->tilemap_base);
    l->column_parts = (uint8_t)((uint8_t)l->column_length >= 0x21 ? 2 : 1);
}

/*
 * @routine 81:C383 LayerDrawRow
 * WHAT IT DOES: Draws the row of tiles coming into view on a side of a layer
 * (0 bottom, 1 top), from the level's map: from the side's edge, across
 * the map, each metatile's tile in the edge's row half, left and right in
 * turn, into the row ring from the edge's column on (both screens), for
 * the row length. Then the video map word address of its start, and 2
 * parts.
 */
static void row_tiles(ScrollLayer *l, const ScrollEdge *edge)
{
    uint16_t map = edge->map, block = edge->block, half = edge->half, left = l->row_length;
    uint16_t t = (uint16_t)(edge->tilemap & 0x041F);
    unsigned at = (unsigned)((((t & 0x0400) ? (t | 0x0020) : t) & 0x007F) << 1);

    for (;;) {                                   /* each screen across */
        l->block = screen_at(map);
        for (;;) {                               /* each metatile across */
            uint16_t y = half;

            l->metatile = (uint16_t)(level_tile_word((uint32_t)l->block + block) | 0x8000);
            for (;;) {                           /* its two tiles */
                l->row[at / 2u] = level_metatile_word(l->metatiles, l->metatile, y);
                if (--left == 0)
                    return;
                at = (at + 2) & 0x007F;
                y ^= 0x0002;
                if (!(y & 0x0002))
                    break;
            }
            half = y;
            block = (uint16_t)(block + 2);
            if (!(block & 0x001E))
                break;
        }
        block = (uint16_t)(block - 0x0020);
        map = (uint16_t)(map + 2);
    }
}

static void layer_draw_row(ScrollLayer *l, unsigned side)
{
    ScrollEdge *edge = &l->edge[side_edge(2 + side)];

    row_tiles(l, edge);
    l->row_to = (uint16_t)((edge->tilemap & 0x0BE0 & l->tilemap_mask) | l->tilemap_base);
    l->row_parts = 2;
}

/* LayersScroll's draws of every queued column or row */
static void draw_queued(int rows)
{
    LayerEdge *queue = rows ? g.row_queue : g.column_queue;
    uint16_t *head = rows ? &g.row_queue_head : &g.column_queue_head;
    uint16_t *tail = rows ? &g.row_queue_tail : &g.column_queue_tail;

    while ((uint8_t)*head != (uint8_t)*tail) {
        LayerEdge q = queue[(*head & 0x1F) / 4u];
        ScrollLayer *l = &g.layers[q.layer & 1];

        if (rows)
            l->row_side = q.side;
        else
            l->column_side = q.side;
        *head = (uint16_t)((*head & 0xFF00) | ((*head + 4) & 0x1F));
        if (!(l->edges & side_edge_bit((rows ? 2u : 0u) + q.side)))
            continue;
        if (rows) {
            layer_draw_row(l, q.side);
            layer_step_up_down(l);
        } else {
            layer_draw_column(l, q.side);
            layer_step_across(l);
        }
    }
}

/*
 * @routine 81:C1FA LayersScroll
 * WHAT IT DOES: Once a frame, keeps both scroll layers' video maps ahead of
 * their scroll: for each layer that draws an edge or has a column to copy,
 * LayerQueueColumn and LayerQueueRow; then each queued column of a side
 * whose edge its layer draws, LayerDrawColumn and LayerStepAcross; then
 * each queued row the same, with LayerDrawRow and LayerStepUpDown.
 * HOW: LayersScrollLong (81:C1F6), its call for other banks, is this. The
 * record tests the edge mask and the column parts as one word.
 */
void LayersScroll(void)
{
    for (int layer = 0; layer < 2; layer++)
        if (g.layers[layer].edges || g.layers[layer].column_parts) {
            layer_queue(layer, 0);
            layer_queue(layer, 1);
        }
    draw_queued(0);
    draw_queued(1);
}

/* a ring of map words as the original's bytes, low byte first */
static void ring_bytes(const uint16_t *words, unsigned n, uint8_t *bytes)
{
    for (unsigned i = 0; i < n; i++) {
        bytes[2 * i] = (uint8_t)words[i];
        bytes[2 * i + 1] = (uint8_t)(words[i] >> 8);
    }
}

/*
 * @routine 80:837B LayerCopyToVram
 * WHAT IT DOES: Copies a layer's drawn column and row, if any, into its
 * video map, 32 words a part: the column down, the address stepping a row
 * at a time; the row across, its second part to the other screen ($0400
 * on). Each copied, its part count is cleared.
 * HOW: A column's second part (a column longer than 32 tiles) is the
 * record's bytes after the 32-word ring, which nothing writes; no layer's
 * set-up gives one (both columns are 32 tiles), so it is reported and left
 * out.
 */
static void layer_copy_to_vram(ScrollLayer *l)
{
    uint8_t bytes[2 * 32];

    if (l->column_parts) {
        uint8_t parts = l->column_parts;

        l->column_parts = 0;
        ring_bytes(l->column, 32, bytes);
        vram_write_stepped(l->column_to, bytes, 32, 32);
        if (parts > 1) {
            static int told;

            if (!told) {
                told = 1;
                fprintf(stderr, "video: a scroll layer's column of more than 32 tiles: its second part left out\n");
            }
        }
    }
    if (l->row_parts) {
        uint8_t parts = l->row_parts;

        l->row_parts = 0;
        ring_bytes(l->row, 32, bytes);
        vram_write(l->row_to, bytes, 32, VRAM_WORDS);
        if (parts > 1) {
            ring_bytes(l->row + 32, 32, bytes);
            vram_write((uint16_t)(l->row_to ^ 0x0400), bytes, 32, VRAM_WORDS);
        }
    }
}

/*
 * @routine 80:8366 LayersCopyToVram
 * WHAT IT DOES: Copies both scroll layers' drawn columns and rows into
 * their video maps: the play field's, then the backdrop's.
 * HOW: LayersCopyToVramLong (80:8362), its call for other banks, is this.
 */
void LayersCopyToVram(void)
{
    layer_copy_to_vram(&g.layers[0]);
    layer_copy_to_vram(&g.layers[1]);
}

/*
 * @routine 81:C060 LayersFill
 * WHAT IT DOES: As a level starts, fills both scroll layers' video maps from
 * the level's map, then sets the layers up to scroll from there:
 * LayersSetup; then for each layer, column after column from the left edge
 * (48, or 66 in levels 0, 2, 3, 6, $14 and $15): LayerDrawColumn on the
 * left side, EdgeStepRight for the left edge, LayersCopyToVram; then
 * LayersSetup again.
 * HOW: LayersFillLong (81:C05C), its call for other banks, is this.
 */
void LayersFill(void)
{
    uint8_t level = (uint8_t)g.level;
    int wide = level == 0x00 || level == 0x15 || level == 0x14 || level == 0x02 || level == 0x03 || level == 0x06;

    LayersSetup();
    g.fill_layers_left = 2;
    for (int layer = 0; layer < 2; layer++) {
        ScrollLayer *l = &g.layers[layer];

        g.fill_columns_left = (uint8_t)(wide ? 0x42 : 0x30);
        do {
            layer_draw_column(l, 1);
            edge_step_right(l, EDGE_LEFT);
            LayersCopyToVram();
        } while (--g.fill_columns_left);
        --g.fill_layers_left;
    }
    LayersSetup();
}

/*
 * level_maps.c: the level's play field and backdrop as whole-level engine
 * maps (docs/aladdin/native/VIEW.md stage V3, section 4.3).
 *
 * DESIGN NOTE. The original kept each layer in a 64 x 32-tile video map that
 * wraps (a ring) and drew the column or row of tiles about to come into view
 * as the layer scrolled (video/scroll_layer.c's ring). The ring is 512 x 256
 * pixels, so a view wider than about 464 or taller than 224 overwrites what is
 * still shown. Here each layer is one engine map the size of its layout, built
 * once as the level loads and changed for each tile change; the engine draws
 * the tiles in view, so what is shown follows the view, not a ring.
 *
 * The paths by which level tiles reached the two layers, and now:
 *   LayersFill (initial columns, 48 or 66 of them)      -> LevelMapsBuild: the whole layout.
 *   LayersScroll (queue, draw, step, 6 px trigger),
 *     LayerQueue/Draw/Step, LayersCopyToVram's column
 *     and row of the level layers                        -> removed for a level's layers: the map has
 *                                                          every tile already. (Kept for the title
 *                                                          and the Genie wheel, below.)
 *   TilemapWord addresses (the ring's video words)      -> a map position: tile x / 8, y / 8.
 *   TileChangesApply's queued quarters (vram_words)     -> LevelMapsTile: the four tiles of the
 *                                                          16 x 16 tile, in the map at once.
 *   MapTileSet (map only; level 5's closing scene)       -> LevelMapsTile too: the original showed the
 *                                                          tile when the ring next drew that column or
 *                                                          row, and the scene's block is on screen.
 *   Screen map patches before the fill (level_screen_
 *     setups.c: $0A's place $21)                         -> in the map, since it is built after them;
 *                                                          level 0's two at places $240 and $24A are
 *                                                          the backdrop's border (outside its layout):
 *                                                          the map has none, nothing shows them.
 *   Screen map patches in play (level_events.c: place
 *     $21 of $0A; kind $81's place $23)                   -> $0A's place $21: LevelMapsPlace builds that screen
 *                                                          again at once (the original redrew a patched place
 *                                                          only when the ring next drew it). Kind $81's place
 *                                                          $23 is written after LevelMapsFreeze, when the level
 *                                                          maps are off and nothing draws the layers again: its
 *                                                          high byte is the animation sets', not a screen's.
 *   Frozen layers (edges = 0): level 6's and 7's
 *     backdrop (level_backdrop.c), kind $81's arena       -> level 6: a map of the backdrop's first 512
 *                                                          pixels, drawn once as the ring was. Level 7:
 *                                                          the backdrop is a picture uploaded into
 *                                                          BACK_TALL (LevelBackdropStart clears and the
 *                                                          upload fills it), the level map is off for it.
 *                                                          Kind $81: LevelMapsFreeze.
 *   Level end ($24), story, title, options...            -> not level layers: DisplayDefaults turns the
 *                                                          level maps off and they are drawn from the
 *                                                          video maps ($4000, $4800, $5C00) as before.
 *
 * What decides that the level maps are shown: LayersFill for a level's play
 * (every level but the title's $40 and the Genie wheel's $2F, which call it
 * to fill the video maps with a map used as a picture) builds them and turns
 * them on; DisplayDefaults, which every screen change calls, turns them off.
 * A layer is shown from its level map while it is on and BG1 / BG2 still show
 * the video map the layer is (FRONT / BACK): a screen that points them at
 * another map shows that map.
 *
 * The backdrop. Its layout (layer 1: layout 1, or layout 0 where the level
 * has one only) is the map's size, except in the band levels (layer setup
 * mode $0A, $0C, $0E: levels 0, 2, 3, 6, $14, $15, $1D): there the map is the
 * layout's first 512 pixels (64 tiles) across and wraps at 512, as the ring
 * did. The ring held 512 pixels of the backdrop about its scroll, and its bands'
 * scrolls (parallax.c) reach columns far from that scroll, so a band showed
 * the ring's column at its scroll modulo 512, which is the layout's own column
 * only if the art repeats every 512 pixels; the band levels' art is meant to.
 * Level 6's drift fills the ring once (66 columns from -16: the layout's columns
 * 0-63) and is the same map. Per-line scrolls are no longer cut to 10 bits
 * where a layer is shown from a level map (video/lines.c).
 *
 * Empty screen places (negative) are blank tiles (map word 0) in the map; the
 * ring drew screen 0's there.
 *
 * Margins (video/level_fill.c). Each map is FILL_MARGIN metatiles (1,024
 * pixels) larger past each of its four edges (a wrapping band backdrop: its
 * top and bottom only), and the margin holds scenery made from the level's own
 * metatiles, so a view that shows more than the level (the frame around the
 * original's 256 x 224 screen at the level's left and top edges, or past its
 * right and bottom ones; a held view of kind $81's arena) shows art and not
 * the map's wrap to its other side. The level's (0, 0) is at (mx, my) of the map
 * (level_maps_margin gives it in pixels; video.c and lines.c add it to the
 * scroll of a layer shown from its map). The margin is in the engine map only
 * (g.level_tiles and the screen map, which collision and tile changes read, are
 * untouched, and LevelMapsTile refuses a place outside the level's own layout).
 *
 * Cost: a map is at most 1,024 x 96 tiles (level $12) or 416 x 256 ($0A),
 * written at load (one engine write per layer) and 2 x 2 tiles per tile
 * change. Maps are engine objects that persist, so one is made for each size
 * a layer has and kept for the next level of that size.
 */
#include <stdio.h>
#include <string.h>

#include "data/data.h"
#include "data/scroll_tables.h"
#include "system/state.h"
#include "video/level_fill.h"
#include "video/level_maps.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

#define MAP_WORDS (FILL_CELLS_MAX * 4)  /* the most tiles a layer's map has: the level and its margins, 2 x 2 tiles a metatile */
#define POOL_SIZE 96
#define RING_COLUMNS 64
#define RING_ROWS 32
#define BAND_COLUMNS 64             /* the band levels' backdrop: 512 pixels */
#define METATILE_TILES 16           /* a screen is 16 x 16 metatiles of 2 x 2 tiles */
#define SCREEN_PLACE_ROW 32         /* the screen map's places in a row */
#define NEGATIVE_PLACE 0x8000

typedef struct LevelMap {
    uint8_t on;
    uint8_t wraps;                  /* the band levels' backdrop: its tiles repeat every BAND_COLUMNS */
    uint32_t w, h;                  /* tiles, with the margins the fill makes past the level's edges */
    uint32_t mx, my;                /* the margins left of and above the level's own layout, in tiles: the level's (0, 0) is at (mx, my) */
    uint32_t level_w, level_h;      /* tiles of the level's own layout (a tile change is never outside it) */
    nshvc_map map;
    uint16_t *words;                /* w x h map words, row by row (kept for LevelMapsFreeze) */
} LevelMap;

static LevelMap layer_map[2];
static uint16_t field_words[MAP_WORDS], back_words[MAP_WORDS];
static uint16_t cells[FILL_CELLS_MAX];      /* a layer's metatiles, level and margins, for the fill */
static FillReport fill_report[2];
static int32_t prov_src[2][FILL_CELLS_MAX];      /* the fill's provenance by cell, per layer (level_fill_source, _kind): for the map dump */
static uint8_t prov_kind[2][FILL_CELLS_MAX];

/* the engine objects made so far, by layer and size */
static struct { uint8_t layer; uint32_t w, h; nshvc_map map; } pool[POOL_SIZE];
static unsigned pooled;

static int map_of(int layer, uint32_t w, uint32_t h, nshvc_map *m)
{
    static int told;

    for (unsigned i = 0; i < pooled; i++)
        if (pool[i].layer == layer && pool[i].w == w && pool[i].h == h) {
            *m = pool[i].map;
            return 1;
        }
    if (pooled == POOL_SIZE) {
        if (!told) {
            told = 1;
            fprintf(stderr, "video: more than %d level map sizes; a layer is left to its video map\n", POOL_SIZE);
        }
        return 0;
    }
    pool[pooled].layer = (uint8_t)layer;
    pool[pooled].w = w;
    pool[pooled].h = h;
    pool[pooled].map = nshvc_map_create(w, h);
    *m = pool[pooled++].map;
    return 1;
}

/* a tile of a metatile, the word it gives: `half` the byte offset 0, 2, 4, 6 (top left, top right, bottom left, bottom
   right) in the metatile whose level tile word is `tile` */
static uint16_t tile_word(DataRef metatiles, uint16_t tile, unsigned half)
{
    return level_metatile_word(metatiles, (uint16_t)(tile | 0x8000), half);
}

/* a screen's metatile (the original's level tiles, at the screen map place's byte offset plus the metatile's) */
static uint16_t screen_tile(uint16_t place_word, unsigned metatile_offset)
{
    uint32_t at = ((uint32_t)place_word + metatile_offset) / 2u;

    return at < LEVEL_TILES_WORDS ? g.level_tiles[at] : 0;
}

/* layer `layer`'s map from its layout: `columns` x `rows` screens of the screen map from place `first`, `columns` of
   them (all of the layout's, or the band levels' first 512 pixels) written, the layout's `layout_columns` wide */
static void build_layer(int layer, unsigned layout)
{
    LevelMap *m = &layer_map[layer];
    uint16_t *words = layer ? back_words : field_words;
    DataRef metatiles = level_metatiles(g.map_level);
    unsigned layout_columns = level_map_layout_columns(g.level, layout), rows = level_map_layout_rows(g.level, layout);
    unsigned base = (unsigned)(layer_setup_entry((unsigned)layer).map_base_high << 8) / 2u;
    int band = layer == 1 && (g.layer_setup_mode == 0x0A || g.layer_setup_mode == 0x0C || g.layer_setup_mode == 0x0E);
    unsigned columns = band && layout_columns > 2 ? 2 : layout_columns;
    unsigned lw = columns * METATILE_TILES, lh = rows * METATILE_TILES;     /* the layout in metatiles */
    unsigned ox = band ? 0 : FILL_MARGIN, oy = FILL_MARGIN;                 /* the fill's margins left and above; a wrapping backdrop has none across */
    unsigned fw = lw + 2 * ox, fh = lh + 2 * oy;                            /* the whole, with the margins on every side */
    uint32_t w, h;

    m->on = 0;
    memset(&fill_report[layer], 0, sizeof fill_report[layer]);
    if ((size_t)fw * fh > FILL_CELLS_MAX) {
        fprintf(stderr, "video: a level map of %u x %u metatiles is more than %d; its margins are not made\n", fw, fh, FILL_CELLS_MAX);
        ox = oy = 0;
        fw = lw;
        fh = lh;
    }
    w = fw * 2u;
    h = fh * 2u;
    if (w * h > MAP_WORDS) {
        fprintf(stderr, "video: a level map of %u x %u tiles is more than %d; the layer is left to its video map\n", (unsigned)w,
                (unsigned)h, MAP_WORDS);
        return;
    }
    if (!map_of(layer, w, h, &m->map))
        return;
    m->w = w;
    m->h = h;
    m->mx = ox * 2u;
    m->my = oy * 2u;
    m->level_w = lw * 2u;
    m->level_h = lh * 2u;
    m->words = words;
    m->wraps = (uint8_t)band;
    for (size_t i = 0; i < (size_t)fw * fh; i++)
        cells[i] = FILL_BLANK;              /* what the fill does not make is empty, not the last level's */
    for (unsigned cy = 0; cy < rows; cy++)
        for (unsigned cx = 0; cx < columns; cx++) {
            unsigned place = base + (cy + 1) * SCREEN_PLACE_ROW + cx + 1;
            uint16_t screen = place < SCREEN_MAP_PLACES ? g.screen_map.places[place] : (uint16_t)NEGATIVE_PLACE;

            for (unsigned my = 0; my < METATILE_TILES; my++)
                for (unsigned mx = 0; mx < METATILE_TILES; mx++)
                    cells[(size_t)(oy + cy * METATILE_TILES + my) * fw + ox + cx * METATILE_TILES + mx] =
                        screen & NEGATIVE_PLACE ? (uint16_t)FILL_BLANK : (uint16_t)(screen_tile(screen, my * 0x20u + mx * 2u) & 0x7FFFu);
        }
    LevelFill(cells, lw, lh, fw, fh, ox, oy, layer == 0, metatiles, (uint32_t)(g.level + 1u) * 0x9E3779B1u ^ (uint32_t)layer * 0x85EBCA6Bu,
              &fill_report[layer]);
    memcpy(prov_src[layer], level_fill_source(), (size_t)fw * fh * sizeof(int32_t));
    memcpy(prov_kind[layer], level_fill_kind(), (size_t)fw * fh);
    for (unsigned r = 0; r < fh; r++)
        for (unsigned c = 0; c < fw; c++) {
            uint16_t *at = &words[(size_t)(r * 2u) * w + c * 2u];
            uint16_t tile = cells[(size_t)r * fw + c];

            if (tile == FILL_BLANK) {
                at[0] = at[1] = at[w] = at[w + 1] = 0;
                continue;
            }
            at[0] = tile_word(metatiles, tile, 0);
            at[1] = tile_word(metatiles, tile, 2);
            at[w] = tile_word(metatiles, tile, 4);
            at[w + 1] = tile_word(metatiles, tile, 6);
        }
    nshvc_map_write_snes(m->map, 0, 0, w, h, words);
    m->on = 1;
}

void LevelMapsBuild(void)
{
    unsigned back = level_map_layout_count(g.level) > 1 ? 1u : 0u;
    int top, bottom;

    build_layer(0, 0);
    build_layer(1, back);
    level_maps_seamless(&top, &bottom);
    view_fit_level(top, bottom);        /* the frame shows only what the fill goes on with */
}

void LevelMapsOff(void)
{
    layer_map[0].on = layer_map[1].on = 0;
}

void LevelMapsBackdropOff(void)
{
    layer_map[1].on = 0;
}

int level_maps_active(int layer)
{
    return layer_map[layer].on && g.bg_map[layer] == (layer ? VIDEO_MAP_BACK : VIDEO_MAP_FRONT);
}

nshvc_map level_maps_object(int layer)
{
    return layer_map[layer].map;
}

void LevelMapsTile(uint16_t x, uint16_t y, uint16_t tile)
{
    LevelMap *m = &layer_map[0];
    DataRef metatiles = level_metatiles(g.map_level);
    uint32_t tx = (uint32_t)(x >> 4) * 2u, ty = (uint32_t)(y >> 4) * 2u;
    uint16_t quad[4];

    if (!m->on || tx >= m->level_w || ty >= m->level_h)
        return;
    tx += m->mx;
    ty += m->my;
    for (unsigned i = 0; i < 4; i++)
        quad[i] = tile_word(metatiles, tile, i * 2u);
    m->words[ty * m->w + tx] = quad[0];
    m->words[ty * m->w + tx + 1] = quad[1];
    m->words[(ty + 1) * m->w + tx] = quad[2];
    m->words[(ty + 1) * m->w + tx + 1] = quad[3];
    nshvc_map_write_snes(m->map, tx, ty, 2, 2, quad);
}

/* the map word of layer `layer` at tile column c, row r of the level (any integers; the margins are the negative ones and
   those past the layout): a wrapping backdrop repeats across, tiles past the margins are blank */
static uint16_t word_at(const LevelMap *m, int32_t c, int32_t r)
{
    c += (int32_t)m->mx;
    r += (int32_t)m->my;
    if (m->wraps)
        c &= (int32_t)(m->w - 1);
    if (c < 0 || r < 0 || (uint32_t)c >= m->w || (uint32_t)r >= m->h)
        return 0;
    return m->words[(uint32_t)r * m->w + (uint32_t)c];
}

void LevelMapsFreeze(void)
{
    for (int layer = 0; layer < 2; layer++) {
        const LevelMap *m = &layer_map[layer];
        uint16_t ring[RING_COLUMNS * RING_ROWS];
        int32_t c0 = ((int32_t)(g.layers[layer].scroll_x >> 8 & 0xFFFF) + edge_start_x(0)) >> 3;
        int32_t r0 = ((int32_t)(g.layers[layer].scroll_y >> 8 & 0xFFFF) + edge_start_y(2)) >> 3;

        if (!m->on)
            continue;
        /* the ring's slot for a tile is its column mod 64 and row mod 32, holding the tile nearest the scroll: the
           columns from the left edge on and the rows from the top edge on */
        for (int32_t dr = 0; dr < RING_ROWS; dr++)
            for (int32_t dc = 0; dc < RING_COLUMNS; dc++)
                ring[((c0 + dc) & (RING_COLUMNS - 1)) + ((r0 + dr) & (RING_ROWS - 1)) * RING_COLUMNS] =
                    word_at(m, c0 + dc, r0 + dr);
        video_map_write(layer ? VIDEO_MAP_BACK : VIDEO_MAP_FRONT, ring);
    }
    LevelMapsOff();
}

void level_maps_margin(int layer, int32_t *x, int32_t *y)
{
    *x = (int32_t)layer_map[layer].mx * 8;
    *y = (int32_t)layer_map[layer].my * 8;
}

const uint16_t *level_maps_words(int layer, uint32_t *w, uint32_t *h, uint32_t *level_w, uint32_t *level_h, uint32_t *margin_x,
                                 uint32_t *margin_y)
{
    const LevelMap *m = &layer_map[layer];

    *margin_x = m->mx;
    *margin_y = m->my;
    *w = m->w;
    *h = m->h;
    *level_w = m->level_w;
    *level_h = m->level_h;
    return m->on ? m->words : NULL;
}

void level_maps_provenance(int layer, const int32_t **src, const uint8_t **kind)
{
    *src = prov_src[layer];
    *kind = prov_kind[layer];
}

const FillReport *level_maps_fill(int layer)
{
    return &fill_report[layer];
}

void level_maps_seamless(int *top, int *bottom)
{
    *top = fill_report[0].seam_up && fill_report[1].seam_up;
    *bottom = fill_report[0].seam_down && fill_report[1].seam_down;
}

/*
 * LevelMapsPlace: a screen map place patched in play (a level's own events: level $0A's place $21 to $FE): the
 * screen at that place of the play field's layout is built again from the place's word, in the map and the engine
 * object, as LevelMapsTile does for one tile. A place that is not a screen of the layout is left alone. The original
 * redrew a patched place only when its ring next drew it; the map shows it at once, which a view wider than the
 * original's can see.
 */
void LevelMapsPlace(unsigned place)
{
    LevelMap *m = &layer_map[0];
    DataRef metatiles = level_metatiles(g.map_level);
    unsigned base = (unsigned)(layer_setup_entry(0).map_base_high << 8) / 2u;
    unsigned rel = place - base, cy, cx;
    uint16_t screen = g.screen_map.places[place % SCREEN_MAP_PLACES];

    if (!m->on || place < base || rel % SCREEN_PLACE_ROW == 0 || rel < SCREEN_PLACE_ROW)
        return;
    cy = rel / SCREEN_PLACE_ROW - 1;
    cx = rel % SCREEN_PLACE_ROW - 1;
    if ((cx + 1) * METATILE_TILES * 2u > m->level_w || (cy + 1) * METATILE_TILES * 2u > m->level_h)
        return;
    for (unsigned my = 0; my < METATILE_TILES; my++)
        for (unsigned mx = 0; mx < METATILE_TILES; mx++) {
            uint16_t *at = &m->words[(m->my + (cy * METATILE_TILES + my) * 2u) * m->w + m->mx + (cx * METATILE_TILES + mx) * 2u];
            uint16_t tile;

            if (screen & NEGATIVE_PLACE) {
                at[0] = at[1] = at[m->w] = at[m->w + 1] = 0;
                continue;
            }
            tile = screen_tile(screen, my * 0x20u + mx * 2u);
            at[0] = tile_word(metatiles, tile, 0);
            at[1] = tile_word(metatiles, tile, 2);
            at[m->w] = tile_word(metatiles, tile, 4);
            at[m->w + 1] = tile_word(metatiles, tile, 6);
        }
    for (unsigned row = 0; row < METATILE_TILES * 2u; row++)
        nshvc_map_write_snes(m->map, m->mx + cx * METATILE_TILES * 2u, m->my + cy * METATILE_TILES * 2u + row, METATILE_TILES * 2u, 1,
                             &m->words[(m->my + cy * METATILE_TILES * 2u + row) * m->w + m->mx + cx * METATILE_TILES * 2u]);
}

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
 * The view follows the window in play (VIEW.md 4.8). A map holds MAP_PAD_CELLS (32) metatiles of margin each side across whatever the view's
 * depth, the fill made as deep as the view shows (view.dx) and the rest blank, so a map's size is the level's and not the window's: one engine
 * object per level and layer (objects are never freed) serves every width, and a change of the view makes the fill again (LevelMapsResize) into
 * the same object. The layout's cells as the level loaded them are kept (level_cells) for it, and the layout's words as they are now (tile
 * changes made) stay as they are.
 *
 * Cost: a map is at most 1,024 x 96 tiles (level $12) or 416 x 256 ($0A),
 * written at load (one engine write per layer) and 2 x 2 tiles per tile
 * change. Maps are engine objects that persist, so one is made for each size
 * a layer has and kept for the next level of that size.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "data/data.h"
#include "data/scroll_tables.h"
#include "system/state.h"
#include "video/between.h"
#include "video/level_fill.h"
#include "video/level_maps.h"
#include "system/runahead.h"
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
#define MAP_PAD_CELLS 32            /* the map holds this many cells of margin each side across, whatever the view's depth (the fill is made as deep as the view
                                       shows and the rest is blank), so the map's size does not change with the window and one engine object serves every width */

typedef struct LevelMap {
    uint8_t on;
    uint8_t wraps;                  /* the band levels' backdrop: its tiles repeat every BAND_COLUMNS */
    uint32_t w, h;                  /* tiles, with the margins the fill makes past the level's edges */
    uint32_t mx, my;                /* the margins left of and above the level's own layout, in tiles: the level's (0, 0) is at (mx, my) */
    uint32_t level_w, level_h;      /* tiles of the level's own layout (a tile change is never outside it) */
    nshvc_map map;
    uint16_t *words;                /* w x h map words, row by row (kept for LevelMapsFreeze) */
    /* what a change of the view needs to make the margins again (VIEW.md 4.8) */
    uint8_t made;                   /* built from the layout below (a layer shown from a video map is not) */
    uint32_t lcols, lrows;          /* the layout, in metatiles */
    uint32_t reach;                 /* the columns of it the original's window can reach (layout_reach_x), in metatiles */
    uint32_t pad;                   /* the cells of margin the map holds each side across (its mx / 2): at least MAP_PAD_CELLS */
    uint32_t fill_ox;               /* the cells of margin the fill made each side across: the view's depth when it was made */
} LevelMap;

static LevelMap layer_map[2];
static uint16_t level_cells[2][FILL_CELLS_MAX];     /* each layer's layout as the level loaded it (metatiles), for the margins made again */
static uint16_t field_words[MAP_WORDS], back_words[MAP_WORDS];
static uint16_t cells[FILL_CELLS_MAX];      /* a layer's metatiles, level and margins, for the fill */
static FillReport fill_report[2];
static double fill_clock;

static double fill_now(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}
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

/* a developer's note (ALADDIN_DEV_OVER): per level and layer, the layout's width and the width the window can reach (layout_reach_x), and the window's
   own least x and greatest right edge a play went to (level_maps_note); written at the end of the run with the overshoots below */
static struct { int32_t width, reach, least, most, art; } window_seen[2][64];
static const char *over_file;
static void over_init(void);

static void level_maps_reach_note(int layer, int32_t width, int32_t reach, int32_t art)
{
    over_init();
    window_seen[layer][g.level & 63].width = width;
    window_seen[layer][g.level & 63].reach = reach;
    window_seen[layer][g.level & 63].art = art;
    window_seen[layer][g.level & 63].least = 1 << 30;
    window_seen[layer][g.level & 63].most = -(1 << 30);
}

/* The columns of a layer's layout the original's 256 x 224 window can ever reach (docs VIEW.md 5.y), in pixels from the layout's left: the layout's
   width, or less. The camera's x is at most the play field's layout width less 256 (scroll_limit_x as the load leaves it; a scene's lock only lowers
   it, an arena raises it by one screen to that width). The play field's window is the camera's. The backdrop's scroll x is by the level's handler
   (parallax.c, g.layer_setup_mode) from the camera's: half (modes 0, 6, 8), the same (2, 4, $14, and $10 until the scroll passes $1000, then half
   as fast), so its window ends at the scroll at the camera's greatest x, plus 256. The banded handlers ($0A, $0C, $0E) wrap their layer round,
   and $12 does not scroll; those, and a layout the handler's scroll does not account for, are all reachable (the whole layout). The scroll is at
   least 0 (the camera's x is), so nothing on the left is unreachable. A column cut at a multiple of 16 pixels (a metatile). `field_px` is the
   play field's layout width. */
static int32_t layout_reach_x(int layer, int32_t layout_px, int32_t field_px)
{
    int32_t cam = field_px - 256, scroll;

    if (layer == 0 || cam <= 0)
        return layout_px;
    if (g.bg2_follows == 0x80)
        return layout_px;
    switch (g.layer_setup_mode) {
    case 0x00: case 0x06: case 0x08: scroll = cam / 2; break;
    case 0x02: case 0x04: case 0x14: scroll = cam; break;
    case 0x10: scroll = cam >= 0x1000 ? (cam - 0x1000) / 2 + 0x1000 : cam; break;
    default: return layout_px;
    }
    scroll = (scroll + 256 + 15) / 16 * 16;
    return scroll < layout_px ? scroll : layout_px;
}

/* The margins the view asks of a layer, in metatiles (docs VIEW.md 5.x): across, the frame round the original's screen, `dx` pixels, and a cell more
   each side; the backdrop's also the 128 pixels past its layout that the original's own window reaches in levels $0F and $10 (the plays' counts,
   level_maps_note). Down and up, none for the play field (its window never leaves the layout: the camera's limits) and 7 cells for the backdrop (the
   original's window goes 96 pixels above level $10's). A wrapping backdrop has none across. Where the layer and its margins would be more than
   FILL_CELLS_MAX cells, none (`*fell` set). */
static void margins_for(int layer, int band, unsigned lw, unsigned lh, int32_t dx, unsigned *ox, unsigned *oy, int *fell)
{
    unsigned dx_cells = (unsigned)((dx + 15) / 16) + 1u + (layer == 1 ? 8u : 0u);

    *ox = band ? 0 : dx_cells;
    *oy = layer == 1 ? 7u : 0u;
    *fell = (size_t)(lw + 2 * *ox) * (lh + 2 * *oy) > FILL_CELLS_MAX;
    if (*fell)
        *ox = *oy = 0;
}

/* the cells of margin the map holds each side across for a fill `ox` deep: at least MAP_PAD_CELLS (so the map's size is the same at every width a
   window can have; one engine object per level and layer), or the fill's depth where that is more, and no more than fit in MAP_WORDS */
static unsigned pad_for(unsigned ox, unsigned lw, unsigned fh)
{
    unsigned pad = ox == 0 ? 0 : ox > MAP_PAD_CELLS ? ox : MAP_PAD_CELLS;

    if ((size_t)(lw + 2 * pad) * 2 * fh * 2 > MAP_WORDS)
        pad = ox;
    return pad;
}

/* layer `layer`'s map made from the layout kept in the layer (level_cells, lcols, lrows, reach, wraps) at the view's depth now: the fill, the words, the
   engine map. A first build (resize 0) makes it whole; a resize (the view changed in play) makes the margins again and keeps the layout's words as they
   are now (tile changes already made stay), and does nothing where the margins would be the ones it holds. Returns 1 if it made the map. */
static int layer_make(int layer, int resize)
{
    LevelMap *m = &layer_map[layer];
    uint16_t *words = layer ? back_words : field_words;
    DataRef metatiles = level_metatiles(g.map_level);
    unsigned lw = m->lcols, lh = m->lrows, reach = m->reach, ox, oy, pad, fw, fh;
    uint32_t w, h;
    int fell;
    nshvc_map map;
    uint16_t *kept = NULL;
    size_t kept_w = (size_t)reach * 2u;

    margins_for(layer, m->wraps, lw, lh, view.dx, &ox, &oy, &fell);
    fw = lw + 2 * ox;
    fh = lh + 2 * oy;
    pad = pad_for(ox, lw, fh);
    if (resize && ox == m->fill_ox && pad == m->pad)
        return 0;
    if (fell && !resize)
        fprintf(stderr, "video: a level map of %u x %u metatiles with its margins is more than %d; its margins are not made\n", lw, lh, FILL_CELLS_MAX);
    w = (lw + 2 * pad) * 2u;
    h = fh * 2u;
    if (w * h > MAP_WORDS) {
        fprintf(stderr, "video: a level map of %u x %u tiles is more than %d; the layer is left to its video map\n", (unsigned)w, (unsigned)h,
                MAP_WORDS);
        if (!resize)
            m->on = 0;
        return 0;
    }
    if (!map_of(layer, w, h, &map)) {
        if (!resize)
            m->on = 0;
        return 0;
    }
    if (resize) {
        kept = malloc(kept_w * lh * 2u * sizeof *kept);
        if (!kept) {
            fprintf(stderr, "video: no memory to keep a layer's words\n");
            exit(1);
        }
        for (unsigned r = 0; r < lh * 2u; r++)
            memcpy(&kept[r * kept_w], &m->words[(size_t)(m->my + r) * m->w + m->mx], kept_w * sizeof *kept);
    }
    for (size_t i = 0; i < (size_t)fw * fh; i++)
        cells[i] = FILL_BLANK;              /* what the fill does not make is empty, not the last level's */
    for (unsigned cy = 0; cy < lh; cy++)
        for (unsigned cx = 0; cx < reach; cx++)       /* art the original's window never reaches (layout_reach_x) is margin, made by the fill */
            cells[(size_t)(oy + cy) * fw + ox + cx] = level_cells[layer][(size_t)cy * lw + cx];
    memset(&fill_report[layer], 0, sizeof fill_report[layer]);
    fill_clock = fill_now();
    LevelFill(cells, reach, lh, fw, fh, ox, oy, layer == 0, view.dx > 0, metatiles, (uint32_t)(g.level + 1u) * 0x9E3779B1u ^ (uint32_t)layer * 0x85EBCA6Bu,
              &fill_report[layer]);
    if (getenv("ALADDIN_DEV_FILL"))
        fprintf(stderr, "fillms layer %d level %u x %u cells, whole %u x %u: %.1f ms%s\n", layer, lw, lh, fw, fh, (fill_now() - fill_clock) * 1000.0,
                resize ? " (the view changed)" : "");
    memcpy(prov_src[layer], level_fill_source(), (size_t)fw * fh * sizeof(int32_t));
    memcpy(prov_kind[layer], level_fill_kind(), (size_t)fw * fh);
    memset(words, 0, (size_t)w * h * sizeof *words);       /* the padding is blank */
    for (unsigned r = 0; r < fh; r++)
        for (unsigned c = 0; c < fw; c++) {
            uint16_t *at = &words[(size_t)(r * 2u) * w + (pad - ox + c) * 2u];
            uint16_t tile = cells[(size_t)r * fw + c];

            if (tile == FILL_BLANK)
                continue;
            at[0] = tile_word(metatiles, tile, 0);
            at[1] = tile_word(metatiles, tile, 2);
            at[w] = tile_word(metatiles, tile, 4);
            at[w + 1] = tile_word(metatiles, tile, 6);
        }
    if (resize) {
        for (unsigned r = 0; r < lh * 2u; r++)
            memcpy(&words[(size_t)(oy * 2u + r) * w + pad * 2u], &kept[r * kept_w], kept_w * sizeof *kept);
        free(kept);
    }
    between_map_write_snes(map, 0, 0, w, h, words);
    m->map = map;
    m->w = w;
    m->h = h;
    m->mx = pad * 2u;
    m->my = oy * 2u;
    m->level_w = lw * 2u;
    m->level_h = lh * 2u;
    m->words = words;
    m->pad = pad;
    m->fill_ox = ox;
    m->on = 1;
    return 1;
}

/* layer `layer`'s map from its layout: `columns` x `rows` screens of the screen map from place `first`, `columns` of
   them (all of the layout's, or the band levels' first 512 pixels) written, the layout's `layout_columns` wide */
static int32_t field_px;                     /* the play field layout's width in pixels (the last built) */

/* layer `layer`'s layout from the screen map and the level's tiles as they are now: its cells, size, wrapping and the columns the original's window
   reaches, kept in the layer for the margins to be made again (a level's load, and a snapshot's load, which rebuilds them from the game state it brings:
   a screen map place patched in play (level $0A's) is then in them as it is). Returns 0 where the layer is left to its video map. */
static int layer_layout(int layer, unsigned layout, int note)
{
    LevelMap *m = &layer_map[layer];
    unsigned layout_columns = level_map_layout_columns(g.level, layout), rows = level_map_layout_rows(g.level, layout);
    unsigned base = (unsigned)(layer_setup_entry((unsigned)layer).map_base_high << 8) / 2u;
    int band = layer == 1 && (g.layer_setup_mode == 0x0A || g.layer_setup_mode == 0x0C || g.layer_setup_mode == 0x0E);
    unsigned columns = band && layout_columns > 2 ? 2 : layout_columns;
    unsigned lw = columns * METATILE_TILES, lh = rows * METATILE_TILES;     /* the layout in metatiles */
    unsigned reach;

    if ((size_t)lw * lh > FILL_CELLS_MAX) {
        fprintf(stderr, "video: a level map of %u x %u metatiles is more than %d; the layer is left to its video map\n", lw, lh, FILL_CELLS_MAX);
        return 0;
    }
    for (unsigned cy = 0; cy < rows; cy++)
        for (unsigned cx = 0; cx < columns; cx++) {
            unsigned place = base + (cy + 1) * SCREEN_PLACE_ROW + cx + 1;
            uint16_t screen = place < SCREEN_MAP_PLACES ? g.screen_map.places[place] : (uint16_t)NEGATIVE_PLACE;

            for (unsigned my = 0; my < METATILE_TILES; my++)
                for (unsigned mx = 0; mx < METATILE_TILES; mx++)
                    level_cells[layer][(size_t)(cy * METATILE_TILES + my) * lw + cx * METATILE_TILES + mx] =
                        screen & NEGATIVE_PLACE ? (uint16_t)FILL_BLANK : (uint16_t)(screen_tile(screen, my * 0x20u + mx * 2u) & 0x7FFFu);
        }
    if (layer == 0)
        field_px = (int32_t)lw * 16;
    reach = (unsigned)(layout_reach_x(layer, (int32_t)lw * 16, field_px) / 16);
    if (note && getenv("ALADDIN_DEV_OVER")) {
        int32_t art = 0;

        for (unsigned cy = 0; cy < lh; cy++)
            for (unsigned cx = reach; cx < lw; cx++)
                art += level_cells[layer][(size_t)cy * lw + cx] != FILL_BLANK;
        level_maps_reach_note(layer, (int32_t)lw * 16, (int32_t)reach * 16, art);
    }
    m->lcols = lw;
    m->lrows = lh;
    m->reach = reach;
    m->wraps = (uint8_t)band;
    return 1;
}

static void build_layer(int layer, unsigned layout)
{
    LevelMap *m = &layer_map[layer];

    m->on = 0;
    m->made = 0;
    if (!layer_layout(layer, layout, 1))
        return;
    layer_make(layer, 0);
    m->made = m->on;
}

void LevelMapsBuild(void)
{
    unsigned back = level_map_layout_count(g.level) > 1 ? 1u : 0u;

    if (runahead_guard())
        return;

    build_layer(0, 0);
    build_layer(1, back);
}

/* the view's depth changed (VIEW.md 4.8): whether a layer shown from its level map would have other margins at a view of `dx` */
int LevelMapsNeedResize(int32_t dx)
{
    for (int layer = 0; layer < 2; layer++) {
        const LevelMap *m = &layer_map[layer];
        unsigned ox, oy;
        int fell;

        if (!m->on || !m->made)
            continue;
        margins_for(layer, m->wraps, m->lcols, m->lrows, dx, &ox, &oy, &fell);
        if (ox != m->fill_ox || pad_for(ox, m->lcols, m->lrows + 2 * oy) != m->pad)
            return 1;
    }
    return 0;
}

/* the margins made again for the view now, in each layer that shows its level map and whose depth changed; returns how many were made */
int LevelMapsResize(void)
{
    int n = 0;

    for (int layer = 0; layer < 2; layer++)
        if (layer_map[layer].on && layer_map[layer].made)
            n += layer_make(layer, 1);
    return n;
}

/* the layers' records set anew in "was" and "now" (the view followed the window: the picture of that update draws the new margins) */
void LevelMapsReset(void)
{
    between_reset_part(layer_map);
}

void LevelMapsOff(void)
{
    layer_map[0].on = layer_map[1].on = 0;
}

void LevelMapsBackdropOff(void)
{
    layer_map[1].on = 0;
}

/* the layers' maps as the picture reads them (which map, its margins, on), kept as the update before's world
   (between.h); a map's words are not: the picture reads the engine's map, whose writes are held */
void level_maps_keep(void)
{
    between_keep(layer_map, sizeof layer_map);
    between_owned(layer_map);
}

/* ---- the snapshot (system/snapshot.c): each layer's record as numbers, then its words ---- */

#define SNAP_HEAD 10u                   /* 32-bit numbers a layer: on, wraps, w, h, mx, my, level_w, level_h, which words array, 0 */

size_t level_maps_snapshot_max(void)
{
    return 2 * (SNAP_HEAD * 4 + (size_t)MAP_WORDS * 2);
}

static uint32_t put32(uint8_t *o, uint32_t v)
{
    o[0] = (uint8_t)v; o[1] = (uint8_t)(v >> 8); o[2] = (uint8_t)(v >> 16); o[3] = (uint8_t)(v >> 24);
    return 4;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

size_t level_maps_snapshot_save(uint8_t *out)
{
    size_t at = 0;

    for (int layer = 0; layer < 2; layer++) {
        const LevelMap *m = &layer_map[layer];
        uint32_t head[SNAP_HEAD] = { m->on, m->wraps, m->w, m->h, m->mx, m->my, m->level_w, m->level_h, m->words == back_words, 0 };

        for (unsigned i = 0; i < SNAP_HEAD; i++)
            at += put32(out + at, head[i]);
        if (m->on) {
            for (size_t i = 0; i < (size_t)m->w * m->h; i++) {
                out[at++] = (uint8_t)m->words[i];
                out[at++] = (uint8_t)(m->words[i] >> 8);
            }
        }
    }
    return at;
}

/* whether `in` is two layers' records that fit (sizes within what a layer holds, the words all there) */
int level_maps_snapshot_check(const uint8_t *in, size_t len)
{
    size_t at = 0;

    for (int layer = 0; layer < 2; layer++) {
        uint32_t on, w, h;

        if (at + SNAP_HEAD * 4 > len)
            return 0;
        on = get32(in + at);
        w = get32(in + at + 8);
        h = get32(in + at + 12);
        at += SNAP_HEAD * 4;
        if (on > 1 || get32(in + at - 8) > 1)
            return 0;
        if (on) {
            if (!w || !h || (uint64_t)w * h > MAP_WORDS || at + (size_t)w * h * 2 > len)
                return 0;
            at += (size_t)w * h * 2;
        }
    }
    return at == len;
}

/* put in: the engine map of that layer and size from the pool as a level's load takes it (made if the process has none), the
   words in the game's array and written to the engine map; a layer that was off is off */
int level_maps_snapshot_load(const uint8_t *in, size_t len)
{
    size_t at = 0;

    (void)len;
    for (int layer = 0; layer < 2; layer++) {
        LevelMap *m = &layer_map[layer];
        uint32_t head[SNAP_HEAD];
        nshvc_map map = m->map;

        for (unsigned i = 0; i < SNAP_HEAD; i++)
            head[i] = get32(in + at + 4 * i);
        at += SNAP_HEAD * 4;
        if (head[0] && !map_of(layer, head[2], head[3], &map))
            return 0;
        m->on = (uint8_t)head[0];
        m->wraps = (uint8_t)head[1];
        m->w = head[2];
        m->h = head[3];
        m->mx = head[4];
        m->my = head[5];
        m->level_w = head[6];
        m->level_h = head[7];
        m->words = head[8] ? back_words : field_words;
        m->map = map;
        if (m->on) {
            for (size_t i = 0; i < (size_t)m->w * m->h; i++)
                m->words[i] = (uint16_t)(in[at + 2 * i] | in[at + 2 * i + 1] << 8);
            nshvc_map_write_snes(m->map, 0, 0, m->w, m->h, m->words);
            at += (size_t)m->w * m->h * 2;
        }
        /* what the margins are made again from (VIEW.md 4.8): the layout's cells from the game state just put in; the margin's depth is the
           adopted view's (the view and the margins are only ever changed together), the map's padding what the record says */
        m->made = 0;
        if (m->on && layer_layout(layer, layer ? (level_map_layout_count(g.level) > 1 ? 1u : 0u) : 0u, 0)) {
            unsigned ox, oy;
            int fell;

            margins_for(layer, m->wraps, m->lcols, m->lrows, view.dx, &ox, &oy, &fell);
            m->pad = m->mx / 2u;
            m->fill_ox = ox;
            m->made = 1;
        }
    }
    return 1;
}

/* the developer's state hash (ALADDIN_DEV_STATE_HASH): the layers' maps and their words as they are now */
uint64_t level_maps_hash(uint64_t h)
{
    for (int layer = 0; layer < 2; layer++) {
        const LevelMap *m = &layer_map[layer];
        uint32_t head[9] = { m->on, m->wraps, m->w, m->h, m->mx, m->my, m->level_w, m->level_h, m->words == back_words };   /* not the engine's handle: it is the process's */
        const uint8_t *p = (const uint8_t *)head;
        size_t n = sizeof head;

        for (int pass = 0; pass < 2; pass++) {
            for (size_t i = 0; i < n; i++)
                h = (h ^ p[i]) * 0x100000001b3ull;
            if (pass == 0) {
                if (!m->on || !m->words)
                    break;
                p = (const uint8_t *)m->words;
                n = (size_t)m->w * m->h * sizeof *m->words;
            }
        }
    }
    return h;
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
    runahead_log(&m->words[ty * m->w + tx], 2 * sizeof *m->words);          /* put back with the real world (runahead.h) */
    runahead_log(&m->words[(ty + 1) * m->w + tx], 2 * sizeof *m->words);
    for (unsigned i = 0; i < 4; i++)
        quad[i] = tile_word(metatiles, tile, i * 2u);
    m->words[ty * m->w + tx] = quad[0];
    m->words[ty * m->w + tx + 1] = quad[1];
    m->words[(ty + 1) * m->w + tx] = quad[2];
    m->words[(ty + 1) * m->w + tx + 1] = quad[3];
    between_map_write_snes(m->map, tx, ty, 2, 2, quad);
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

/* a developer's note (ALADDIN_DEV_OVER=FILE): how far past the level's own layout each layer's window (scroll x, y, 256 x 224) goes, per level, at the
   end of the run: left, right, top, bottom in pixels (0: never) */
static struct { int32_t v[4]; } over_seen[2][64];

static void over_report(void)
{
    FILE *f = over_file ? fopen(over_file, "a") : NULL;

    if (!f)
        return;
    for (int l = 0; l < 2; l++)
        for (int n = 0; n < 64; n++)
            if (window_seen[l][n].width)
                fprintf(f, "window level %02X layer %d x %d..%d reach %d of %d art_cells_cut %d\n", n, l,
                        window_seen[l][n].most > -(1 << 30) ? window_seen[l][n].least : -1, window_seen[l][n].most > -(1 << 30) ? window_seen[l][n].most : -1,
                        window_seen[l][n].reach, window_seen[l][n].width, window_seen[l][n].art);
    for (int l = 0; l < 2; l++)
        for (int n = 0; n < 64; n++)
            if (over_seen[l][n].v[0] || over_seen[l][n].v[1] || over_seen[l][n].v[2] || over_seen[l][n].v[3])
                fprintf(f, "level %02X layer %d left %d right %d top %d bottom %d\n", n, l, over_seen[l][n].v[0], over_seen[l][n].v[1],
                        over_seen[l][n].v[2], over_seen[l][n].v[3]);
    fclose(f);
}

static void over_init(void)
{
    static int known;

    if (!known) {
        known = 1;
        over_file = getenv("ALADDIN_DEV_OVER");
        if (over_file)
            atexit(over_report);
    }
}

void level_maps_note(int layer, int32_t scroll_x, int32_t scroll_y)
{
    const LevelMap *m = &layer_map[layer];
    int32_t v[4];

    over_init();
    if (!over_file || !m->on || m->wraps)
        return;
    if (scroll_x < window_seen[layer][g.level & 63].least)
        window_seen[layer][g.level & 63].least = scroll_x;
    if (scroll_x + 256 > window_seen[layer][g.level & 63].most)
        window_seen[layer][g.level & 63].most = scroll_x + 256;
    v[0] = -scroll_x;
    v[1] = scroll_x + 256 - (int32_t)m->level_w * 8;
    v[2] = -scroll_y;
    v[3] = scroll_y + 224 - (int32_t)m->level_h * 8;
    for (int i = 0; i < 4; i++)
        if (v[i] > over_seen[layer][g.level & 63].v[i])
            over_seen[layer][g.level & 63].v[i] = v[i];
}

int32_t level_maps_height(int layer)
{
    return layer_map[layer].on ? (int32_t)layer_map[layer].h * 8 : 0;
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
    static uint16_t *crop[2];
    uint32_t cw, off;

    if (!m->on)
        return NULL;
    /* the map as the fill made it: the padding the map holds past the fill's depth (VIEW.md 4.8) is left out */
    cw = (m->level_w + 2u * m->fill_ox * 2u);
    off = m->mx - m->fill_ox * 2u;
    free(crop[layer]);
    crop[layer] = malloc((size_t)cw * m->h * sizeof **crop);
    if (!crop[layer])
        return NULL;
    for (uint32_t r = 0; r < m->h; r++)
        memcpy(&crop[layer][(size_t)r * cw], &m->words[(size_t)r * m->w + off], cw * sizeof **crop);
    *margin_x = m->fill_ox * 2u;
    *margin_y = m->my;
    *w = cw;
    *h = m->h;
    *level_w = m->level_w;
    *level_h = m->level_h;
    return crop[layer];
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
    for (unsigned row = 0; row < METATILE_TILES * 2u; row++)    /* the screen's rows, put back with the real world (runahead.h) */
        runahead_log(&m->words[(m->my + cy * METATILE_TILES * 2u + row) * m->w + m->mx + cx * METATILE_TILES * 2u],
                     METATILE_TILES * 2u * sizeof *m->words);
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
        between_map_write_snes(m->map, m->mx + cx * METATILE_TILES * 2u, m->my + cy * METATILE_TILES * 2u + row, METATILE_TILES * 2u, 1,
                             &m->words[(m->my + cy * METATILE_TILES * 2u + row) * m->w + m->mx + cx * METATILE_TILES * 2u]);
}

/*
 * video.c: the engine objects the picture is made of, the one function that
 * turns a video address into them, the uploads the original's NMI made, and
 * the picture (video.h says when the loop calls each).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "system/system.h"
#include "video/level_maps.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

/* the objects (video.h lists the video addresses each shows) */
static nshvc_tiles tiles[VIDEO_TILE_KINDS];
static nshvc_map maps[VIDEO_MAP_KINDS];
static nshvc_tiles mode7_tiles;
static nshvc_map mode7_map;

/* Mode 7's map and pixels as the game's own arrays: 128 x 128 tiles, and
   256 tiles of 64 pixels, a byte each; sent when they change. */
static uint8_t mode7_cells[128 * 128];
static uint8_t mode7_pixels[256 * 64];
static uint8_t mode7_cells_changed, mode7_pixels_changed;
static uint8_t mode7_up;            /* Mode7ScreenSetup ran since DisplayDefaults: writes reach Mode 7 */
static uint8_t mode7_outside;       /* outside the plane transparent (Mode7ScreenSetup); else it repeats */

/* the sprites the picture shows: the game's list as the last frame that
   sent it left it (the original's sprite table keeps its last upload) */
static nshvc_sprite *shown_sprites;
static uint32_t shown_count;
static nshvc_sprite *moved_sprites;  /* the shown sprites moved by the crop or a still screen's origin (video_draw) */

/* the lists of sprites that are sized for the view: the game's (OamInit) and the picture's two */
static void sprite_lists_make(void)
{
    OamInit();
    free(shown_sprites);
    free(moved_sprites);
    shown_sprites = calloc(OamSpriteCapacity(), sizeof *shown_sprites);
    moved_sprites = calloc(OamSpriteCapacity(), sizeof *moved_sprites);
    shown_count = 0;
    if (!shown_sprites || !moved_sprites) {
        fprintf(stderr, "video: no memory for the shown sprites\n");
        exit(1);
    }
}

/*
 * The view changes only here, where a level loads and where a still screen starts (view.h). Everything that keeps
 * something sized from the view is redone: the sprite lists (the budget follows the view's area). Everything else
 * the game derives from the view reads it where it is used, or is set after this by the level's own set-up (the
 * camera's limits and aim, the range point, the spawn window, the line tables, the window edges); a level's load
 * runs this first, before LevelScrollLimits.
 */
static int level_loading;       /* video_view_level ran and LevelScreenSetup has not yet: a DisplayDefaults in between keeps the level's view */

static void view_changed(int changed)
{
    if (changed && shown_sprites)
        sprite_lists_make();
}

void video_view_level(void)
{
    level_loading = 1;
    view_changed(view_choose());
}

void video_view_still(void)
{
    if (!level_loading)
        view_changed(view_choose_still());
}

void video_view_level_done(void)
{
    level_loading = 0;
}

void video_init(void)
{
    sprite_lists_make();
    tiles[VIDEO_TILES_LOW] = nshvc_tiles_create(4, 1024, 1);
    tiles[VIDEO_TILES_HIGH] = nshvc_tiles_create(4, 1024, 1);
    tiles[VIDEO_TILES_TEXT] = nshvc_tiles_create(2, 1024, 1);
    maps[VIDEO_MAP_BACK] = nshvc_map_create(64, 32);
    maps[VIDEO_MAP_FRONT] = nshvc_map_create(64, 32);
    maps[VIDEO_MAP_TEXT] = nshvc_map_create(32, 32);
    maps[VIDEO_MAP_BACK_TALL] = nshvc_map_create(64, 64);
    mode7_tiles = nshvc_tiles_create(8, 256, 1);
    mode7_map = nshvc_map_create(128, 128);
    hud_layer_init();
}

nshvc_tiles video_sprite_tiles(void)
{
    return tiles[VIDEO_TILES_HIGH];
}

/* ---- writes by video address ---- */

/* A write that is not whole tiles of a tile set it reaches: reported once
   (the record's writes measured so far are all whole tiles). */
static void not_whole_tiles(uint16_t address, uint32_t words)
{
    static int told;

    if (!told) {
        told = 1;
        fprintf(stderr, "video: a write of %u words at video address %04X is not whole tiles; left out of the tile sets\n",
                words, address);
    }
}

/* words [first, first + n) of a tile set whose tiles are `size` words,
   its tile 0 at video address `base` (below 0 for a set that wraps round) */
static void tiles_part(nshvc_tiles t, int32_t base, uint32_t size, uint32_t first, uint32_t n, const uint16_t *w)
{
    uint32_t at = (uint32_t)((int32_t)first - base);
    uint8_t bytes[2 * 256];

    if (at % size || n % size) {
        not_whole_tiles((uint16_t)first, n);
        return;
    }
    /* the planar bytes in the original's order: each word low, then high; in runs of 256 words
       (whole tiles of 16 or 8 words), since a write can be longer than bytes (the text map's 1,024) */
    while (n > 0) {
        uint32_t run = n < 256 ? n : 256;

        for (uint32_t i = 0; i < run; i++) {
            bytes[2 * i] = (uint8_t)w[i];
            bytes[2 * i + 1] = (uint8_t)(w[i] >> 8);
        }
        nshvc_tiles_write(t, at / size, bytes, run / size, NSHVC_TILES_PLANAR);
        at += run;
        w += run;
        n -= run;
    }
}

/* words [first, first + n) of a map of 32 x 32 pages, page 0 at `base`; the pages lie two across (page 1 right of 0),
   and in a map 64 high (`rows` 64) two more below (page 2 under 0) */
static void map_part(nshvc_map m, uint16_t base, uint32_t first, uint32_t n, const uint16_t *w, unsigned rows)
{
    while (n > 0) {
        uint32_t at = first - base, page = at >> 10, cell = at & 0x3FF;
        uint32_t x = cell & 31, y = cell >> 5, run = 32 - x;

        if (run > n)
            run = n;
        nshvc_map_write_snes(m, (page & 1) * 32 + x, y + (rows == 64 ? (page >> 1) * 32 : 0), run, 1, w);
        first += run;
        w += run;
        n -= run;
    }
}

/* a play field or backdrop's video map (64 x 32) written whole, from its 64 x 32 words row by row (LevelMapsFreeze) */
void video_map_write(VideoMap map, const uint16_t *words)
{
    nshvc_map_write_snes(maps[map], 0, 0, 64, 32, words);
}

/* cells [first, first + n) of the text map (row by row, 32 a row) to the map a level draws its text layer from */
static void text_map_part(uint32_t first, uint32_t n, const uint16_t *w)
{
    while (n > 0) {
        uint32_t x = first & 31, run = 32 - x;

        if (run > n)
            run = n;
        hud_layer_write(x, first >> 5, run, w);
        first += run;
        w += run;
        n -= run;
    }
}

/* the part of [first, first + n) inside [lo, hi), as its first and count; 0 if none */
static uint32_t inside(uint32_t first, uint32_t n, uint32_t lo, uint32_t hi, uint32_t *from)
{
    uint32_t a = first > lo ? first : lo, b = first + n < hi ? first + n : hi;

    *from = a;
    return b > a ? b - a : 0;
}

static void mode7_word(uint32_t at, uint16_t w)
{
    mode7_cells[at] = (uint8_t)w;
    mode7_pixels[at] = (uint8_t)(w >> 8);
    mode7_cells_changed = mode7_pixels_changed = 1;
}

static void write_words(uint16_t address, const uint16_t *w, uint32_t n)
{
    uint32_t first = address & 0x7FFFu, from, k;

    /* the video address has 15 bits: past $7FFF a write goes on at 0 */
    if (first + n > 0x8000) {
        uint32_t part = 0x8000 - first;

        write_words((uint16_t)first, w, part);
        write_words(0, w + part, n - part);
        return;
    }
    if (DevVramShadow)
        memcpy(DevVramShadow + first, w, n * sizeof *w);

    if ((k = inside(first, n, 0x0000, 0x4000, &from)) != 0) {
        const uint16_t *p = w + (from - first);
        uint32_t f2, k2;

        tiles_part(tiles[VIDEO_TILES_LOW], 0x0000, 16, from, k, p);
        /* the high set's tiles 768-1,023: its base $5000 wraps round to 0 */
        if ((k2 = inside(from, k, 0x0000, 0x1000, &f2)) != 0)
            tiles_part(tiles[VIDEO_TILES_HIGH], -0x3000, 16, f2, k2, p + (f2 - from));
        if (mode7_up)
            for (uint32_t i = 0; i < k; i++)
                mode7_word(from + i, p[i]);
    }
    if ((k = inside(first, n, 0x3800, 0x4800, &from)) != 0)
        map_part(maps[VIDEO_MAP_BACK_TALL], 0x3800, from, k, w + (from - first), 64);
    if ((k = inside(first, n, 0x4000, 0x4800, &from)) != 0)
        map_part(maps[VIDEO_MAP_BACK], 0x4000, from, k, w + (from - first), 32);
    if ((k = inside(first, n, 0x4800, 0x5000, &from)) != 0)
        map_part(maps[VIDEO_MAP_FRONT], 0x4800, from, k, w + (from - first), 32);
    if ((k = inside(first, n, 0x5000, 0x8000, &from)) != 0)
        tiles_part(tiles[VIDEO_TILES_HIGH], 0x5000, 16, from, k, w + (from - first));
    if ((k = inside(first, n, 0x5000, 0x7000, &from)) != 0)
        tiles_part(tiles[VIDEO_TILES_TEXT], 0x5000, 8, from, k, w + (from - first));
    if ((k = inside(first, n, 0x5C00, 0x6000, &from)) != 0) {
        map_part(maps[VIDEO_MAP_TEXT], 0x5C00, from, k, w + (from - first), 32);
        text_map_part(from - 0x5C00, k, w + (from - first));
    }
}

void vram_write(uint16_t address, const void *data, uint32_t count, VramWrite how)
{
    const uint8_t *b = (const uint8_t *)data;

    switch (how) {
    case VRAM_WORDS: {
        uint16_t w[256];

        /* in runs the size of w, from bytes in the original's order (low, high) */
        while (count > 0) {
            uint32_t run = count < 256 ? count : 256;

            for (uint32_t i = 0; i < run; i++)
                w[i] = (uint16_t)(b[2 * i] | b[2 * i + 1] << 8);
            write_words(address, w, run);
            address = (uint16_t)(address + run);
            b += 2 * run;
            count -= run;
        }
        break;
    }
    case VRAM_LOW_BYTES:
        for (uint32_t i = 0; i < count; i++)
            if ((uint16_t)(address + i) < 0x4000) {
                mode7_cells[(uint16_t)(address + i)] = b[i];
                mode7_cells_changed = 1;
            }
        break;
    case VRAM_HIGH_BYTES:
        for (uint32_t i = 0; i < count; i++)
            if ((uint16_t)(address + i) < 0x4000) {
                mode7_pixels[(uint16_t)(address + i)] = b[i];
                mode7_pixels_changed = 1;
            }
        break;
    }
}

void vram_write_stepped(uint16_t address, const void *data, uint32_t count, uint16_t step)
{
    const uint8_t *b = (const uint8_t *)data;

    if (step == 1) {
        vram_write(address, data, count, VRAM_WORDS);
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        uint16_t w = (uint16_t)(b[2 * i] | b[2 * i + 1] << 8);

        write_words(address, &w, 1);
        address = (uint16_t)(address + step);
    }
}

void vram_fill(uint16_t address, uint16_t word, uint32_t count)
{
    uint16_t w[256];

    for (int i = 0; i < 256; i++)
        w[i] = word;
    while (count > 0) {
        uint32_t run = count < 256 ? count : 256;

        write_words(address, w, run);
        address = (uint16_t)(address + run);
        count -= run;
    }
}

/* ---- Mode 7 ---- */

void video_mode7_off(void)
{
    mode7_up = 0;
    mode7_outside = 0;
}

void video_mode7_build(uint8_t row_flag, const uint8_t *planar, const uint8_t *map_words)
{
    /* the pixels: each of the 256 tiles a byte a pixel, its colours in the
       palette row the table gives for its row of 16 in the sheet */
    for (int t = 0; t < 256; t++) {
        const uint8_t *tile = planar + 32 * t;
        uint8_t row = mode7_palette_row((uint8_t)((t >> 4) | row_flag));

        for (int y = 0; y < 8; y++) {
            uint8_t p0 = tile[2 * y], p1 = tile[2 * y + 1];
            uint8_t p2 = tile[16 + 2 * y], p3 = tile[16 + 2 * y + 1];

            for (int x = 0; x < 8; x++) {
                int bit = 7 - x;
                uint8_t px = (uint8_t)((p0 >> bit & 1) | (p1 >> bit & 1) << 1 | (p2 >> bit & 1) << 2 | (p3 >> bit & 1) << 3);

                mode7_pixels[t * 64 + y * 8 + x] = px ? (uint8_t)(px | row) : 0;
            }
        }
    }
    /* the map: all tile 0, then the 32 x 32 map's words' low bytes at its top left */
    memset(mode7_cells, 0, sizeof mode7_cells);
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            mode7_cells[y * 128 + x] = map_words[2 * (y * 32 + x)];
    mode7_cells_changed = mode7_pixels_changed = 1;
    mode7_up = 1;
    /* the original's staging left in its bank $7F, which the title's waves read later (window_effects.c): the
       last block's pixels (tiles 0-63) at $5800, the map's low bytes at $5000, and the back map's second page
       (1,024 bytes read back from video memory, which no caller holds) at $5400. Measured in a recording of
       the original (power-on to the title, $7F:5400-$7F:57FF at every 40th frame from the title on): those
       1,024 bytes are 0 there, as the opening story leaves that page; the waves are the only reader. */
    for (int i = 0; i < 64 * 64; i++)
        g.unpacked[0x5800 + i] = mode7_pixels[i];
    for (int i = 0; i < 32 * 32; i++)
        g.unpacked[0x5000 + i] = map_words[2 * i];
    memset(&g.unpacked[0x5400], 0, 32 * 32);
    mode7_outside = 1;
}

/* Mode7ElementScale (80:867A): the element times the scale, both negated
   first as the original's multiplier took them, the product's middle and
   high bytes */
static int16_t mode7_scaled(int16_t element, uint8_t scale)
{
    int32_t product = (int32_t)(int16_t)(uint16_t)(0u - (uint16_t)element) * (int8_t)(uint8_t)(0u - scale);

    return (int16_t)(uint16_t)((uint32_t)product >> 8);
}

/* ---- the NMI's uploads ---- */

/* how far the address steps after each word, from a queue record's step
   byte (bits 0-1: 1, 32, 128, 128; bit 7: after the high byte) */
uint16_t vram_step(uint8_t step_byte)
{
    static const uint16_t steps[4] = { 1, 32, 128, 128 };

    if (!(step_byte & 0x80)) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: a word write that steps after its low byte (%02X); taken as after the high byte\n", step_byte);
        }
    }
    return steps[step_byte & 3];
}

void video_vblank(void)
{
    video_scroll_latch();
    if (g.frame_ready) {
        memcpy(shown_sprites, OamSprites(), g.sprite_count * sizeof shown_sprites[0]);
        shown_count = g.sprite_count;
        if (g.vram_queue_pending) {
            for (int i = 0; i < g.vram_queue_count; i++) {
                const VramQueueEntry *c = &g.vram_queue[i];

                vram_write_stepped(c->to, c->from, c->bytes / 2u, vram_step(c->step));
            }
            g.vram_queue_count = 0;
            g.vram_queue_pending = 0;
        }
        LayersCopyToVram();
    }
    VramListsUpload();
    if (g.tile_blocks_pending) {
        for (unsigned i = 0; i < g.tile_blocks_count; i++) {
            const TileBlock *t = &g.tile_blocks[i];

            write_words(t->to, &t->words[0], 2);
            write_words((uint16_t)(t->to + 32), &t->words[2], 2);
        }
        g.tile_blocks_count = 0;
        g.tile_blocks_pending = 0;
    }
    VramWordsUpload();
    if (g.text_map_upload) {
        g.text_map_upload = 0;
        write_words(g.text_upload_to, g.text_map, g.text_upload_bytes / 2u < 1024 ? g.text_upload_bytes / 2u : 1024);
    }
    TextBufferPartsUpload();
    VramUploadByNumber();
    if (g.palette_upload) {
        g.palette_upload = 0;
        nshvc_palette_write(0, 256, g.palette);
    }
    g.uploads_held = 0;                 /* the NMI clears it each frame */
    /* the NMI points channels 4-7 that are on at their tables' places */
    for (int ch = 4; ch < 8; ch++)
        if (g.line_effects_on >> ch & 1)
            g.line_channel[ch].table_at = g.line_table_at[ch - 4];
}

/* ---- the picture ---- */

/* a window use nibble of the original's (bit 0 window 1 inverted, 1 on,
   2 window 2 inverted, 3 on) and its logic, as the engine's */
uint8_t video_window_use(uint8_t nibble, uint8_t logic)
{
    return (uint8_t)((nibble & 2 ? NSHVC_WIN1 : 0) | (nibble & 1 ? NSHVC_WIN1_INVERT : 0) |
                     (nibble & 8 ? NSHVC_WIN2 : 0) | (nibble & 4 ? NSHVC_WIN2_INVERT : 0) | (logic & 3) << 4);
}

/* The bits a pixel BG layer `layer`'s tiles have, in the mode its tiles are
   drawn in: the picture's, or when that is Mode 7, the other mode a per-line
   mode change gives (the title's mode 0 above and below its Mode 7 band) */
static int video_tile_bpp(int layer)
{
    static const uint8_t bpp_of[8][3] = {
        { 2, 2, 2 }, { 4, 4, 2 }, { 4, 4, 0 }, { 8, 4, 0 }, { 8, 2, 0 }, { 4, 2, 0 }, { 4, 0, 0 }, { 0, 0, 0 }
    };
    int mode = g.mode;

    if (mode == 7)
        mode = video_lines_tile_mode();
    return bpp_of[mode & 7][layer];
}

/* The object a layer's map and tiles are (g.bg_map, g.bg_tiles); one no screen
   of the record uses is reported once and the layer left off. */
static int layer_objects(int layer, nshvc_tiles *t, nshvc_map *m)
{
    static int told;
    int map = g.bg_map[layer];
    int set = -1;
    int bpp = video_tile_bpp(layer);

    /* the 2bpp set is the one at $5000; the layer's mode says which depth it draws in */
    if (bpp == 2 && g.bg_tiles[layer] != VIDEO_TILES_LOW)
        set = VIDEO_TILES_TEXT;
    else if (bpp == 4 && g.bg_tiles[layer] == VIDEO_TILES_LOW)
        set = VIDEO_TILES_LOW;
    else if (bpp == 4)
        set = VIDEO_TILES_HIGH;
    if (map < 0 || map >= VIDEO_MAP_KINDS || set < 0) {
        if (!told && !g.blank) {        /* power-on's zeroed bases while the screen is still blank are no screen */
            told = 1;
            fprintf(stderr, "video: BG%d's map %d with tiles %d (drawn at %d bits) is no screen's of the record; left off\n",
                    layer + 1, map, g.bg_tiles[layer], bpp);
        }
        return 0;
    }
    *t = tiles[set];
    *m = maps[map];
    return 1;
}

static void draw_layers(void)
{
    for (int i = 0; i < 3; i++) {
        nshvc_layer l;

        memset(&l, 0, sizeof l);
        if (!layer_objects(i, &l.tiles, &l.map)) {
            nshvc_layer_set((uint8_t)i, &l);
            continue;
        }
        l.enabled_main = (uint8_t)(g.main_layers >> i & 1);
        l.enabled_sub = (uint8_t)(g.sub_layers >> i & 1);
        l.tile_size = (uint8_t)(g.tile_size_16 >> i & 1 ? 16 : 8);
        l.scroll_x = video_layer_x(i);
        l.scroll_y = video_layer_y(i);
        if (i < 2 && level_maps_active(i)) {
            /* a level's layer: its whole-level map, at its scroll as a signed 16-bit position (the map wraps at its own
               size, not at 512 or 1,024) */
            int32_t mx, my;

            l.map = level_maps_object(i);
            level_maps_margin(i, &mx, &my);     /* the level's (0, 0) is inside the map's margins */
            l.scroll_x = (int16_t)l.scroll_x + mx;
            l.scroll_y = (int16_t)l.scroll_y + my;
        }
        l.mosaic = (uint8_t)(g.mosaic_layers >> i & 1 ? g.mosaic_size + 1 : 1);
        l.wrap = 1;
        if (i == 2 && video_level_view() && g.bg_map[2] == VIDEO_MAP_TEXT)
            hud_layer_set(&l, l.scroll_x);      /* the text layer in a level: the status line at the view's corners */
        nshvc_layer_set((uint8_t)i, &l);
    }
}

static void draw_mode7(void)
{
    nshvc_mode7 m;

    if (mode7_pixels_changed) {
        nshvc_tiles_write(mode7_tiles, 0, mode7_pixels, 256, NSHVC_TILES_MODE7);
        mode7_pixels_changed = 0;
    }
    if (mode7_cells_changed) {
        static nshvc_map_entry cells[128 * 128];

        for (int i = 0; i < 128 * 128; i++)
            cells[i].tile = mode7_cells[i];
        nshvc_map_write(mode7_map, 0, 0, 128, 128, cells);
        mode7_cells_changed = 0;
    }
    memset(&m, 0, sizeof m);
    if (g.mode7_how == 1) {
        m.a = mode7_scaled(g.m7_matrix[0], g.mode7_scale[0]);
        m.b = mode7_scaled(g.m7_matrix[1], g.mode7_scale[1]);
        m.c = mode7_scaled(g.m7_matrix[2], g.mode7_scale[2]);
        m.d = mode7_scaled(g.m7_matrix[3], g.mode7_scale[3]);
    } else {
        m.a = g.m7_matrix[0];
        m.b = g.m7_matrix[1];
        m.c = g.m7_matrix[2];
        m.d = g.m7_matrix[3];
    }
    m.centre_x = (int16_t)g.m7_centre_x;
    m.centre_y = (int16_t)g.m7_centre_y;
    m.scroll_x = (int16_t)video_layer_x(0);
    m.scroll_y = (int16_t)video_layer_y(0);
    m.outside = mode7_outside;
    m.tiles = mode7_tiles;
    m.map = mode7_map;
    nshvc_mode7_set(&m);
}

/* the rows cropped from the top of the picture: the IRQ split's dark rows L (8, or the level's split line), 0 without it */
int32_t video_cropped_rows(void)
{
    return g.split_on ? (int32_t)(g.split_line ? g.split_line : 8) : 0;
}

void video_draw(void)
{
    nshvc_screen scr;
    nshvc_windows win;
    nshvc_blend blend;
    uint32_t top = 0, lit = 0;

    memset(&scr, 0, sizeof scr);
    scr.width = view.w;
    scr.height = view.h;
    scr.brightness = g.brightness;
    scr.blank = g.blank;
    if (g.split_on) {
        /* the IRQ: the screen is dark above its line L and lit from row L (the interrupt is at the line's dot $98 and
           its writes show before the line is drawn: the original's rows 0-7 are dark for L = 8); the tables the
           handler restarts give their first value from the row after. The dark rows are cropped, not shown
           (Jeff, 2026-11-22): the picture starts at the original's line L, so its height is the view's less L and
           its origin is -L; the line tables, the layers' scrolls and the line numbers of every effect keep the
           original's counting (the engine counts lines from the origin), and the sprites are moved up L below */
        lit = (uint32_t)video_cropped_rows();
        top = lit + 1u;
        scr.blank = 1;
        scr.height = view.h - (int32_t)lit;
        scr.origin_y = -(int32_t)lit;
    }
    if (!video_level_view()) {
        /* a still screen (no level is the screen): the original's 256 x 224 centred in the view, black around it
           (VIEW.md 6.1, 6.4). The engine's origin moves the layers, Mode 7 and the line tables; the sprites and the
           windows are moved below and in lines.c */
        scr.origin_x = view.dx;
        scr.origin_y = view.dy;
        scr.show_left = view.dx;
        scr.show_right = view.dx + VIEW_ORIGINAL_W;
        scr.show_top = view.dy;
        scr.show_bottom = view.dy + VIEW_ORIGINAL_H;
    }
    if (view.square)        /* the picture at its own shape: square game pixels, fitted to the window and letterboxed if the window's shape is not its */
        scr.aspect = (float)scr.width / (float)scr.height;
    nshvc_screen_set(&scr);

    nshvc_mode_set(g.mode, g.bg3_priority, 0);
    draw_layers();
    if (g.mode == 7)
        draw_mode7();

    memset(&win, 0, sizeof win);
    win.w[0].left = g.window_edges[0];
    win.w[0].right = g.window_edges[1];
    win.w[1].left = g.window_edges[2];
    win.w[1].right = g.window_edges[3];
    if (!video_level_view())            /* a still screen's windows are the original's columns, moved by the origin */
        for (int i = 0; i < 2; i++) {
            win.w[i].left += view.dx;
            win.w[i].right += view.dx;
        }
    for (int i = 0; i < 6; i++)
        win.layer_use[i] = video_window_use(g.window_use[i], g.window_logic[i]);
    for (int i = 0; i < 5; i++) {
        win.mask_main[i] = (uint8_t)(g.window_main >> i & 1);
        win.mask_sub[i] = (uint8_t)(g.window_sub >> i & 1);
    }
    nshvc_windows_set(&win);

    memset(&blend, 0, sizeof blend);
    blend.subtract = g.blend_subtract;
    blend.half = g.blend_half;
    blend.use_fixed = (uint8_t)!g.blend_use_sub;
    blend.layers = g.blend_layers;
    blend.clip = g.blend_clip;
    blend.prevent = g.blend_prevent;
    blend.fixed_bgr15 = (uint16_t)(g.fixed_red | g.fixed_green << 5 | g.fixed_blue << 10);
    nshvc_blend_set(&blend);

    nshvc_sprites_show((uint8_t)(g.main_layers >> 4 & 1), (uint8_t)(g.sub_layers >> 4 & 1));
    /* the engine's origin does not move sprites: the cropped rows' L lines up in a level with the split, and in
       a still screen the origin's dx and dy across and down (the sprite list as the game left it is kept) */
    int32_t sprite_dx = video_level_view() ? 0 : view.dx, sprite_dy = video_level_view() ? -(int32_t)lit : view.dy;

    if (sprite_dx || sprite_dy) {
        memcpy(moved_sprites, shown_sprites, shown_count * sizeof moved_sprites[0]);
        for (uint32_t i = 0; i < shown_count; i++) {
            moved_sprites[i].x += sprite_dx;
            moved_sprites[i].y += sprite_dy;
        }
        nshvc_sprites_set(moved_sprites, shown_count, 0);
    } else {
        nshvc_sprites_set(shown_sprites, shown_count, 0);
    }

    if (top) {
        nshvc_line(lit, NSHVC_LINE_BLANK, g.blank);
        nshvc_line(lit, NSHVC_LINE_BRIGHTNESS, g.brightness);
    }
    video_lines_send(top);
}

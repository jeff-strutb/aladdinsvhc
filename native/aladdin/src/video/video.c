/*
 * video.c: the engine objects the picture is made of, the one function that
 * turns a video address into them, the uploads the original's NMI made, and
 * the picture (video.h says when the loop calls each).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nshvc/nshvc.h"
#include "actor/actor.h"
#include "data/video_tables.h"
#include "system/runahead.h"
#include "system/state.h"
#include "system/system.h"
#include "video/between.h"
#include "video/level_maps.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

/* the objects (video.h lists the video addresses each shows) */
static nshvc_tiles tiles[VIDEO_TILE_KINDS];
static nshvc_map maps[VIDEO_MAP_KINDS];
static nshvc_tiles mode7_tiles;
/* the tiles of the slots past the original's four: a tile set for each SLOT_SET_SLOTS of them, made as the slots open */
static nshvc_tiles slot_sets[(SLOTS_MAX - SLOTS_STD + SLOT_SET_SLOTS - 1) / SLOT_SET_SLOTS];
static unsigned slot_sets_made;
static nshvc_map mode7_map;

/* Mode 7's map and pixels as the game's own arrays: 128 x 128 tiles, and
   256 tiles of 64 pixels, a byte each; sent when they change. */
static uint8_t mode7_cells[128 * 128];
static uint8_t mode7_pixels[256 * 64];
static uint8_t mode7_cells_changed, mode7_pixels_changed;
RUNAHEAD_KEEP(mode7_cells)
RUNAHEAD_KEEP(mode7_pixels)
RUNAHEAD_KEEP(mode7_cells_changed)
RUNAHEAD_KEEP(mode7_pixels_changed)
/* BG2's 64 x 64 map at $3800-$47FF as the game's own words: a byte upload to the map (one half of each word, as the lava
   flow's picture on level 7 is animated) keeps the other half */
#define BACK_TALL_AT 0x3800
#define BACK_TALL_WORDS 0x1000
static uint16_t back_tall_words[BACK_TALL_WORDS];
RUNAHEAD_KEEP(back_tall_words)
static uint8_t mode7_up;            /* Mode7ScreenSetup ran since DisplayDefaults: writes reach Mode 7 */
static uint8_t mode7_outside;       /* outside the plane transparent (Mode7ScreenSetup); else it repeats */

/* the sprites the picture shows: the game's list as the last frame that
   sent it left it (the original's sprite table keeps its last upload) */
static nshvc_sprite *shown_sprites;
static uint8_t *shown_owner;        /* each shown sprite's actor record (oam.c), for the easing between updates */
static int16_t (*shown_anchor)[2];  /* and its actor's screen place as drawn */
static int32_t (*shown_anchor_fx)[2];   /* the same in 256ths, from the actor's 16.8 place (places between pixels) */
static uint32_t shown_count;
static nshvc_sprite *moved_sprites;  /* the shown sprites moved by the crop or a still screen's origin (video_draw) */
static uint32_t lists_cap;          /* the picture's two lists' room: the most any view has needed (between.c keeps the
                                       shown list of the update before, which may be a larger view's) */

/* the lists of sprites that are sized for the view: the game's (OamInit) and the picture's two. `keep`: the view changed in play, so the lists
   are made to the new size and what they hold stays (the picture's shown sprites are the update before's world) */
static void sprite_lists_make(int keep)
{
    nshvc_sprite *was = shown_sprites;
    uint32_t was_cap = lists_cap;

    if (keep)
        OamResize();
    else
        OamInit();
    if (OamSpriteCapacity() > lists_cap) {
        lists_cap = OamSpriteCapacity();
        uint8_t *was_owner = shown_owner;
        int16_t (*was_anchor)[2] = shown_anchor;
        int32_t (*was_anchor_fx)[2] = shown_anchor_fx;

        free(moved_sprites);
        shown_sprites = realloc(shown_sprites, lists_cap * sizeof *shown_sprites);
        shown_owner = realloc(shown_owner, lists_cap * sizeof *shown_owner);
        shown_anchor = realloc(shown_anchor, lists_cap * sizeof *shown_anchor);
        shown_anchor_fx = realloc(shown_anchor_fx, lists_cap * sizeof *shown_anchor_fx);
        moved_sprites = calloc(lists_cap, sizeof *moved_sprites);
        if (!shown_sprites || !shown_owner || !shown_anchor || !shown_anchor_fx || !moved_sprites) {
            fprintf(stderr, "video: no memory for the shown sprites\n");
            exit(1);
        }
        if (was) {
            between_rekeep(was, shown_sprites, lists_cap * sizeof *shown_sprites);
            between_rekeep(was_owner, shown_owner, lists_cap * sizeof *shown_owner);
            between_rekeep(was_anchor, shown_anchor, lists_cap * sizeof *shown_anchor);
            between_rekeep(was_anchor_fx, shown_anchor_fx, lists_cap * sizeof *shown_anchor_fx);
        }
        if (keep) {         /* what the lists held stays; the room added is empty */
            memset(shown_sprites + was_cap, 0, (lists_cap - was_cap) * sizeof *shown_sprites);
            memset(shown_owner + was_cap, 0, (lists_cap - was_cap) * sizeof *shown_owner);
            memset(shown_anchor + was_cap, 0, (lists_cap - was_cap) * sizeof *shown_anchor);
            memset(shown_anchor_fx + was_cap, 0, (lists_cap - was_cap) * sizeof *shown_anchor_fx);
        }
    }
    if (keep)
        return;
    memset(shown_sprites, 0, lists_cap * sizeof *shown_sprites);
    memset(shown_owner, 0, lists_cap * sizeof *shown_owner);
    memset(shown_anchor, 0, lists_cap * sizeof *shown_anchor);
    memset(shown_anchor_fx, 0, lists_cap * sizeof *shown_anchor_fx);
    shown_count = 0;
}

/*
 * The view is chosen here where a level loads and where a still screen starts (view.h), and follows the window at an update's start
 * (video_view_follow, below; VIEW.md 4.8). Everything that keeps
 * something sized from the view is redone: the sprite lists (the budget follows the view's area). Everything else
 * the game derives from the view reads it where it is used, or is set after this by the level's own set-up (the
 * camera's limits and aim, the range point, the spawn window, the line tables, the window edges); a level's load
 * runs this first, before LevelScrollLimits.
 */
static int level_loading;       /* video_view_level ran and LevelScreenSetup has not yet: a DisplayDefaults in between keeps the level's view */
RUNAHEAD_KEEP(level_loading)

static void view_changed(int changed)
{
    if (changed && runahead_guard())    /* the sprite lists are sized again: not run ahead */
        return;
    if (changed && shown_sprites)
        sprite_lists_make(0);
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

/* A target view that needs the level's margins made again (they cost the fill: 70 to 250 ms) is taken once the window has held it this many updates
   (0.1 s), so a drag makes none and the end of a drag one; a developer's view (ALADDIN_VIEW, ALADDIN_DEV_VIEW_AT) is taken at once. */
#define VIEW_SETTLE_UPDATES 6
#define VIEW_NARROWEST_W 288

static int32_t pending_w, pending_h;     /* the target a change that needs the margins made again is waiting on, and for how many updates it has held */
static unsigned follow_held;
static int follow_at_once;               /* a snapshot's load put a view in: the next change is taken without the wait */

static double follow_now(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec * 1e-6;
}

void video_view_follow(void)
{
    int32_t w, h;
    int level, square;
    double t0;

    view_dev_at(nshvc_updates());
    if (level_loading)
        return;
    level = video_level_view();
    view_target(level, &w, &h, &square);
    if (w < VIEW_NARROWEST_W && view.w >= VIEW_NARROWEST_W)
        return;                 /* a window never gives the original's 256 x 224 (a developer's ALADDIN_VIEW does): a view of 288 or more is not taken down to it */
    if (w == view.w && h == view.h) {
        follow_held = 0;
        follow_at_once = 0;
        return;
    }
    if (level && !view_forced() && !follow_at_once && LevelMapsNeedResize((w - VIEW_ORIGINAL_W) / 2)) {
        if (w != pending_w || h != pending_h) {
            pending_w = w;
            pending_h = h;
            follow_held = 0;
        }
        if (++follow_held < VIEW_SETTLE_UPDATES)
            return;
    }
    follow_held = 0;
    follow_at_once = 0;
    pending_w = pending_h = 0;
    t0 = follow_now();
    view_apply(w, h, square);
    if (shown_sprites)
        sprite_lists_make(1);
    between_reset_part(&view);          /* the picture of this update draws the new shape from "was": nothing eases between two */
    if (level) {
        int maps = LevelMapsResize();

        LevelMapsReset();

        PlacementsViewChanged();
        if (getenv("ALADDIN_DEV_VIEW"))
            fprintf(stderr, "view followed the window at update %llu: %d level maps made again, %.3f ms\n", (unsigned long long)nshvc_updates(), maps,
                    follow_now() - t0);
    }
}

void video_view_level_done(void)
{
    level_loading = 0;
}

/* A snapshot's view put in as a level's load puts a chosen one in: what is sized from it is made again (the sprite lists).
   Not run ahead: a snapshot is never loaded in an ahead step. */
void video_view_adopt(const View *saved, unsigned slots_open)
{
    view = *saved;
    slots_adopt(slots_open);
    if (shown_sprites)
        sprite_lists_make(0);
    follow_held = 0;            /* the window's shape is read afresh, and taken without the wait: a load is a cut (VIEW.md 4.8) */
    follow_at_once = 1;
}

/* Mode 7's tiles and map written to the engine from the game's arrays, at once (after a snapshot's load) */
void video_mode7_rewrite(void)
{
    static nshvc_map_entry cells[128 * 128];

    nshvc_tiles_write(mode7_tiles, 0, mode7_pixels, 256, NSHVC_TILES_MODE7);
    for (int i = 0; i < 128 * 128; i++) {
        cells[i].tile = mode7_cells[i];
        cells[i].palette = cells[i].priority = cells[i].hflip = cells[i].vflip = 0;
    }
    nshvc_map_write(mode7_map, 0, 0, 128, 128, cells);
}

/* the shown lists' elements in use (the rest of the lists are what an earlier, larger update left) */
static uint32_t shown_used(void)
{
    return shown_count;
}

void video_init(void)
{
    sprite_lists_make(0);
    tiles[VIDEO_TILES_LOW] = nshvc_tiles_create(4, 1024, 1);
    between_tiles_made(tiles[VIDEO_TILES_LOW], "tiles.low", 1024, 32);
    tiles[VIDEO_TILES_HIGH] = nshvc_tiles_create(4, 1024, 1);
    between_tiles_made(tiles[VIDEO_TILES_HIGH], "tiles.high", 1024, 32);
    tiles[VIDEO_TILES_TEXT] = nshvc_tiles_create(2, 1024, 1);
    between_tiles_made(tiles[VIDEO_TILES_TEXT], "tiles.text", 1024, 16);
    maps[VIDEO_MAP_BACK] = nshvc_map_create(64, 32);
    between_map_made(maps[VIDEO_MAP_BACK], "map.back", 64, 32);
    maps[VIDEO_MAP_FRONT] = nshvc_map_create(64, 32);
    between_map_made(maps[VIDEO_MAP_FRONT], "map.front", 64, 32);
    maps[VIDEO_MAP_TEXT] = nshvc_map_create(32, 32);
    between_map_made(maps[VIDEO_MAP_TEXT], "map.text", 32, 32);
    maps[VIDEO_MAP_BACK_TALL] = nshvc_map_create(64, 64);
    between_map_made(maps[VIDEO_MAP_BACK_TALL], "map.back_tall", 64, 64);
    video_slot_sets_make(slots_count());
    mode7_tiles = nshvc_tiles_create(8, 256, 1);
    mode7_map = nshvc_map_create(128, 128);
    hud_layer_init();
    line_history_init();
    arena_maps_init();
    /* what the picture reads that an update changes, kept as the update before's world (between.h) */
    between_keep(shown_sprites, lists_cap * sizeof *shown_sprites);
    between_keep(shown_owner, lists_cap * sizeof *shown_owner);
    between_keep(shown_anchor, lists_cap * sizeof *shown_anchor);
    between_keep(shown_anchor_fx, lists_cap * sizeof *shown_anchor_fx);
    between_keep(&shown_count, sizeof shown_count);
    between_keep(&mode7_up, sizeof mode7_up);
    between_keep(&mode7_outside, sizeof mode7_outside);
    between_keep(&view, sizeof view);
    between_variable(shown_sprites, sizeof *shown_sprites, shown_used);
    between_variable(shown_owner, sizeof *shown_owner, shown_used);
    between_variable(shown_anchor, sizeof *shown_anchor, shown_used);
    between_variable(shown_anchor_fx, sizeof *shown_anchor_fx, shown_used);
    video_scroll_keep();
    level_maps_keep();
}

/* the tile sets that slots 5 to `count` draw from, made if not yet */
void video_slot_sets_make(unsigned count)
{
    unsigned need = count > SLOTS_STD ? (count - SLOTS_STD + SLOT_SET_SLOTS - 1) / SLOT_SET_SLOTS : 0;

    while (slot_sets_made < need) {
        char name[24];

        slot_sets[slot_sets_made] = nshvc_tiles_create(4, SLOT_TILES * SLOT_SET_SLOTS, 1);
        snprintf(name, sizeof name, "slot_set.%u", slot_sets_made);
        between_tiles_made(slot_sets[slot_sets_made], name, SLOT_TILES * SLOT_SET_SLOTS, 32);
        slot_sets_made++;
    }
}

nshvc_tiles video_slot_tiles(unsigned slot)
{
    return slot_sets[(slot - 5) / SLOT_SET_SLOTS];
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
        between_tiles_write(t, at / size, bytes, run / size, NSHVC_TILES_PLANAR, 2u * run);
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
        between_map_write_snes(m, (page & 1) * 32 + x, y + (rows == 64 ? (page >> 1) * 32 : 0), run, 1, w);
        if (m.id == maps[VIDEO_MAP_BACK].id || m.id == maps[VIDEO_MAP_FRONT].id)
            arena_maps_note(m.id == maps[VIDEO_MAP_FRONT].id ? VIDEO_MAP_FRONT : VIDEO_MAP_BACK, (page & 1) * 32 + x, y, run, 1, w);
        first += run;
        w += run;
        n -= run;
    }
}

/* a play field or backdrop's video map (64 x 32) written whole, from its 64 x 32 words row by row (LevelMapsFreeze) */
void video_map_write(VideoMap map, const uint16_t *words)
{
    between_map_write_snes(maps[map], 0, 0, 64, 32, words);
    arena_maps_note(map, 0, 0, 64, 32, words);
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
    if ((k = inside(first, n, BACK_TALL_AT, BACK_TALL_AT + BACK_TALL_WORDS, &from)) != 0) {
        memcpy(back_tall_words + (from - BACK_TALL_AT), w + (from - first), k * sizeof *w);
        map_part(maps[VIDEO_MAP_BACK_TALL], 0x3800, from, k, w + (from - first), 64);
    }
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

/* `count` bytes at video address `address`, each the low or the high half of a word of BG2's map at $3800-$47FF, the other
   half kept: the map's words written as they are then */
static void back_tall_bytes(uint32_t address, const uint8_t *b, uint32_t count, int high)
{
    uint32_t first, last, i, n = inside(address, count, BACK_TALL_AT, BACK_TALL_AT + BACK_TALL_WORDS, &first);

    if (!n)
        return;
    last = first + n;
    for (i = first; i < last; i++) {
        uint16_t *word = &back_tall_words[i - BACK_TALL_AT];

        *word = high ? (uint16_t)((*word & 0x00FF) | b[i - address] << 8) : (uint16_t)((*word & 0xFF00) | b[i - address]);
    }
    write_words((uint16_t)first, back_tall_words + (first - BACK_TALL_AT), last - first);
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
        back_tall_bytes(address, b, count, 0);
        break;
    case VRAM_HIGH_BYTES:
        for (uint32_t i = 0; i < count; i++)
            if ((uint16_t)(address + i) < 0x4000) {
                mode7_pixels[(uint16_t)(address + i)] = b[i];
                mode7_pixels_changed = 1;
            }
        back_tall_bytes(address, b, count, 1);
        break;
    }
}

/* words of the slot tile set from word address `address` (VRAM_SLOT_TILES on), whole tiles only */
static void slot_words(uint32_t address, const uint16_t *w, uint32_t n)
{
    uint32_t at = address - VRAM_SLOT_TILES;
    uint32_t tiles_in = SLOT_TILES * (slots_count() - SLOTS_STD) * 16u;
    uint32_t set_words = SLOT_TILES * SLOT_SET_SLOTS * 16u;

    if (slots_count() <= SLOTS_STD || at >= tiles_in)
        return;
    if (at + n > tiles_in)
        n = tiles_in - at;
    while (n) {                     /* each set's part: a set holds SLOT_SET_SLOTS slots */
        uint32_t in_set = at % set_words, part = set_words - in_set < n ? set_words - in_set : n;

        tiles_part(slot_sets[at / set_words], 0, 16, in_set, part, w);
        at += part;
        w += part;
        n -= part;
    }
}

void vram_write_stepped(uint32_t address, const void *data, uint32_t count, uint16_t step)
{
    const uint8_t *b = (const uint8_t *)data;

    if (address >= VRAM_SLOT_TILES) {
        /* a slot's tiles: contiguous (the queue's uploads step by 1, or 0x80 after the high byte, which is 1) */
        for (uint32_t i = 0; i < count;) {
            uint16_t w[256];
            uint32_t run = count - i < 256 ? count - i : 256;

            if (step != 1)
                run = 1;
            for (uint32_t k = 0; k < run; k++)
                w[k] = (uint16_t)(b[2 * (i + k)] | b[2 * (i + k) + 1] << 8);
            slot_words(address, w, run);
            address += run * step;
            i += run;
        }
        return;
    }
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

/* mode7_scaled with the scale in 256ths of its own (an eased scale, between.h): the same at a whole scale (scale_fx
   256 times it), each negated first the same way */
static int16_t mode7_scaled_fx(int16_t element, int32_t scale_fx)
{
    int64_t product = (int64_t)(int16_t)(uint16_t)(0u - (uint16_t)element) * (int16_t)(uint16_t)(0u - (uint32_t)scale_fx);

    return (int16_t)(uint16_t)((uint64_t)product >> 16);
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
        memcpy(shown_owner, OamSpriteOwners(), g.sprite_count * sizeof shown_owner[0]);
        memcpy(shown_anchor, OamSpriteAnchors(), g.sprite_count * sizeof shown_anchor[0]);
        memcpy(shown_anchor_fx, OamSpriteAnchorsFx(), g.sprite_count * sizeof shown_anchor_fx[0]);
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
        between_palette_write(0, 256, g.palette);
        if (slots_count() > SLOTS_STD)
            between_palette_write(256, 16u * (slots_count() - SLOTS_STD), g.palette + 256);
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
static const int32_t map_rows[VIDEO_MAP_KINDS] = {        /* each video map's rows, as made (video_init) */
    [VIDEO_MAP_BACK] = 32, [VIDEO_MAP_FRONT] = 32, [VIDEO_MAP_TEXT] = 32, [VIDEO_MAP_BACK_TALL] = 64,
};

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
            level_maps_note(i, (int16_t)l.scroll_x, (int16_t)l.scroll_y);
            level_maps_margin(i, &mx, &my);     /* the level's (0, 0) is inside the map's margins */
            l.scroll_x = (int16_t)l.scroll_x + mx;
            l.scroll_y = (int16_t)l.scroll_y + my;
        }
        if (i < 2 && video_level_view())
            l.scroll_x -= view.dx;
        if (i < 2 && !level_maps_active(i))
            (void)arena_maps_layer(i, &l);     /* level $11's fight: the floor across a wide view (arena_maps.c) */      /* the original's screen is dx across in the view (video_draw) */
        l.scroll_x_sub = (int16_t)video_layer_x_sub(i);     /* between updates: the eased place's fraction (6.10) */
        l.scroll_y_sub = (int16_t)video_layer_y_sub(i);
        l.mosaic = (uint8_t)(g.mosaic_layers >> i & 1 ? g.mosaic_size + 1 : 1);
        l.wrap = 1;
        if (i == 0 && video_title_view())
            l.wrap = 0;     /* the title's menu rows are BG1's text map (the title's mode-1 lines): drawn once at the original's frame, not repeated every 256 across the whole view (VIEW.md 6.5) */
        if (i == 1 && video_level_view() && g.bg_map[1] == VIDEO_MAP_BACK_TALL)
            l.wrap = 0;     /* level 7's lava flow picture sits in the map's first 256 columns: the original's 256 never reached the map's wrapped copy of it (the flow is not near past 128 left of the view), a wide view does, at its right edge */
        if (i == 1 && video_level_view() && (uint8_t)g.level == 0x11 && video_lines_have_mode(2)) {
            /* the floor's columns (line_history.c), in the terms of the map BG2 draws: the level's, with its margin, or a
               video map (in the arena: $4800, 64 x 32) */
            int32_t mx = 0, my = 0, tall = map_rows[g.bg_map[1]] * 8;

            if (level_maps_active(1)) {
                level_maps_margin(1, &mx, &my);
                tall = level_maps_height(1);
            }
            line_history_offsets(&l, video_layer_x(1), video_layer_x(2), my, tall);
        }
        if (i == 2 && video_level_view() && g.bg_map[2] == VIDEO_MAP_TEXT) {
            l.scroll_x_sub = l.scroll_y_sub = 0;    /* the status line stays on the screen's pixels */
            hud_layer_set(&l, l.scroll_x);      /* the text layer in a level: the status line at the view's corners */
        }
        nshvc_layer_set((uint8_t)i, &l);
    }
}

/* Mode 7's arrays to its objects when an update changed them, at the update's end: held to the next update with the
   update's other writes (between.h), so a picture's Mode 7 art is the update before's as the rest */
void video_update_end(void)
{
    arena_maps_update_end();
    if (mode7_pixels_changed) {
        between_tiles_write(mode7_tiles, 0, mode7_pixels, 256, NSHVC_TILES_MODE7, sizeof mode7_pixels);
        mode7_pixels_changed = 0;
    }
    if (mode7_cells_changed) {
        static nshvc_map_entry cells[128 * 128];

        for (int i = 0; i < 128 * 128; i++)
            cells[i].tile = mode7_cells[i];
        between_map_write(mode7_map, 0, 0, 128, 128, cells);
        mode7_cells_changed = 0;
    }
}

/* ---- easing between updates (between.h; FRAMERATE.md 4.3, 4.4) ---- */

#define PLACE_FAR 32        /* an actor that moved further between two updates is placed anew (SRFX's SPRITE_FAR) */
#define SCROLL_FAR 64       /* a layer that moved further is set anew (a cut, a room change, a still screen's start) */

/* The shown sprites (the update before's) eased towards the newest. Each sprite moves as its actor's drawn place on the
   screen moved (oam.c's anchor: the actor's place less the sprites' origin, as the update drew it), from the update
   before's to the newest's: so it is exact at both ends, never wobbles against the screen, and follows whatever the
   origin took in (a layer's shift as well as the camera). An actor not drawn at the newest update (ended, off the
   view, or flickered out) is shown where the update before left it. One whose drawn place moved more than PLACE_FAR,
   or whose record is not the same actor at the newest update, is shown where it was too (placed anew). */
static void sprites_ease(void)
{
    const GameState *n = between_now();
    const uint8_t *no = between_now_of(shown_owner);
    const int16_t (*na)[2] = between_now_of(shown_anchor);
    const int32_t (*nf)[2] = between_now_of(shown_anchor_fx);
    uint32_t nc = *(const uint32_t *)between_now_of(&shown_count);
    int32_t to[ACTOR_RECORDS][2], to_fx[ACTOR_RECORDS][2];
    uint8_t drawn[ACTOR_RECORDS];
    int fractions = between_fractions();

    memset(drawn, 0, sizeof drawn);
    for (uint32_t j = 0; j < nc; j++)
        if (!drawn[no[j]]) {
            drawn[no[j]] = 1;
            to[no[j]][0] = na[j][0];
            to[no[j]][1] = na[j][1];
            to_fx[no[j]][0] = nf[j][0];
            to_fx[no[j]][1] = nf[j][1];
        }
    for (uint32_t i = 0; i < shown_count; i++) {
        uint8_t o = shown_owner[i];
        int moving = drawn[o] && g.actors[o].kind == n->actors[o].kind;
        int32_t dx = moving ? to[o][0] - shown_anchor[i][0] : 0, dy = moving ? to[o][1] - shown_anchor[i][1] : 0;

        if (dx > PLACE_FAR + SCROLL_FAR || dx < -(PLACE_FAR + SCROLL_FAR) || dy > PLACE_FAR + SCROLL_FAR || dy < -(PLACE_FAR + SCROLL_FAR))
            moving = 0;
        if (fractions) {
            /* the actor's true place on the screen, in 256ths, eased (or the update before's): the sprite at its
               piece's offset from it, the whole pixels in x, y and the fraction in x_sub, y_sub (engine 6.10) */
            int64_t fx = shown_anchor_fx[i][0], fy = shown_anchor_fx[i][1];

            if (moving) {
                fx = between_ease_fx(fx, to_fx[o][0], 32, (int64_t)1 << 30);
                fy = between_ease_fx(fy, to_fx[o][1], 32, (int64_t)1 << 30);
            }
            shown_sprites[i].x += (int32_t)(fx >> 8) - shown_anchor[i][0];
            shown_sprites[i].y += (int32_t)(fy >> 8) - shown_anchor[i][1];
            shown_sprites[i].x_sub = (int16_t)(fx & 0xFF);
            shown_sprites[i].y_sub = (int16_t)(fy & 0xFF);
        } else if (moving) {
            shown_sprites[i].x += between_ease(0, dx, 0, 32, 1 << 20);
            shown_sprites[i].y += between_ease(0, dy, 0, 32, 1 << 20);
        }
    }
}

/* the developer's check (between.c): for each actor drawn by the same pieces at both updates (the same tiles, sizes
   and flips, in the same order), its eased sprites against the newest update's: equal at between 1, between the two
   at any between (sprites of an actor that jumped, or a camera that did, are not counted) */
static const int16_t *nanc_of(uint32_t j)
{
    return ((const int16_t (*)[2])between_now_of(shown_anchor))[j];
}

void video_sprites_check(float b, unsigned *checked, unsigned *wrong)
{
    const GameState *n = between_now();
    const nshvc_sprite *ns = between_now_of(shown_sprites);
    const uint8_t *no = between_now_of(shown_owner);
    const uint32_t nc = *(const uint32_t *)between_now_of(&shown_count);
    static nshvc_sprite was[4096];
    uint32_t count = shown_count < 4096 ? shown_count : 4096;

    memcpy(was, shown_sprites, count * sizeof was[0]);
    sprites_ease();
    for (uint32_t i = 0; i < count;) {
        uint8_t o = shown_owner[i];
        uint32_t len = 1, j, k;

        while (i + len < count && shown_owner[i + len] == o)
            len++;
        for (j = 0; j < nc && no[j] != o; j++)
            ;
        for (k = 0; k < len && j + k < nc && no[j + k] == o; k++) {
            const nshvc_sprite *p = &was[i + k], *q = &ns[j + k];

            const int16_t (*nanc)[2] = between_now_of(shown_anchor);

            if (p->tile != q->tile || p->width != q->width || p->hflip != q->hflip || p->vflip != q->vflip ||
                p->x - shown_anchor[i + k][0] != q->x - nanc[j + k][0] || p->y - shown_anchor[i + k][1] != q->y - nanc[j + k][1])
                break;          /* a piece placed otherwise in the actor: the art changed, which is not eased */
        }
        if (k == len && (j + k >= nc || no[j + k] != o) && g.actors[o].kind == n->actors[o].kind) {
            /* places in 256ths: with fractions, the actor's true place (its anchor's 256ths) plus the piece's offset;
               the eased sprite's whole pixels and fraction */
            const int32_t (*nfx)[2] = between_now_of(shown_anchor_fx);
            int fr = between_fractions();
            int64_t wx = fr ? (int64_t)(was[i].x - shown_anchor[i][0]) * 256 + shown_anchor_fx[i][0] : (int64_t)was[i].x * 256;
            int64_t wy = fr ? (int64_t)(was[i].y - shown_anchor[i][1]) * 256 + shown_anchor_fx[i][1] : (int64_t)was[i].y * 256;
            int64_t nx = fr ? (int64_t)(ns[j].x - nanc_of(j)[0]) * 256 + nfx[j][0] : (int64_t)ns[j].x * 256;
            int64_t ny = fr ? (int64_t)(ns[j].y - nanc_of(j)[1]) * 256 + nfx[j][1] : (int64_t)ns[j].y * 256;
            int64_t gx = (int64_t)shown_sprites[i].x * 256 + shown_sprites[i].x_sub, gy = (int64_t)shown_sprites[i].y * 256 + shown_sprites[i].y_sub;
            int64_t dx = (nx - wx) / 256, dy = (ny - wy) / 256;
            int64_t ex, ey;

            if (!fr) {
                dx = ns[j].x - was[i].x;
                dy = ns[j].y - was[i].y;
            }
            ex = gx - wx;
            ey = gy - wy;
            dx = nx - wx;
            dy = ny - wy;

            if (dx <= (PLACE_FAR + SCROLL_FAR - 1) * 256 && dx >= -(PLACE_FAR + SCROLL_FAR - 1) * 256 &&
                dy <= (PLACE_FAR + SCROLL_FAR - 1) * 256 && dy >= -(PLACE_FAR + SCROLL_FAR - 1) * 256) {      /* not placed anew */
                (*checked)++;
                if (b >= 1.0f ? (ex != dx || ey != dy)
                              : ((dx >= 0 ? ex < 0 || ex > dx : ex > 0 || ex < dx) || (dy >= 0 ? ey < 0 || ey > dy : ey > 0 || ey < dy))) {
                    static int shown;

                    (*wrong)++;
                    if (shown++ < 12)
                        fprintf(stderr, "between check: anchors was %d,%d now %d,%d; pieces %u; ", shown_anchor[i][0], shown_anchor[i][1],
                                ((const int16_t (*)[2])between_now_of(shown_anchor))[j][0], ((const int16_t (*)[2])between_now_of(shown_anchor))[j][1], len),
                        fprintf(stderr, "sprite of actor %u kind %02X at %.1f: was %d,%d now %d,%d eased %d,%d actor %06X,%06X -> %06X,%06X cam %06X,%06X -> %06X,%06X\n",
                                o, g.actors[o].kind, (double)b, (int)was[i].x, (int)was[i].y, (int)ns[j].x, (int)ns[j].y, (int)shown_sprites[i].x, (int)shown_sprites[i].y,
                                (unsigned)g.actors[o].x, (unsigned)g.actors[o].y, (unsigned)n->actors[o].x, (unsigned)n->actors[o].y,
                                (unsigned)g.layers[0].scroll_x, (unsigned)g.layers[0].scroll_y, (unsigned)n->layers[0].scroll_x, (unsigned)n->layers[0].scroll_y);
                }
            }
        }
        i += len;
    }
    memcpy(shown_sprites, was, count * sizeof was[0]);
}

static void draw_mode7(void)
{
    nshvc_mode7 m;
    const GameState *n = between_now();

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
    if (between_easing() && g.mode7_how == n->mode7_how) {
        /* the matrix, centre and scales eased with their own steps (between.h): the Genie wheel's slowing turns,
           the ending's spin-zoom's zoom */
        int16_t e[4], centre[2];
        int32_t scale[4];

        if (between_mode7(e, centre, scale)) {
            if (g.mode7_how == 1) {
                m.a = mode7_scaled_fx(e[0], scale[0]);
                m.b = mode7_scaled_fx(e[1], scale[1]);
                m.c = mode7_scaled_fx(e[2], scale[2]);
                m.d = mode7_scaled_fx(e[3], scale[3]);
            } else {
                m.a = e[0];
                m.b = e[1];
                m.c = e[2];
                m.d = e[3];
            }
            m.centre_x = centre[0];
            m.centre_y = centre[1];
        }
    }
    m.scroll_x = (int16_t)video_layer_x(0);
    m.scroll_y = (int16_t)video_layer_y(0);
    m.scroll_x_sub = (int16_t)video_layer_x_sub(0);
    m.scroll_y_sub = (int16_t)video_layer_y_sub(0);
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

static int no_sprites(void)
{
    static int on = -1;

    if (on < 0)
        on = getenv("ALADDIN_DEV_NO_SPRITES") != NULL;
    return on;
}

void video_draw(void)
{
    nshvc_screen scr;
    nshvc_windows win;
    nshvc_blend blend;
    uint32_t top = 0, lit = 0;

    if (between_easing()) {     /* a picture between updates: the moving parts eased towards the newest */
        video_scroll_ease();
        sprites_ease();
    }
    memset(&scr, 0, sizeof scr);
    scr.width = view.w;
    scr.height = view.h;
    {
        uint32_t b = between_brightness();     /* a fade's steps eased (between.h) */

        scr.brightness = (uint8_t)(b >> 8);
        scr.brightness_sub = (uint8_t)(b & 0xFF);
    }
    scr.blank = g.blank;
    if (g.split_on) {
        /* the IRQ: the screen is dark above its line L and lit from row L (the interrupt is at the line's dot $98 and
           its writes show before the line is drawn: the original's rows 0-7 are dark for L = 8); the tables the
           handler restarts give their first value from the row after. The dark rows are cropped, not shown
           (Jeff, 2026-11-22): the picture starts at the original's line L, so its height is the view's less L */
        lit = (uint32_t)video_cropped_rows();
        top = lit + 1u;
        if (!video_level_view())
            scr.blank = 1;      /* a level shows the frame round its screen above its line L too, so it is not dark there */
        scr.height = view.h - (int32_t)lit;
    }
    if (video_level_view()) {
        /* a level: the original's 256 x 224 screen, the game as it plays, sits at (dx, dy) in the view, less the cropped
           rows L (the picture starts at the original's line L). Down, the engine's origin does it: it moves the layers
           and the line tables. Across it is done here, in the layers' scrolls (draw_layers), in lines.c (the per-line
           scrolls, the windows) and in the sprites below: the status line's columns (hud_layer.c) are the picture's
           and a split inside a line cannot be placed left of the origin. What is round the screen is the level's own
           (its layers) and the actors the game keeps live there (oam.c). VIEW.md 4.4 */
        scr.origin_x = 0;
        scr.origin_y = view.dy - (int32_t)lit;
    } else {
        /* a still screen (no level is the screen): the original's 256 x 224 centred in the view, black around it
           (VIEW.md 6.1, 6.4). The engine's origin moves the layers, Mode 7 and the line tables; the sprites and the
           windows are moved below and in lines.c */
        scr.origin_x = view.dx;
        scr.origin_y = view.dy;
        if (!video_title_view()) {      /* the title is not cut to the frame: what the engine draws round it shows (VIEW.md 6.5) */
            scr.show_left = view.dx;
            scr.show_right = view.dx + VIEW_ORIGINAL_W;
            scr.show_top = view.dy;
            scr.show_bottom = view.dy + VIEW_ORIGINAL_H;
        }
    }
    if (view.square)        /* the picture at its own shape: square game pixels, fitted to the window and letterboxed if the window's shape is not its */
        scr.aspect = (float)scr.width / (float)scr.height;
    nshvc_screen_set(&scr);

    nshvc_mode_set(g.mode, g.bg3_priority, 0);
    draw_layers();
    if (g.mode == 7)
        draw_mode7();

    memset(&win, 0, sizeof win);
    /* the original's columns in the picture's (the engine's origin does not move windows; lines.c) */
    {
        uint8_t e[4];

        video_window_edges(e);  /* eased with the windows' steps (lines.c) */
        win.w[0].left = video_window_left(e[0]);
        win.w[0].right = video_window_right(e[1]);
        win.w[1].left = video_window_left(e[2]);
        win.w[1].right = video_window_right(e[3]);
    }
    for (int i = 0; i < 6; i++)
        win.layer_use[i] = video_window_use(g.window_use[i], g.window_logic[i]);
    for (int i = 0; i < 5; i++) {
        win.mask_main[i] = (uint8_t)(g.window_main >> i & 1);
        win.mask_sub[i] = (uint8_t)(g.window_sub >> i & 1);
    }
    if (video_level_view() && g.bg_map[1] == VIDEO_MAP_BACK_TALL && view.dx) {
        /* level 7's lava flow picture is placed from the original's screen (at the start it sits just left of it): in a wider view
           it is kept to the original's columns, as the original showed it, by a window no layer of the game uses (the level's end
           wipe uses both: then none, and BG2 is not masked) */
        int used[2] = { 0, 0 }, free_window = -1;

        for (int i = 0; i < 6; i++) {
            used[0] |= win.layer_use[i] & NSHVC_WIN1;
            used[1] |= win.layer_use[i] & NSHVC_WIN2;
        }
        free_window = !used[1] ? 1 : !used[0] ? 0 : -1;
        if (free_window >= 0 && !win.mask_main[1] && !win.mask_sub[1] && !win.layer_use[1]) {
            win.w[free_window].left = view.dx;
            win.w[free_window].right = view.dx + VIEW_ORIGINAL_W - 1;
            win.layer_use[1] = (uint8_t)((free_window ? NSHVC_WIN2 | NSHVC_WIN2_INVERT : NSHVC_WIN1 | NSHVC_WIN1_INVERT));
            win.mask_main[1] = win.mask_sub[1] = 1;
        }
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
    int32_t sprite_dx = view.dx, sprite_dy = scr.origin_y;

    if (no_sprites()) {
        nshvc_sprites_set(shown_sprites, 0, 0);     /* the developer's: the level's layers alone (ALADDIN_DEV_NO_SPRITES, change_report.py) */
    } else if (sprite_dx || sprite_dy) {
        memcpy(moved_sprites, shown_sprites, shown_count * sizeof moved_sprites[0]);
        for (uint32_t i = 0; i < shown_count; i++) {
            moved_sprites[i].x += sprite_dx;
            moved_sprites[i].y += sprite_dy;
        }
        nshvc_sprites_set(moved_sprites, shown_count, 0);
    } else {
        nshvc_sprites_set(shown_sprites, shown_count, 0);
    }

    if (top && !video_level_view()) {
        nshvc_line(lit, NSHVC_LINE_BLANK, g.blank);
        nshvc_line(lit, NSHVC_LINE_BRIGHTNESS, (int32_t)(between_brightness() >> 8));
        nshvc_line(lit, NSHVC_LINE_BRIGHTNESS_SUB, (int32_t)(between_brightness() & 0xFF));
    }
    video_lines_send(top);
}

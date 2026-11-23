/*
 * between.c: pictures between updates (between.h).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/runahead.h"
#include "system/state.h"
#include "video/between.h"
#include "video/slot_gfx.h"
#include "video/video.h"
#include "video/video_internal.h"

#define KEPT_MAX 32

/* ---- the kept state: g and the registered video parts ---- */

typedef struct Kept {
    void *live;
    void *was;
    size_t bytes;
    int owned;                  /* its owner hashes and snapshots it (it holds pointers or a handle): the generic ones leave it */
    size_t elem;                /* an array sized by the view (the sprite lists): the size of an element, and */
    uint32_t (*count)(void);    /* how many the view uses; the layout mark takes the name alone, the snapshot and the hash the used part */
    const char *name;
} Kept;

static Kept kept[KEPT_MAX];
static unsigned kept_count;
static GameState g_was, g_now;      /* g_now: the live state, put aside while a picture draws from "was" */
static void *kept_now[KEPT_MAX];
static int have_was;            /* an update has begun, so "was" is a world */
static int in_update;
static int drawing;             /* a picture is drawing from "was" */
static float amount;            /* its between */

void between_keep_named(void *p, size_t bytes, const char *name)
{
    if (kept_count == KEPT_MAX) {
        fprintf(stderr, "between: more than %d kept parts\n", KEPT_MAX);
        exit(2);
    }
    kept[kept_count].live = p;
    kept[kept_count].was = calloc(1, bytes);
    kept[kept_count].bytes = bytes;
    kept[kept_count].name = name;
    kept_now[kept_count] = calloc(1, bytes);
    if (!kept[kept_count].was || !kept_now[kept_count]) {
        fprintf(stderr, "between: no memory for a kept part of %zu bytes\n", bytes);
        exit(2);
    }
    kept_count++;
}

void between_owned(const void *live)
{
    for (unsigned i = 0; i < kept_count; i++)
        if (kept[i].live == live)
            kept[i].owned = 1;
}

void between_variable(const void *live, size_t elem, uint32_t (*count)(void))
{
    for (unsigned i = 0; i < kept_count; i++)
        if (kept[i].live == live) {
            kept[i].elem = elem;
            kept[i].count = count;
        }
}

/* the part's bytes that matter: all of a fixed part; of an array sized by the view, the elements the view uses */
static size_t kept_used(unsigned i)
{
    size_t used;

    if (!kept[i].count)
        return kept[i].bytes;
    used = kept[i].elem * kept[i].count();
    return used < kept[i].bytes ? used : kept[i].bytes;
}

unsigned between_kept_count(void)
{
    return kept_count;
}

void between_kept_at(unsigned i, const char **name, void **live, size_t *used, size_t *total, int *owned)
{
    *name = kept[i].name;
    *live = kept[i].live;
    *used = kept_used(i);
    *total = kept[i].bytes;
    *owned = kept[i].owned;
}

void between_rekeep(void *old, void *p, size_t bytes)
{
    for (unsigned i = 0; i < kept_count; i++)
        if (kept[i].live == old) {
            void *was = realloc(kept[i].was, bytes);
            void *now = realloc(kept_now[i], bytes);

            if (!was || !now) {
                fprintf(stderr, "between: no memory for a kept part of %zu bytes\n", bytes);
                exit(2);
            }
            if (bytes > kept[i].bytes)
                memset((uint8_t *)was + kept[i].bytes, 0, bytes - kept[i].bytes);
            kept[i].live = p;
            kept[i].was = was;
            kept_now[i] = now;
            kept[i].bytes = bytes;
            return;
        }
}

/* the live state put aside and "was" put in its place (a picture may change what it draws from, as easing does: the
   "was" copies stay as the update left them, for the next picture) */
static void enter_was(void)
{
    memcpy(&g_now, &g, sizeof g);
    memcpy(&g, &g_was, sizeof g);
    for (unsigned i = 0; i < kept_count; i++) {
        memcpy(kept_now[i], kept[i].live, kept[i].bytes);
        memcpy(kept[i].live, kept[i].was, kept[i].bytes);
    }
}

static void leave_was(void)
{
    memcpy(&g, &g_now, sizeof g);
    for (unsigned i = 0; i < kept_count; i++)
        memcpy(kept[i].live, kept_now[i], kept[i].bytes);
}

/* ---- for the easing, while a picture draws ---- */

int between_easing(void)
{
    return drawing;
}

int between_fractions(void)
{
    static int known, whole;

    if (!known) {
        known = 1;
        whole = getenv("ALADDIN_DEV_WHOLE_PIXELS") != NULL;    /* the developer's: places in whole pixels (behind.sh) */
    }
    return drawing && !whole;
}

int64_t between_ease_fx(int64_t was, int64_t now, unsigned bits, int64_t far)
{
    int64_t period = (int64_t)1 << bits, d = (now - was) % period;

    if (d < 0)
        d += period;
    if (d >= period / 2)
        d -= period;
    if (!drawing || d > far || d < -far)
        return was;
    return was + (int64_t)floor((double)d * (double)amount);
}

float between_amount(void)
{
    return amount;
}

const GameState *between_now(void)
{
    return drawing ? &g_now : &g;
}

const void *between_now_of(const void *live)
{
    for (unsigned i = 0; drawing && i < kept_count; i++)
        if (kept[i].live == live)
            return kept_now[i];
    return live;
}

int32_t between_ease(int64_t was, int64_t now, unsigned frac_bits, unsigned bits, int32_t far)
{
    int64_t period = (int64_t)1 << bits, d = (now - was) % period, at;

    if (d < 0)
        d += period;
    if (d >= period / 2)
        d -= period;                    /* the shorter way round */
    if (!drawing || d > ((int64_t)far << frac_bits) || d < -((int64_t)far << frac_bits))
        return 0;                       /* not moving smoothly: shown where the update before left it */
    at = was + (int64_t)floor((double)d * (double)amount);     /* the eased place, in fractions */
    return (int32_t)((at >> frac_bits) - (was >> frac_bits));
}

/* ---- a copy of what the game has written to the engine's tiles, maps and palette (the engine has no read back): the
   snapshot's engine art (FRAMERATE.md section 10). A write is copied when it is held, never from an ahead step. ---- */

enum { SH_TILES, SH_MAP };

typedef struct Shadow {
    char name[24];
    uint8_t kind;
    uint32_t id;                        /* the engine object's id in this process */
    uint32_t unit;                      /* bytes a tile; a map's cell is 2 */
    uint32_t units;                     /* tiles; map cells */
    uint32_t w;                         /* a map's width */
    uint8_t *data;
} Shadow;

#define SHADOW_MAX 32
static Shadow shadows[SHADOW_MAX];
static unsigned shadow_count;
static uint16_t palette_shadow[PALETTE_ENTRIES];

static Shadow *shadow_make(const char *name, uint8_t kind, uint32_t id, uint32_t unit, uint32_t units, uint32_t w)
{
    Shadow *s;

    if (shadow_count == SHADOW_MAX) {
        fprintf(stderr, "between: more than %d shadowed engine objects\n", SHADOW_MAX);
        exit(2);
    }
    s = &shadows[shadow_count++];
    snprintf(s->name, sizeof s->name, "%s", name);
    s->kind = kind;
    s->id = id;
    s->unit = unit;
    s->units = units;
    s->w = w;
    s->data = calloc(units, unit);
    if (!s->data) {
        fprintf(stderr, "between: no memory for a shadow of %u x %u bytes\n", units, unit);
        exit(2);
    }
    return s;
}

void between_tiles_made(nshvc_tiles t, const char *name, uint32_t count, uint32_t bytes_per_tile)
{
    shadow_make(name, SH_TILES, t.id, bytes_per_tile, count, 0);
}

void between_map_made(nshvc_map m, const char *name, uint32_t w, uint32_t h)
{
    shadow_make(name, SH_MAP, m.id, 2, w * h, w);
}

static Shadow *shadow_of(uint8_t kind, uint32_t id)
{
    for (unsigned i = 0; i < shadow_count; i++)
        if (shadows[i].kind == kind && shadows[i].id == id)
            return &shadows[i];
    return NULL;
}

static void shadow_tiles(uint32_t object, uint32_t first, const void *data, uint32_t count, size_t bytes)
{
    Shadow *s = shadow_of(SH_TILES, object);

    if (!s || runahead_ahead())
        return;
    if (count == 0 || bytes != (size_t)count * s->unit || (uint64_t)first + count > s->units) {
        fprintf(stderr, "between: a write of %u tiles (%zu bytes) at %u does not fit the shadow %s\n", count, bytes, first, s->name);
        exit(2);
    }
    memcpy(s->data + (size_t)first * s->unit, data, bytes);
}

static void shadow_map(uint32_t object, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *words)
{
    Shadow *s = shadow_of(SH_MAP, object);

    if (!s || runahead_ahead())
        return;
    if ((uint64_t)x + w > s->w || (uint64_t)(y + h) * s->w > s->units) {
        fprintf(stderr, "between: a map write of %u x %u at %u, %u does not fit the shadow %s\n", w, h, x, y, s->name);
        exit(2);
    }
    for (uint32_t r = 0; r < h; r++)
        memcpy(s->data + 2u * ((size_t)(y + r) * s->w + x), words + (size_t)r * w, 2u * w);
}

static void shadow_palette(uint32_t first, uint32_t count, const uint16_t *bgr15)
{
    if (runahead_ahead())
        return;
    if ((uint64_t)first + count > PALETTE_ENTRIES) {
        fprintf(stderr, "between: a palette write of %u at %u is past the shadow\n", count, first);
        exit(2);
    }
    memcpy(palette_shadow + first, bgr15, 2u * count);
}

/* ---- the held engine writes ---- */

enum { HOLD_TILES, HOLD_MAP, HOLD_MAP_SNES, HOLD_PALETTE };

typedef struct Held {
    uint8_t kind;
    uint32_t object;                    /* the tiles' or map's id */
    uint32_t a, b, c, d;                /* first, count, format / x, y, w, h / first, count */
    nshvc_tile_format format;
    size_t at, bytes;                   /* its data in hold_data */
} Held;

static Held *held;
static size_t held_count, held_cap;
static uint8_t *hold_data;
static size_t hold_used, hold_cap;

static Held *hold(uint8_t kind, const void *data, size_t bytes)
{
    Held *h;

    if (held_count == held_cap) {
        held_cap = held_cap ? 2 * held_cap : 256;
        held = realloc(held, held_cap * sizeof *held);
    }
    if (hold_used + bytes > hold_cap) {
        while (hold_used + bytes > hold_cap)
            hold_cap = hold_cap ? 2 * hold_cap : 1u << 20;
        hold_data = realloc(hold_data, hold_cap);
    }
    if (!held || !hold_data) {
        fprintf(stderr, "between: no memory to hold %zu bytes of writes\n", bytes);
        exit(2);
    }
    h = &held[held_count++];
    memset(h, 0, sizeof *h);
    h->kind = kind;
    h->at = hold_used;
    h->bytes = bytes;
    memcpy(hold_data + hold_used, data, bytes);
    hold_used += bytes;
    return h;
}

/* the writes the last update made, in its order */
static void write_held(void)
{
    for (size_t i = 0; i < held_count; i++) {
        const Held *h = &held[i];
        const void *d = hold_data + h->at;

        switch (h->kind) {
        case HOLD_TILES:
            nshvc_tiles_write((nshvc_tiles){ h->object }, h->a, d, h->b, h->format);
            break;
        case HOLD_MAP:
            nshvc_map_write((nshvc_map){ h->object }, h->a, h->b, h->c, h->d, d);
            break;
        case HOLD_MAP_SNES:
            nshvc_map_write_snes((nshvc_map){ h->object }, h->a, h->b, h->c, h->d, d);
            break;
        case HOLD_PALETTE:
            nshvc_palette_write(h->a, h->b, d);
            break;
        }
    }
    held_count = 0;
    hold_used = 0;
}

void between_tiles_write(nshvc_tiles t, uint32_t first, const void *data, uint32_t count, nshvc_tile_format f, size_t bytes)
{
    Held *h;

    shadow_tiles(t.id, first, data, count, bytes);
    if (!in_update) {
        nshvc_tiles_write(t, first, data, count, f);
        return;
    }
    h = hold(HOLD_TILES, data, bytes);
    h->object = t.id;
    h->a = first;
    h->b = count;
    h->format = f;
}

void between_map_write(nshvc_map m, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const nshvc_map_entry *entries)
{
    Held *k;

    if (!in_update) {
        nshvc_map_write(m, x, y, w, h, entries);
        return;
    }
    k = hold(HOLD_MAP, entries, (size_t)w * h * sizeof *entries);
    k->object = m.id;
    k->a = x;
    k->b = y;
    k->c = w;
    k->d = h;
}

void between_map_write_snes(nshvc_map m, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *words)
{
    Held *k;

    shadow_map(m.id, x, y, w, h, words);
    if (!in_update) {
        nshvc_map_write_snes(m, x, y, w, h, words);
        return;
    }
    k = hold(HOLD_MAP_SNES, words, (size_t)w * h * sizeof *words);
    k->object = m.id;
    k->a = x;
    k->b = y;
    k->c = w;
    k->d = h;
}

void between_palette_write(uint32_t first, uint32_t count, const uint16_t *bgr15)
{
    Held *h;

    shadow_palette(first, count, bgr15);
    if (!in_update) {
        nshvc_palette_write(first, count, bgr15);
        return;
    }
    h = hold(HOLD_PALETTE, bgr15, (size_t)count * sizeof *bgr15);
    h->a = first;
    h->b = count;
}

/* ---- runahead: the real world saved while ahead steps run, and put back (system/runahead.c) ---- */

static GameState g_saved;
static void *kept_saved[KEPT_MAX];
static size_t kept_saved_bytes[KEPT_MAX];

void between_save(void)
{
    memcpy(&g_saved, &g, sizeof g);
    for (unsigned i = 0; i < kept_count; i++) {
        if (!kept_saved[i] || kept_saved_bytes[i] != kept[i].bytes) {
            kept_saved[i] = realloc(kept_saved[i], kept[i].bytes);
            kept_saved_bytes[i] = kept[i].bytes;
            if (!kept_saved[i]) {
                fprintf(stderr, "between: no memory for a saved part of %zu bytes\n", kept[i].bytes);
                exit(2);
            }
        }
        memcpy(kept_saved[i], kept[i].live, kept[i].bytes);
    }
}

/* a hash of what between_save kept (g and the kept parts): the developer's check that ahead steps leave the real world alone */
static void ptr_to_offset(Actor **at, size_t n)
{
    for (size_t i = 0; i < n; i++)
        at[i] = at[i] ? (Actor *)((uintptr_t)at[i] - (uintptr_t)&g) : (Actor *)~(uintptr_t)0;
}

uint64_t between_saved_hash(uint64_t h, int parts)
{
    static GameState flat;      /* g_saved with its pointers into g as offsets into g (the same in every run) */
    const uint8_t *p = (const uint8_t *)&flat;
    uint64_t one = 0xcbf29ce484222325ull;

    memcpy(&flat, &g_saved, sizeof flat);
    ptr_to_offset(&flat.chain_head, 1);
    ptr_to_offset(&flat.chain_tail, 1);
    ptr_to_offset(flat.free_pool, sizeof flat.free_pool / sizeof flat.free_pool[0]);
    ptr_to_offset(flat.free_pool_b, sizeof flat.free_pool_b / sizeof flat.free_pool_b[0]);
    ptr_to_offset(&flat.draw_list[0][0], sizeof flat.draw_list / sizeof flat.draw_list[0][0]);
    for (unsigned i = 0; i < ACTOR_RECORDS; i++) {
        ptr_to_offset(&flat.actors[i].next, 1);
        if (flat.actors[i].code)
            flat.actors[i].code = (int (*)(struct Actor *))((uintptr_t)flat.actors[i].code - (uintptr_t)&g);
    }
    /* g.line_data (the per-line tables) is left out: some of its bytes differ from run to run of one build with no runahead
       (level 1-1's play p1-1, from update 9089: bytes 9 to 11, 26 and 27 of it; the cause is not found), so the hash could not
       tell a leak from that (FRAMERATE.md 9.9) */
    memset(flat.line_data, 0, sizeof flat.line_data);
    for (unsigned i = 0; i < VRAM_QUEUE_SIZE; i++)
        if (flat.vram_queue[i].from)
            flat.vram_queue[i].from = (const uint8_t *)((uintptr_t)flat.vram_queue[i].from - (uintptr_t)&g);
    for (size_t i = 0; i < sizeof flat; i++)
        one = (one ^ p[i]) * 0x100000001b3ull;
    if (parts)
        fprintf(stderr, " g=%016llx", (unsigned long long)one);
    h = (h ^ one) * 0x100000001b3ull;
    for (unsigned k = 0; k < kept_count; k++) {
        if (kept[k].owned)
            continue;
        one = 0xcbf29ce484222325ull;
        p = kept_saved[k];
        for (size_t i = 0; i < kept_used(k); i++)
            one = (one ^ p[i]) * 0x100000001b3ull;
        if (parts)
            fprintf(stderr, " k%u=%016llx", k, (unsigned long long)one);
        h = (h ^ one) * 0x100000001b3ull;
    }
    return h;
}

/* g and the kept parts as between_save left them; the writes the ahead steps held for the engine are dropped (the
   engine's art is what the last ahead step's begin wrote: the real update's, see FRAMERATE.md 9.4) */
void between_restore(void)
{
    memcpy(&g, &g_saved, sizeof g);
    for (unsigned i = 0; i < kept_count; i++) {
        if (kept_saved_bytes[i] != kept[i].bytes) {
            fprintf(stderr, "between: a kept part changed size while the game ran ahead\n");
            exit(2);
        }
        memcpy(kept[i].live, kept_saved[i], kept[i].bytes);
    }
    held_count = 0;
    hold_used = 0;
}

/* ---- the update and the picture ---- */

void between_update_begin(void)
{
    write_held();               /* the engine's art is now the world the last update left ... */
    memcpy(&g_was, &g, sizeof g);   /* ... and so is "was" */
    for (unsigned i = 0; i < kept_count; i++)
        memcpy(kept[i].was, kept[i].live, kept[i].bytes);
    have_was = 1;
    in_update = 1;
}

void between_update_end(void)
{
    video_update_end();         /* what video holds to the update's end (Mode 7's art) */
    in_update = 0;
}

/* ALADDIN_DEV_BETWEEN_CHECK=1: before each picture, the easing checked at between 1 and 0.5 against the update before
   and the newest (scroll.c, video.c); the counts printed at the end */
static unsigned check_n[2], check_wrong[2];

static void check_report(void)
{
    fprintf(stderr, "between check: at 1, %u places, %u wrong; at 0.5, %u places, %u wrong\n", check_n[0], check_wrong[0],
            check_n[1], check_wrong[1]);
}

static void check(void)
{
    static int known, on;
    static const float at[2] = { 1.0f, 0.5f };

    if (!known) {
        known = 1;
        on = getenv("ALADDIN_DEV_BETWEEN_CHECK") != NULL;
        if (on)
            atexit(check_report);
    }
    if (!on)
        return;
    for (int k = 0; k < 2; k++) {
        amount = at[k];
        enter_was();
        drawing = 1;
        video_scroll_check(at[k], &check_n[k], &check_wrong[k]);
        video_sprites_check(at[k], &check_n[k], &check_wrong[k]);
        drawing = 0;
        leave_was();
    }
}

void between_draw(float between)
{
    if (have_was)
        check();
    (void)between;
    if (!have_was) {            /* before the first update: init's world, written at once */
        video_draw();
        return;
    }
    {   /* ALADDIN_DEV_BETWEEN_AT=F: every picture at between F (the developer's: pictures between updates headless) */
        static int known;
        static float at = -1;

        if (!known) {
            const char *v = getenv("ALADDIN_DEV_BETWEEN_AT");

            known = 1;
            if (v && *v)
                at = strtof(v, NULL);
        }
        if (at >= 0)
            between = at;
    }
    amount = between < 0 ? 0 : between > 1 ? 1 : between;
    enter_was();
    drawing = 1;
    video_draw();
    drawing = 0;
    leave_was();
}

/* ---- the snapshot's engine art and picture (system/snapshot.c) ---- */

/* The art as bytes: a count, then each shadowed object (name, kind, unit, units, bytes), then the palette's entries. */
size_t between_art_size(void)
{
    size_t n = 4;

    for (unsigned i = 0; i < shadow_count; i++)
        n += 24 + 12 + (size_t)shadows[i].units * shadows[i].unit;
    return n + 4 + sizeof palette_shadow;
}

static uint8_t *put32(uint8_t *o, uint32_t v)
{
    o[0] = (uint8_t)v; o[1] = (uint8_t)(v >> 8); o[2] = (uint8_t)(v >> 16); o[3] = (uint8_t)(v >> 24);
    return o + 4;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

size_t between_art_save(uint8_t *out)
{
    uint8_t *o = put32(out, shadow_count);

    for (unsigned i = 0; i < shadow_count; i++) {
        const Shadow *s = &shadows[i];
        size_t bytes = (size_t)s->units * s->unit;

        memcpy(o, s->name, 24);
        o = put32(o + 24, s->kind);
        o = put32(o, s->unit);
        o = put32(o, s->units);
        memcpy(o, s->data, bytes);
        o += bytes;
    }
    o = put32(o, PALETTE_ENTRIES);
    for (unsigned i = 0; i < PALETTE_ENTRIES; i++) {
        o[2 * i] = (uint8_t)palette_shadow[i];
        o[2 * i + 1] = (uint8_t)(palette_shadow[i] >> 8);
    }
    return (size_t)(o + 2u * PALETTE_ENTRIES - out);
}

/* the file's entry for a name, or NULL; checks the entry's shape against the shadow (1 when whole) */
static const uint8_t *art_entry(const uint8_t *in, size_t len, const char *name, const Shadow *s, size_t *at_end)
{
    size_t at = 4;
    uint32_t n = len >= 4 ? get32(in) : 0;

    for (uint32_t i = 0; i < n; i++) {
        uint32_t kind, unit, units;
        size_t bytes;

        if (at + 36 > len)
            return NULL;
        kind = get32(in + at + 24);
        unit = get32(in + at + 28);
        units = get32(in + at + 32);
        bytes = (size_t)unit * units;
        if (at + 36 + bytes > len)
            return NULL;
        if (!strncmp((const char *)in + at, name, 24)) {
            if (s && (kind != s->kind || unit != s->unit || units != s->units))
                return NULL;
            if (at_end)
                *at_end = at + 36 + bytes;
            return in + at + 36;
        }
        at += 36 + bytes;
    }
    if (at_end)
        *at_end = at;
    return NULL;
}

/* whether `in` holds every object this process shadows, each of the shape it has, and the palette; the slot sets (as many as the
   view had) may be more or fewer: a set the file lacks is zeroed, one only the file has is made by the view it brings */
int between_art_check(const uint8_t *in, size_t len)
{
    size_t end = 0;

    if (len < 4)
        return 0;
    for (unsigned i = 0; i < shadow_count; i++)
        if (strncmp(shadows[i].name, "slot_set.", 9) && !art_entry(in, len, shadows[i].name, &shadows[i], NULL))
            return 0;
    art_entry(in, len, "", NULL, &end);
    return end + 4 + 2u * PALETTE_ENTRIES <= len && get32(in + end) == PALETTE_ENTRIES;
}

/* the art put back whole: every shadowed object written from the file's bytes, the palette up to `palette_entries` */
void between_art_load(const uint8_t *in, size_t len, unsigned palette_entries)
{
    size_t end = 0;

    for (unsigned i = 0; i < shadow_count; i++) {
        Shadow *s = &shadows[i];
        const uint8_t *from = art_entry(in, len, s->name, s, NULL);

        if (from)
            memcpy(s->data, from, (size_t)s->units * s->unit);
        else
            memset(s->data, 0, (size_t)s->units * s->unit);
        if (s->kind == SH_TILES)
            nshvc_tiles_write((nshvc_tiles){ s->id }, 0, s->data, s->units, NSHVC_TILES_PLANAR);
        else
            nshvc_map_write_snes((nshvc_map){ s->id }, 0, 0, s->w, s->units / s->w, (const uint16_t *)(const void *)s->data);
    }
    art_entry(in, len, "", NULL, &end);
    for (unsigned i = 0; i < PALETTE_ENTRIES; i++)
        palette_shadow[i] = (uint16_t)(in[end + 4 + 2 * i] | in[end + 5 + 2 * i] << 8);
    if (palette_entries > PALETTE_ENTRIES)
        palette_entries = PALETTE_ENTRIES;
    nshvc_palette_write(0, palette_entries < 256 ? palette_entries : 256, palette_shadow);
    if (palette_entries > 256)
        nshvc_palette_write(256, palette_entries - 256, palette_shadow + 256);
}

/* the picture's "was" is the live world and nothing is held (after a load): the next picture shows the loaded world */
void between_rebase(void)
{
    held_count = 0;
    hold_used = 0;
    memcpy(&g_was, &g, sizeof g);
    for (unsigned i = 0; i < kept_count; i++)
        memcpy(kept[i].was, kept[i].live, kept[i].bytes);
    have_was = 1;
    in_update = 0;
}

/* the layout of what this file keeps, folded into a hash: the kept parts and the shadows */
uint64_t between_layout(uint64_t h)
{
#define FOLD(v) h = (h ^ (uint64_t)(v)) * 0x100000001b3ull
    for (unsigned i = 0; i < kept_count; i++) {
        for (const char *c = kept[i].name; *c; c++)
            FOLD((uint8_t)*c);
        FOLD(kept[i].owned);
        FOLD(kept[i].count ? kept[i].elem : kept[i].bytes);
    }
    for (unsigned i = 0; i < shadow_count; i++) {
        if (!strncmp(shadows[i].name, "slot_set.", 9))
            continue;                   /* how many there are follows the view */
        for (const char *c = shadows[i].name; *c; c++)
            FOLD((uint8_t)*c);
        FOLD(shadows[i].kind);
        FOLD(shadows[i].unit);
        FOLD(shadows[i].units);
    }
    FOLD(PALETTE_ENTRIES);
#undef FOLD
    return h;
}

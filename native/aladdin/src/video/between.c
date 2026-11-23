/*
 * between.c: pictures between updates (between.h).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/state.h"
#include "video/between.h"
#include "video/video.h"
#include "video/video_internal.h"

#define KEPT_MAX 32

/* ---- the kept state: g and the registered video parts ---- */

typedef struct Kept {
    void *live;
    void *was;
    size_t bytes;
    int unhashed;               /* the developer's state hash leaves it to its owner (it holds pointers) */
} Kept;

static Kept kept[KEPT_MAX];
static unsigned kept_count;
static GameState g_was, g_now;      /* g_now: the live state, put aside while a picture draws from "was" */
static void *kept_now[KEPT_MAX];
static int have_was;            /* an update has begun, so "was" is a world */
static int in_update;
static int drawing;             /* a picture is drawing from "was" */
static float amount;            /* its between */

void between_keep(void *p, size_t bytes)
{
    if (kept_count == KEPT_MAX) {
        fprintf(stderr, "between: more than %d kept parts\n", KEPT_MAX);
        exit(2);
    }
    kept[kept_count].live = p;
    kept[kept_count].was = calloc(1, bytes);
    kept[kept_count].bytes = bytes;
    kept_now[kept_count] = calloc(1, bytes);
    if (!kept[kept_count].was || !kept_now[kept_count]) {
        fprintf(stderr, "between: no memory for a kept part of %zu bytes\n", bytes);
        exit(2);
    }
    kept_count++;
}

void between_unhashed(const void *live)
{
    for (unsigned i = 0; i < kept_count; i++)
        if (kept[i].live == live)
            kept[i].unhashed = 1;
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
        if (kept[k].unhashed)
            continue;
        one = 0xcbf29ce484222325ull;
        p = kept_saved[k];
        for (size_t i = 0; i < kept[k].bytes; i++)
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

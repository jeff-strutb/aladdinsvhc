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

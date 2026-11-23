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
#define BRIGHTNESS_STEPS_MAX 2      /* a fade's steps are eased over up to this many updates (FRAMERATE.md, R4) */
#define WINDOW_STEPS_MAX 3          /* the windows' */

static uint64_t updates;            /* updates ended */
static int no_steps;                /* ALADDIN_DEV_NO_STEPS: every track eased over one update (R3's pictures) */

/* ---- the world: g and the registered video parts, kept at its steps ---- */

typedef struct Kept {
    void *live;
    void *was;                      /* at the world's step before its newest */
    void *now;                      /* at its newest step */
    void *aside;                    /* the live state, put aside while a picture draws from "was" */
    size_t bytes;
} Kept;

static Kept kept[KEPT_MAX];
static unsigned kept_count;
static GameState g_was, g_now, g_aside;
static int have_was;            /* an update has ended, so "was" is a world */
static int in_update;
static int drawing;             /* a picture is drawing from "was" */
static float amount;            /* how far the easing in hand goes: the world's while a picture draws */
static float between_drawn;     /* the picture's between */

static uint64_t world_at;       /* the update of the world's newest step */
static unsigned world_steps = 1;/* the updates it is eased over */
static int play_turned;         /* the play loop turned in this update (between_play_turn) */
static uint64_t last_turn;
static unsigned slow;           /* the slow motion's wait (g.play_wait at its last turn), 0 outside it */

static void *zalloc(size_t bytes)
{
    void *p = calloc(1, bytes);

    if (!p) {
        fprintf(stderr, "between: no memory for a kept part of %zu bytes\n", bytes);
        exit(2);
    }
    return p;
}

void between_keep(void *p, size_t bytes)
{
    if (kept_count == KEPT_MAX) {
        fprintf(stderr, "between: more than %d kept parts\n", KEPT_MAX);
        exit(2);
    }
    kept[kept_count].live = p;
    kept[kept_count].was = zalloc(bytes);
    kept[kept_count].now = zalloc(bytes);
    kept[kept_count].aside = zalloc(bytes);
    kept[kept_count].bytes = bytes;
    kept_count++;
}

static void *regrow(void *p, size_t old, size_t bytes)
{
    void *q = realloc(p, bytes);

    if (!q) {
        fprintf(stderr, "between: no memory for a kept part of %zu bytes\n", bytes);
        exit(2);
    }
    if (bytes > old)
        memset((uint8_t *)q + old, 0, bytes - old);
    return q;
}

void between_rekeep(void *old, void *p, size_t bytes)
{
    for (unsigned i = 0; i < kept_count; i++)
        if (kept[i].live == old) {
            kept[i].was = regrow(kept[i].was, kept[i].bytes, bytes);
            kept[i].now = regrow(kept[i].now, kept[i].bytes, bytes);
            kept[i].aside = regrow(kept[i].aside, kept[i].bytes, bytes);
            kept[i].live = p;
            kept[i].bytes = bytes;
            return;
        }
}

/* the live state put aside and "was" put in its place (a picture may change what it draws from, as easing does: the
   "was" copies stay as the world's step left them, for the next picture) */
static void enter_was(void)
{
    memcpy(&g_aside, &g, sizeof g);
    memcpy(&g, &g_was, sizeof g);
    for (unsigned i = 0; i < kept_count; i++) {
        memcpy(kept[i].aside, kept[i].live, kept[i].bytes);
        memcpy(kept[i].live, kept[i].was, kept[i].bytes);
    }
}

static void leave_was(void)
{
    memcpy(&g, &g_aside, sizeof g);
    for (unsigned i = 0; i < kept_count; i++)
        memcpy(kept[i].live, kept[i].aside, kept[i].bytes);
}

void between_play_turn(void)
{
    play_turned = 1;
}

/* ---- the brightness and the windows: tracks of their own ---- */

static int bright_was, bright_now;  /* the brightness before its newest step, and after */
static uint64_t bright_at;
static unsigned bright_steps;       /* 0: its newest change was not a fade's step (the world's own is shown) */

static GameState *win_was, *win_now;    /* the state at the windows' step before the newest, and at the newest */
static uint32_t win_hash;
static uint64_t win_at;
static unsigned win_steps = 1;

/* how far a track is from its step before to its newest, a picture `b` past the newest update: the updates since its
   newest step and b, over the updates it is eased over */
static float track_amount(uint64_t at, unsigned steps, float b)
{
    float t;

    if (!steps)
        return 1.0f;
    t = ((float)(updates - at) + b) / (float)steps;
    return t > 1.0f ? 1.0f : t < 0.0f ? 0.0f : t;
}

static void tracks_step(void)
{
    int b = g.brightness & 15;
    uint32_t h = video_lines_windows_hash();

    if (b != bright_now) {
        uint64_t gap = updates - bright_at;

        bright_steps = no_steps || (b != bright_now + 1 && b != bright_now - 1) ? 0
                       : gap < BRIGHTNESS_STEPS_MAX ? (unsigned)gap : BRIGHTNESS_STEPS_MAX;
        bright_was = bright_now;
        bright_now = b;
        bright_at = updates;
    }
    if (h != win_hash) {
        uint64_t gap = updates - win_at;

        win_steps = no_steps ? 1 : gap < WINDOW_STEPS_MAX ? (unsigned)gap : WINDOW_STEPS_MAX;
        memcpy(win_was, win_now, sizeof *win_now);
        memcpy(win_now, &g, sizeof g);
        win_hash = h;
        win_at = updates;
    }
}

static void tracks_start(void)
{
    win_was = zalloc(sizeof *win_was);
    win_now = zalloc(sizeof *win_now);
    memcpy(win_was, &g, sizeof g);
    memcpy(win_now, &g, sizeof g);
    win_hash = video_lines_windows_hash();
    bright_was = bright_now = g.brightness & 15;
}

uint32_t between_brightness(void)
{
    int v;

    if (!drawing || !bright_steps)
        return (uint32_t)(g.brightness & 15) << 8;
    v = bright_was * 256 +
        (int)floorf((float)((bright_now - bright_was) * 256) * track_amount(bright_at, bright_steps, between_drawn));
    return (uint32_t)v;
}

int between_windows(const GameState **was, const GameState **now, float *at)
{
    if (!drawing)
        return 0;
    *was = win_was;
    *now = win_now;
    *at = track_amount(win_at, win_steps, between_drawn);
    return 1;
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

float between_amount_set(float at)
{
    float was = amount;

    amount = at;
    return was;
}

const GameState *between_now(void)
{
    return drawing ? &g_now : &g;
}

const void *between_now_of(const void *live)
{
    for (unsigned i = 0; drawing && i < kept_count; i++)
        if (kept[i].live == live)
            return kept[i].now;
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
static size_t held_count, held_cap, held_to_step;  /* held_to_step: those made up to the world's newest step */
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

/* the first n writes held written, in their order; the rest kept */
static void write_held(size_t n)
{
    size_t done_bytes = n < held_count ? held[n].at : hold_used;

    for (size_t i = 0; i < n; i++) {
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
    for (size_t i = n; i < held_count; i++) {
        held[i - n] = held[i];
        held[i - n].at -= done_bytes;
    }
    if (hold_used > done_bytes)
        memmove(hold_data, hold_data + done_bytes, hold_used - done_bytes);
    held_count -= n;
    hold_used -= done_bytes;
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
    static int started;

    if (!started) {             /* init's world is the newest until the first update ends */
        started = 1;
        no_steps = getenv("ALADDIN_DEV_NO_STEPS") != NULL;
        memcpy(&g_now, &g, sizeof g);
        for (unsigned i = 0; i < kept_count; i++)
            memcpy(kept[i].now, kept[i].live, kept[i].bytes);
        tracks_start();
    }
    in_update = 1;
}

/* ALADDIN_DEV_STEPS=FILE: a line an update, for measuring how effects step: the update, level, brightness, play_wait,
   a hash of the window edges, a hash of the places (the actors' and the layers'), whether the world stepped, and the
   engine writes still held */
static void steps_log(int step)
{
    static FILE *f;
    static int known;
    uint32_t h = 2166136261u;

    if (!known) {
        const char *v = getenv("ALADDIN_DEV_STEPS");

        known = 1;
        if (v && *v)
            f = fopen(v, "w");
    }
    if (!f)
        return;
    for (int i = 0; i < ACTOR_RECORDS; i++) {
        h = (h ^ g.actors[i].x) * 16777619u;
        h = (h ^ g.actors[i].y) * 16777619u;
    }
    for (int i = 0; i < 3; i++) {
        h = (h ^ g.bg_x[i]) * 16777619u;
        h = (h ^ g.bg_y[i]) * 16777619u;
    }
    fprintf(f, "%llu %u %u %u %08x %08x %d %zu\n", (unsigned long long)nshvc_updates(), (unsigned)g.level,
            (unsigned)g.brightness, (unsigned)g.play_wait, video_lines_windows_hash(), h, step, held_count);
}

void between_update_end(void)
{
    int step = 1;

    video_update_end();         /* what video holds to the update's end (Mode 7's art) */
    updates++;
    /* the world steps every update, but in the chain scene's slow motion only when the play loop turns */
    if (play_turned) {
        last_turn = updates;
        slow = !no_steps && g.play_wait > 1 ? g.play_wait : 0;
    } else if (slow && updates - last_turn < slow) {
        step = 0;
    } else {
        slow = 0;
    }
    if (step) {
        uint64_t gap = updates - world_at;

        world_steps = play_turned && slow && gap <= slow ? (unsigned)gap : 1;
        world_at = updates;
        memcpy(&g_was, &g_now, sizeof g);
        memcpy(&g_now, &g, sizeof g);
        for (unsigned i = 0; i < kept_count; i++) {
            memcpy(kept[i].was, kept[i].now, kept[i].bytes);
            memcpy(kept[i].now, kept[i].live, kept[i].bytes);
        }
        /* the engine's art is the world "was" names: the writes made up to the step before this one */
        write_held(held_to_step);
        held_to_step = held_count;
        have_was = 1;
    }
    play_turned = 0;
    tracks_step();
    steps_log(step);
    in_update = 0;
}

/* ALADDIN_DEV_BETWEEN_CHECK=1: before each picture, the easing checked at 1 and 0.5 of the way from "was" to the newest
   (scroll.c, video.c); the counts printed at the end */
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

/*
 * ALADDIN_DEV_STEP_CHECK=FILE: the tracks checked across pictures. Before each picture, a sample of each track's eased
 * value at between 1 is kept, and the next update's at between 0 compared with it: the same, but where a step came
 * sooner than the updates its track was eased over (counted apart). The samples: the world's (the player's place and
 * BG1's, eased in 256ths), the brightness (in 256ths of a level), the windows (lines.c's hash of the eased edges). Each
 * update's samples at between 0, 0.25, 0.5 and 0.75 are written to FILE, a line an update: the update, the world's
 * steps, the brightness's, the windows', then the player's x, the brightness and the windows' hash at each. The counts
 * go to stderr at the end.
 */
#define WIN_SAMPLE (4 + 2 * 224)

typedef struct Sample {
    int64_t px, py, bx, by;
    uint32_t bright, win;               /* win: a hash of the window edges, for the file */
    uint32_t edges[WIN_SAMPLE];
    uint8_t far[WIN_SAMPLE];
} Sample;

static uint64_t step_counts[3][3];  /* world, brightness, windows: checked, jumps, jumps after an early step */

static void step_report(void)
{
    static const char *name[3] = { "world", "brightness", "windows" };

    for (int k = 0; k < 3; k++)
        fprintf(stderr, "step check: %s, %llu pictures, %llu jump, %llu jump after an early step\n", name[k],
                (unsigned long long)step_counts[k][0], (unsigned long long)step_counts[k][1],
                (unsigned long long)step_counts[k][2]);
}

static Sample sample_at(float b)
{
    static Sample s;
    float keep = amount, keep_b = between_drawn;

    between_drawn = b;
    amount = track_amount(world_at, world_steps, b);
    drawing = 1;
    s.px = between_ease_fx(g_was.actors[0].x, g_now.actors[0].x, 32, (int64_t)1 << 30);
    s.py = between_ease_fx(g_was.actors[0].y, g_now.actors[0].y, 32, (int64_t)1 << 30);
    s.bx = between_ease_fx(g_was.bg_x[0], g_now.bg_x[0], 32, (int64_t)1 << 30);
    s.by = between_ease_fx(g_was.bg_y[0], g_now.bg_y[0], 32, (int64_t)1 << 30);
    enter_was();
    s.bright = between_brightness();
    video_lines_windows_sample(s.edges, s.far);
    s.win = 2166136261u;
    for (int i = 0; i < WIN_SAMPLE; i++)
        s.win = (s.win ^ s.edges[i]) * 16777619u;
    leave_was();
    drawing = 0;
    amount = keep;
    between_drawn = keep_b;
    return s;
}

static void step_check(void)
{
    static FILE *f;
    static int known, have_last;
    static Sample last;
    static uint64_t last_u, prev_at[3];
    static unsigned prev_steps[3];
    static Sample q[4];

    if (!known) {
        const char *v = getenv("ALADDIN_DEV_STEP_CHECK");

        known = 1;
        if (v && *v) {
            f = fopen(v, "w");
            atexit(step_report);
        }
    }
    if (!f)
        return;
    for (int k = 0; k < 4; k++)
        q[k] = sample_at(0.25f * (float)k);
    if (have_last && updates == last_u + 1) {
        uint64_t at[3] = { world_at, bright_at, win_at };
        int jump[3] = { q[0].px != last.px || q[0].py != last.py || q[0].bx != last.bx || q[0].by != last.by,
                        q[0].bright != last.bright, 0 };

        /* the windows: a line that differs, but not one whose step was too far to ease (it shows when its step ends) nor
           one the channel starts or stops setting (a switch: the window effect starting or ending) */
        for (int i = 0; i < WIN_SAMPLE; i++)
            if (q[0].edges[i] != last.edges[i] && !last.far[i] && q[0].edges[i] != 0xFFFFFFFFu && last.edges[i] != 0xFFFFFFFFu)
                jump[2] = 1;

        for (int k = 0; k < 3; k++) {
            /* a track whose newest step is this update, sooner than the updates it was eased over, may jump */
            int early = at[k] == updates && prev_steps[k] && updates - prev_at[k] < prev_steps[k];

            step_counts[k][0]++;
            if (!jump[k])
                continue;
            step_counts[k][early ? 2 : 1]++;
            if (!early) {
                static int shown;

                if (shown++ < 20)
                    fprintf(stderr, "step check: %s jumps at update %llu\n",
                            k == 0 ? "world" : k == 1 ? "brightness" : "windows", (unsigned long long)updates);
            }
        }
    }
    prev_at[0] = world_at, prev_steps[0] = world_steps;
    prev_at[1] = bright_at, prev_steps[1] = bright_steps;
    prev_at[2] = win_at, prev_steps[2] = win_steps;
    fprintf(f, "%llu %u %u %u", (unsigned long long)updates, world_steps, bright_steps, win_steps);
    for (int k = 0; k < 4; k++)
        fprintf(f, " %lld %u %08x", (long long)q[k].px, q[k].bright, q[k].win);
    fprintf(f, "\n");
    last = sample_at(1.0f);
    have_last = 1;
    last_u = updates;
}

void between_draw(float between)
{
    if (!have_was) {            /* before the first update: init's world, written at once */
        video_draw();
        return;
    }
    check();
    step_check();
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
    between_drawn = between < 0 ? 0 : between > 1 ? 1 : between;
    amount = track_amount(world_at, world_steps, between_drawn);
    enter_was();
    drawing = 1;
    video_draw();
    drawing = 0;
    leave_was();
}

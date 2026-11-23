/*
 * runahead.c: ahead steps (runahead.h; docs/aladdin/native/FRAMERATE.md, section 9).
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

#include "system/runahead.h"
#include "audio/music.h"
#include "video/between.h"
#include "video/level_maps.h"

#define KEEP_MAX 64

typedef struct Keep {
    void *p;
    size_t bytes;
    const char *name;
    void *saved;
} Keep;

typedef struct Log {
    void *p;
    size_t bytes, at;
} Log;

static Keep keeps[KEEP_MAX];
static unsigned keep_count;
static Log *logs;
static size_t log_count, log_cap;
static uint8_t *log_data;
static size_t log_used, log_data_cap;

static unsigned ahead_steps;        /* the steps a tick takes ahead */
static int ahead;                   /* an ahead step is running */
static int aborted;                 /* the running ahead step reached something that must not run ahead */
static int live;                    /* the world the picture holds is an ahead world: the real one is saved */
static uint64_t ticks_ahead, ticks_aborted;
static int quiet_fd = -1, saved_err = -1;

void runahead_keep(void *p, size_t bytes, const char *name)
{
    if (keep_count == KEEP_MAX) {
        fprintf(stderr, "runahead: more than %d kept statics\n", KEEP_MAX);
        exit(2);
    }
    keeps[keep_count].p = p;
    keeps[keep_count].bytes = bytes;
    keeps[keep_count].name = name;
    keeps[keep_count].saved = NULL;
    keep_count++;
}

unsigned runahead_n(void) { return ahead_steps; }
unsigned runahead_keep_count(void) { return keep_count; }

void runahead_keep_at(unsigned i, const char **name, void **p, size_t *bytes)
{
    *name = keeps[i].name;
    *p = keeps[i].p;
    *bytes = keeps[i].bytes;
}

static int headless_run;
int runahead_headless(void) { return headless_run; }
int runahead_ahead(void) { return ahead; }

int runahead_guard(void)
{
    if (ahead)
        aborted = 1;
    return ahead;
}

void runahead_log(void *p, size_t bytes)
{
    if (!ahead)
        return;
    if (log_count == log_cap) {
        log_cap = log_cap ? 2 * log_cap : 256;
        logs = realloc(logs, log_cap * sizeof *logs);
    }
    if (log_used + bytes > log_data_cap) {
        while (log_used + bytes > log_data_cap)
            log_data_cap = log_data_cap ? 2 * log_data_cap : 1u << 16;
        log_data = realloc(log_data, log_data_cap);
    }
    if (!logs || !log_data) {
        fprintf(stderr, "runahead: no memory for the undo log\n");
        exit(2);
    }
    logs[log_count].p = p;
    logs[log_count].bytes = bytes;
    logs[log_count].at = log_used;
    memcpy(log_data + log_used, p, bytes);
    log_used += bytes;
    log_count++;
}

/* ALADDIN_DEV_RUNAHEAD_REPORT=1: how long each tick took (the update's work from runahead_begin to the end of runahead_after:
   the real step, the developer's checks, and the ahead steps), counted at the end */
#define TIME_MAX 400000
static float *tick_ms;
static unsigned tick_n;
static double tick_t0, real_ms_sum;
static int timing;

static double now_ms(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e3 + (double)t.tv_nsec * 1e-6;
}

static int cmp_float(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;

    return x < y ? -1 : x > y;
}

static void report(void)
{
    fprintf(stderr, "runahead: %" PRIu64 " ticks ahead by %u, %" PRIu64 " ended early (a step reached a load, the view's change or the sound's)\n",
            ticks_ahead, ahead_steps, ticks_aborted);
    if (tick_n) {
        double sum = 0;
        unsigned over = 0;

        for (unsigned i = 0; i < tick_n; i++) {
            sum += tick_ms[i];
            over += tick_ms[i] > 1000.0f / 60.0f;
        }
        qsort(tick_ms, tick_n, sizeof *tick_ms, cmp_float);
        fprintf(stderr, "runahead: %u ticks: mean %.3f ms (the real step alone %.3f), median %.3f, p99 %.3f, worst %.3f; %u over 16.7 ms\n",
                tick_n, sum / tick_n, real_ms_sum / tick_n, tick_ms[tick_n / 2], tick_ms[tick_n * 99 / 100], tick_ms[tick_n - 1], over);
    }
}

void runahead_init(int argc, char **argv)
{
    const char *v = getenv("ALADDIN_RUNAHEAD");
    int headless = 0;

    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--headless"))
            headless = headless_run = 1;
    if (v && *v)
        ahead_steps = (unsigned)strtoul(v, NULL, 0);
    else
        ahead_steps = headless ? 0 : 1;     /* on in a window, off in a headless run unless asked */
    /* the developer's tools that drive or log the steps themselves */
    if (getenv("ALADDIN_DEV_BOT") || getenv("ALADDIN_DEV_TRIGGERS") || getenv("ALADDIN_TASK_TRACE") ||
        getenv("ALADDIN_DEV_MAPDUMP") || getenv("ALADDIN_DEV_CELDUMP"))
        ahead_steps = 0;
    if (ahead_steps > 1) {
        /* the engine's art (tiles, maps, palette) written by a second ahead step would have to be put back too: not built */
        fprintf(stderr, "runahead: %u ahead steps asked; only 1 is built (FRAMERATE.md 9.6); using 1\n", ahead_steps);
        ahead_steps = 1;
    }
    if (getenv("ALADDIN_DEV_RUNAHEAD_REPORT")) {
        timing = 1;
        tick_ms = malloc(TIME_MAX * sizeof *tick_ms);
        atexit(report);
    }
}

/* the steps' stderr (a once-only note a step reaches first ahead) goes nowhere: it is the real step that tells */
static void quiet(int on)
{
#ifndef _WIN32
    if (on) {
        if (quiet_fd < 0)
            quiet_fd = open("/dev/null", O_WRONLY);
        if (quiet_fd >= 0) {
            fflush(stderr);
            saved_err = dup(2);
            dup2(quiet_fd, 2);
        }
    } else if (saved_err >= 0) {
        fflush(stderr);
        dup2(saved_err, 2);
        close(saved_err);
        saved_err = -1;
    }
#else
    (void)on;
#endif
}

static void save_statics(void)
{
    for (unsigned i = 0; i < keep_count; i++) {
        if (!keeps[i].saved) {
            keeps[i].saved = malloc(keeps[i].bytes);
            if (!keeps[i].saved) {
                fprintf(stderr, "runahead: no memory for a kept static of %zu bytes\n", keeps[i].bytes);
                exit(2);
            }
        }
        memcpy(keeps[i].saved, keeps[i].p, keeps[i].bytes);
    }
    log_count = 0;
    log_used = 0;
}

static void restore_statics(void)
{
    for (size_t i = log_count; i-- > 0;)
        memcpy(logs[i].p, log_data + logs[i].at, logs[i].bytes);
    log_count = 0;
    log_used = 0;
    for (unsigned i = 0; i < keep_count; i++)
        memcpy(keeps[i].p, keeps[i].saved, keeps[i].bytes);
}

static void put_back(void)
{
    between_restore();
    restore_statics();
    live = 0;
}

void runahead_begin(void)
{
    music_snapshot_commit();    /* a snapshot's sound state loaded in the update before goes to the driver now (music.h) */
    if (timing)
        tick_t0 = now_ms();
    if (live)
        put_back();
}

/* ALADDIN_DEV_STATE_HASH=1: a line a real update, the hash of the real world (g, the kept video parts and the statics)
   after its step; the same run with and without ahead steps must give the same lines */
static void dev_hash(void)
{
    static int parts = -1;
    uint64_t h = 0xcbf29ce484222325ull;

    if (parts < 0)
        parts = atoi(getenv("ALADDIN_DEV_STATE_HASH")) == 2;
    {   /* ALADDIN_DEV_KEPT_DUMP=U:FILE: at update U the kept video parts' used bytes, one after the other, to FILE (to find what differs) */
        const char *v = getenv("ALADDIN_DEV_KEPT_DUMP");

        if (v && strtoull(v, NULL, 0) == nshvc_updates() && strchr(v, ':')) {
            FILE *f = fopen(strchr(v, ':') + 1, "wb");

            for (unsigned i = 0; f && i < between_kept_count(); i++) {
                const char *name;
                void *live;
                size_t used, total;
                int owned;

                between_kept_at(i, &name, &live, &used, &total, &owned);
                if (!owned) {
                    fprintf(f ? stderr : stderr, "kept %u %s %zu\n", i, name, used);
                    fwrite(live, 1, used, f);
                }
            }
            if (f)
                fclose(f);
        }
    }
    fprintf(stderr, "hash u=%" PRIu64, nshvc_updates());
    h = between_saved_hash(h, parts);
    for (unsigned i = 0; i < keep_count; i++) {
        const uint8_t *p = keeps[i].p;
        uint64_t one = 0xcbf29ce484222325ull;

        for (size_t k = 0; k < keeps[i].bytes; k++)
            one = (one ^ p[k]) * 0x100000001b3ull;
        if (parts)
            fprintf(stderr, " s%u=%016" PRIx64, i, one);
        h = (h ^ one) * 0x100000001b3ull;
    }
    {
        uint64_t lm = level_maps_hash(0xcbf29ce484222325ull), mu = headless_run ? music_state_hash(0xcbf29ce484222325ull) : 0;

        if (parts)
            fprintf(stderr, " maps=%016" PRIx64 " sound=%016" PRIx64, lm, mu);
        h = (h ^ lm) * 0x100000001b3ull;
        h = (h ^ mu) * 0x100000001b3ull;
    }
    fprintf(stderr, " %016" PRIx64 "\n", h);
}

static void runahead_ahead_steps(const nshvc_input *input, void (*step)(const nshvc_input *));

void runahead_after(const nshvc_input *input, void (*step)(const nshvc_input *))
{
    if (timing)
        real_ms_sum += now_ms() - tick_t0;
    runahead_ahead_steps(input, step);
    if (timing && tick_n < TIME_MAX)
        tick_ms[tick_n++] = (float)(now_ms() - tick_t0);
}

static void runahead_ahead_steps(const nshvc_input *input, void (*step)(const nshvc_input *))
{
    static int hash_on = -1;

    if (hash_on < 0)
        hash_on = getenv("ALADDIN_DEV_STATE_HASH") != NULL;
    if (!ahead_steps && !hash_on)
        return;
    between_save();
    save_statics();
    if (hash_on)
        dev_hash();
    if (!ahead_steps)
        return;
    ahead = 1;
    aborted = 0;
    quiet(1);
    for (unsigned i = 0; i < ahead_steps && !aborted; i++) {
        between_update_begin();
        step(input);
        between_update_end();
    }
    quiet(0);
    ahead = 0;
    ticks_ahead++;
    if (aborted) {
        /* the step reached what must not run ahead: the real world is back and the picture holds it (the update
           before's world and the newest are both the real one: between_update_begin made "was" from it) */
        ticks_aborted++;
        put_back();
    } else {
        live = 1;
    }
}

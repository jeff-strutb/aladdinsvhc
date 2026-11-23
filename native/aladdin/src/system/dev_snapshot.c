/*
 * dev_snapshot.c: the developer's save and load of a snapshot by update number (docs/aladdin/native/CHECK.md, "The snapshot").
 *
 * ALADDIN_DEV_SNAP_SAVE=FILE@U: at the start of update U the snapshot (the world after update U - 1) is written to FILE.
 * ALADDIN_DEV_SNAP_LOAD=FILE@L: update L loads FILE and runs no step; the update after it is the one after the save.
 * Both may be set (a save, and later in the same run a load: the round trip in one process).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "system/snapshot.h"
#include "system/system.h"

static double ms(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e3 + (double)t.tv_nsec * 1e-6;
}

static int parse(const char *name, char *file, size_t cap, uint64_t *at)
{
    const char *v = getenv(name), *sep;

    if (!v || !(sep = strrchr(v, '@')) || (size_t)(sep - v) >= cap)
        return 0;
    memcpy(file, v, (size_t)(sep - v));
    file[sep - v] = 0;
    *at = strtoull(sep + 1, NULL, 0);
    return 1;
}

void DevSnapshotSave(uint64_t u)
{
    static int known, on;
    static char file[1024];
    static uint64_t at;
    uint8_t *buf;
    size_t n;
    FILE *f;

    if (!known) {
        known = 1;
        on = parse("ALADDIN_DEV_SNAP_SAVE", file, sizeof file, &at);
    }
    if (!on || u != at)
        return;
    double t0 = ms();

    buf = malloc(snapshot_max());
    if (!buf || !(n = snapshot_save(buf, snapshot_max())) || !(f = fopen(file, "wb"))) {
        fprintf(stderr, "snapshot: the save at update %llu failed\n", (unsigned long long)u);
        exit(3);
    }
    fwrite(buf, 1, n, f);
    fclose(f);
    fprintf(stderr, "snapshot: %zu bytes at update %llu (the buffer %zu; %.3f ms to save)\n", n, (unsigned long long)u, snapshot_max(), ms() - t0);
    free(buf);
}

int DevSnapshotLoad(uint64_t u)
{
    static int known, on;
    static char file[1024];
    static uint64_t at;
    uint8_t *buf;
    size_t n;
    FILE *f;

    if (!known) {
        known = 1;
        on = parse("ALADDIN_DEV_SNAP_LOAD", file, sizeof file, &at);
    }
    if (!on || u != at)
        return 0;
    double t0 = ms();

    buf = malloc(snapshot_max() + 1);
    if (!buf || !(f = fopen(file, "rb"))) {
        fprintf(stderr, "snapshot: cannot read %s\n", file);
        exit(3);
    }
    n = fread(buf, 1, snapshot_max() + 1, f);
    fclose(f);
    t0 = ms();
    if (!snapshot_load(buf, n)) {
        fprintf(stderr, "snapshot: the load at update %llu failed\n", (unsigned long long)u);
        exit(3);
    }
    fprintf(stderr, "snapshot: loaded %zu bytes at update %llu (%.3f ms to load)\n", n, (unsigned long long)u, ms() - t0);
    free(buf);
    return 1;
}

/*
 * music.c: the music code on the audio thread. It runs once a DSP sample,
 * takes the game's messages as they come (the original's driver polled its
 * ports between ticks), and steps the driver every 256 samples, the period
 * of the original's timer 0 at $40 (64 counts of 128 SPC700 cycles, a DSP
 * sample every 32). It never waits, locks, allocates or reads files.
 */
#include <stdatomic.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "nshvc/nshvc.h"
#include "audio/music.h"
#include "system/runahead.h"

#define TICK_SAMPLES 256

static uint8_t message[MUSIC_MESSAGE_MAX];
static void snap_unpack(const uint8_t *in);
static uint32_t samples;

void music_take(const uint8_t *m, uint32_t size)
{
    switch (m[0]) {
    case MUSIC_START: {
        MusicStart s;

        if (size != sizeof s)
            return;
        memcpy(&s, m, sizeof s);
        driver_start(s.data);
        samples = 0;
        break;
    }
    case MUSIC_COMMAND: {
        MusicCommand c;

        if (size != sizeof c)
            return;
        memcpy(&c, m, sizeof c);
        driver_command(c.command, c.fade_speed, c.offset);
        break;
    }
    case MUSIC_RESTORE: {
        static _Alignas(16) uint8_t stage[MUSIC_SNAPSHOT_BYTES];
        uint32_t at;

        if (size < 6)
            return;
        at = (uint32_t)m[2] | (uint32_t)m[3] << 8 | (uint32_t)m[4] << 16 | (uint32_t)m[5] << 24;
        if ((uint64_t)at + (size - 6) > sizeof stage)
            return;
        memcpy(stage + at, m + 6, size - 6);
        if (m[1])
            snap_unpack(stage);
        break;
    }
    case MUSIC_UPLOAD: {
        uint32_t at = 1;

        driver_command(0xF9, 0, 0);
        while (at + 4 <= size) {
            uint16_t to = (uint16_t)(m[at] | m[at + 1] << 8), length = (uint16_t)(m[at + 2] | m[at + 3] << 8);

            at += 4;
            if (at + length > size)
                return;
            driver_song(to, m + at, length);
            at += length;
        }
        break;
    }
    }
}

/* ---- the snapshot: the driver's state to the game thread and back (music.h) ----
   The engine hands the game nothing from the audio thread, so the game asks by a flag the audio code answers at its next sample,
   after it has taken every message the game sent before. Headless there is one thread and the state is read where it lies. */

enum { SNAP_IDLE, SNAP_SAVE, SNAP_SAVED };

static atomic_int snap_state;
static _Alignas(16) uint8_t snap_stage[MUSIC_SNAPSHOT_BYTES];

static void snap_pack(uint8_t *out)
{
    Driver *c = (Driver *)(void *)out;      /* aligned: the stage is */

    memcpy(c, &d, sizeof d);
    memset(c->source, 0, sizeof c->source); /* the engine's handles are this process's */
    memcpy(out + sizeof d, &samples, 4);
}

static void snap_unpack(const uint8_t *in)
{
    nshvc_sample keep[DRIVER_SOURCES];

    memcpy(keep, d.source, sizeof keep);
    memcpy(&d, in, sizeof d);
    memcpy(d.source, keep, sizeof keep);
    memcpy(&samples, in + sizeof d, 4);
    if (d.ready)
        driver_reapply();
}

static void snap_wait(void)
{
    for (int i = 0; i < 2000; i++) {        /* about two seconds */
        if (atomic_load(&snap_state) == SNAP_SAVED || atomic_load(&snap_state) == SNAP_IDLE)
            return;
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }
}

/* the developer's state hash (headless only, one thread): the driver's state with the engine's handles left out, and the tick's phase */
uint64_t music_state_hash(uint64_t h)
{
    static _Alignas(16) uint8_t copy[MUSIC_SNAPSHOT_BYTES];

    snap_pack(copy);
    for (size_t i = 0; i < sizeof copy; i++)
        h = (h ^ copy[i]) * 0x100000001b3ull;
    return h;
}

int music_snapshot_save(uint8_t *out)
{
    if (!d.ready)
        return 0;
    if (runahead_headless()) {
        snap_pack(snap_stage);
    } else {
        atomic_store(&snap_state, SNAP_SAVE);
        snap_wait();
        if (atomic_load(&snap_state) != SNAP_SAVED) {
            atomic_store(&snap_state, SNAP_IDLE);
            return 0;
        }
    }
    memcpy(out, snap_stage, MUSIC_SNAPSHOT_BYTES);
    atomic_store(&snap_state, SNAP_IDLE);
    return 1;
}

static uint8_t pending[MUSIC_SNAPSHOT_BYTES];
static int pending_on;

int music_snapshot_load(const uint8_t *in)
{
    if (!d.ready)
        return 0;
    memcpy(pending, in, sizeof pending);
    pending_on = 1;
    return 1;
}

void music_snapshot_commit(void)
{
    uint8_t part[MUSIC_MESSAGE_MAX];
    enum { PART = 8000 };

    if (!pending_on)
        return;
    pending_on = 0;
    for (uint32_t at = 0; at < sizeof pending; at += PART) {
        uint32_t n = sizeof pending - at < PART ? (uint32_t)sizeof pending - at : PART;

        part[0] = MUSIC_RESTORE;
        part[1] = at + n == sizeof pending;
        part[2] = (uint8_t)at; part[3] = (uint8_t)(at >> 8); part[4] = (uint8_t)(at >> 16); part[5] = (uint8_t)(at >> 24);
        memcpy(part + 6, pending + at, n);
        nshvc_music_send(part, 6 + n);
    }
}

void music_tick(void)
{
    uint32_t n;
    int s;

    while ((n = nshvc_music_receive(message, sizeof message)) != 0)
        music_take(message, n);
    s = atomic_load(&snap_state);
    if (s == SNAP_SAVE) {
        while ((n = nshvc_music_receive(message, sizeof message)) != 0)
            music_take(message, n);
        snap_pack(snap_stage);
        atomic_store(&snap_state, SNAP_SAVED);
    }
    if (!d.ready)
        return;
    if (++samples == TICK_SAMPLES) {
        samples = 0;
        driver_tick();
    }
}

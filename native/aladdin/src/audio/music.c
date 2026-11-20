/*
 * music.c: the music code on the audio thread. It runs once a DSP sample,
 * takes the game's messages as they come (the original's driver polled its
 * ports between ticks), and steps the driver every 256 samples, the period
 * of the original's timer 0 at $40 (64 counts of 128 SPC700 cycles, a DSP
 * sample every 32). It never waits, locks, allocates or reads files.
 */
#include <string.h>

#include "nshvc/nshvc.h"
#include "audio/music.h"

#define TICK_SAMPLES 256

static uint8_t message[MUSIC_MESSAGE_MAX];
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

void music_tick(void)
{
    uint32_t n;

    while ((n = nshvc_music_receive(message, sizeof message)) != 0)
        music_take(message, n);
    if (!d.ready)
        return;
    if (++samples == TICK_SAMPLES) {
        samples = 0;
        driver_tick();
    }
}

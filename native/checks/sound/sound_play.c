/*
 * sound_play.c: the native sound driver on the engine, played to the
 * original's schedule, for comparing the two sounds.
 *
 *   sound_play --apu APU.BIN --headless --updates N --sound OUT.RAW
 *
 * cart.dat is read from beside the program, as the game's.
 * A game with no picture whose music code is the native driver. From the
 * original's run (APU.BIN, as sound_check reads it) it takes the DSP sample at
 * which the original's driver started, finished each command and began each
 * tick, and does the same at the same sample: driver_start, the command
 * (a song's upload as SongPlay's blocks, a fade with the original's speed),
 * driver_tick. Samples are counted from power-on on both sides, so OUT.RAW
 * lines up with the original's --sound sample for sample. What still differs
 * is when inside a tick each voice change lands (the original's are spread
 * over the tick's SPC700 time, the native's all come at its first sample),
 * and the engine's mixer against bsnes's DSP.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "audio/driver.h"
#include "audio/music.h"
#include "audio/sound.h"
#include "data/data.h"

enum { EV_START, EV_COMMAND, EV_SONG, EV_TICK };
typedef struct { uint32_t sample; uint8_t kind, command; uint16_t speed; uint8_t list; } Event;

static Event *ev;
static uint32_t nev, at, samples;
static const char *apu_path;
static uint8_t message[MUSIC_MESSAGE_MAX];

static int song_received(const uint8_t *ram)
{
    unsigned list;
    for (list = 16; list < 55; list++) {
        SoundBlock b;
        if (sound_list_blocks(list) != 1)
            continue;
        b = sound_list_block(list, 0);
        if (b.to == DATA_SONG_BLOCKS && !memcmp(&ram[b.to], b.bytes, b.length))
            return (int)list;
    }
    return -1;
}

static void add(uint32_t s, uint8_t kind, uint8_t command, uint16_t speed, uint8_t list)
{
    static uint32_t room;
    if (nev == room) {
        room = room ? room * 2 : 4096;
        ev = (Event *)realloc(ev, room * sizeof *ev);
        if (!ev)
            exit(70);
    }
    ev[nev].sample = s; ev[nev].kind = kind; ev[nev].command = command; ev[nev].speed = speed; ev[nev].list = list;
    nev++;
}

/* the schedule, read on the game thread */
static void schedule(void)
{
    static uint8_t ram[0x10000];
    FILE *f = fopen(apu_path, "rb");
    uint8_t r[8];
    int started = 0, in_command = 0, tick_due = 0;

    if (!f || fread(ram, 1, sizeof ram, f) != sizeof ram) {
        fprintf(stderr, "sound_play: cannot read %s\n", apu_path);
        exit(2);
    }
    while (fread(r, 1, 8, f) == 8) {
        uint32_t s = (uint32_t)(r[0] | r[1] << 8 | r[2] << 16 | (uint32_t)r[3] << 24);
        uint16_t a = (uint16_t)(r[5] | r[6] << 8);
        uint8_t v = r[7];

        if (a == 0x00B1 && v)
            tick_due = 1;
        if (started && a == 0x00B0 && !in_command && tick_due) {
            tick_due = 0;
            add(s, EV_TICK, 0, 0, 0);
        }
        ram[a] = v;
        if (a == 0x00FA && v == 0x40) {
            add(s, EV_START, 0, 0, 0);
            started = 1;
            in_command = 0;
            continue;
        }
        if (!started)
            continue;
        if (a == 0x00B2)
            in_command = 1;
        if (a == 0x00F7) {
            in_command = 0;
            if (!v)
                continue;
            if (v == 0xFA) {
                int list = song_received(ram);
                if (list < 0) {
                    fprintf(stderr, "sound_play: a song upload at sample %u matches no song's list\n", s);
                    exit(1);
                }
                add(s, EV_SONG, v, 0, (uint8_t)list);
            } else {
                add(s, EV_COMMAND, v, (v == 0xF5 || v == 0xF6) ? (uint16_t)(ram[0xC7] | ram[0xC8] << 8) : 0, 0);
            }
        }
    }
    fclose(f);
}

/* the music code: each event at its sample, before that sample is made */
static void tick(void)
{
    while (at < nev && ev[at].sample <= samples) {
        const Event *e = &ev[at++];
        uint32_t n;
        switch (e->kind) {
        case EV_START:
            /* START from SoundSetsLoad; its two uploads are the original's own $F9s */
            while ((n = nshvc_music_receive(message, sizeof message)) != 0)
                if (message[0] == MUSIC_START)
                    music_take(message, n);
            break;
        case EV_COMMAND:
            driver_command(e->command, e->speed, 0);
            break;
        case EV_SONG: {
            int k, nb = sound_list_blocks(e->list);
            driver_command(0xF9, 0, 0);
            for (k = 0; k < nb; k++) {
                SoundBlock b = sound_list_block(e->list, (unsigned)k);
                driver_song(b.to, b.bytes, b.length);
            }
            driver_command(0xFB, 0, 0);
            break;
        }
        case EV_TICK:
            driver_tick();
            break;
        }
    }
    samples++;
}

static void init(void)
{
    data_load();
    schedule();
    printf("sound_play: %u events, the last at sample %u\n", nev, nev ? ev[nev - 1].sample : 0);
}

/* the engine's music queue exists from the first update on (not in init) */
static void update(const nshvc_input *in)
{
    (void)in;
    if (nshvc_updates() == 0)
        SoundSetsLoad();
}
static void draw(float between) { (void)between; }

int main(int argc, char **argv)
{
    static nshvc_game g;
    int i;
    for (i = 1; i + 1 < argc; i++) {
        if (!strcmp(argv[i], "--apu"))
            apu_path = argv[i + 1];
    }
    if (!apu_path) {
        fprintf(stderr, "usage: sound_play --apu APU.BIN --headless --updates N --sound OUT.RAW\n");
        return 2;
    }
    g.title = "Sound play";
    g.id = "sound-play";
    g.update_hz = 60;
    g.init = init;
    g.update = update;
    g.draw = draw;
    g.music.hz = MUSIC_HZ;
    g.music.tick = tick;
    g.music.queue_bytes = MUSIC_QUEUE_BYTES;
    return nshvc_run(&g, argc, argv);
}

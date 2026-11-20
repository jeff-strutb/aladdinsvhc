/*
 * sound.c: the game's side of the sound (sound.h), on the game thread.
 *
 * The original waited, before each command, for the driver to echo the last
 * one's sequence number, and moved songs and sample sets over by the IPL's
 * handshake, holding the commands meanwhile. Here a command reaches the
 * driver's queue at once and a load is one message, so the waits and the
 * hold have no counterpart; the order of everything sent is kept.
 */
#include <string.h>

#include "nshvc/nshvc.h"
#include "audio/music.h"
#include "audio/sound.h"
#include "data/data.h"
#include "system/state.h"

#define QUEUE 32
#define LEVEL_QUIET_FIRST 0x0014
#define LEVEL_QUIET_END 0x001F
#define LIST_DRIVER 0
#define LIST_EFFECTS 1
#define LIST_SAMPLES 2
#define LIST_BLOCKS_MAX 8
#define BRR_BLOCK 9

static uint8_t sets_loaded;
static SoundData data;          /* the driver's from MUSIC_START on; not written again */

void SoundQueueAdd(uint8_t command)
{
    if (!command || (g.level >= LEVEL_QUIET_FIRST && g.level < LEVEL_QUIET_END))
        return;
    g.sound_queue[g.sound_queue_write % QUEUE] = command;
    g.sound_queue_write = (uint8_t)((g.sound_queue_write + 1) % QUEUE);
}

void SoundQueueF0(void) { SoundQueueAdd(SOUND_STOP_ALL); }
void SoundQueueF1(void) { SoundQueueAdd(SOUND_EFFECTS_STOP); }
void SoundQueueF2(void) { SoundQueueAdd(SOUND_MUSIC_STOP); }

void SoundQueueF5WithA(uint8_t speed)
{
    SoundQueueAdd(SOUND_FADE_OUT);
    SoundQueueAdd(speed);
}

void SoundQueueF5With4(void) { SoundQueueF5WithA(4); }

void SoundQueueF6(uint8_t speed)
{
    SoundQueueAdd(SOUND_FADE_IN);
    SoundQueueAdd(speed);
}

void SoundQueueF6With1(void) { SoundQueueF6(1); }

static void send_command(uint8_t command, uint16_t fade_speed)
{
    MusicCommand c;

    memset(&c, 0, sizeof c);
    c.kind = MUSIC_COMMAND;
    c.command = command;
    c.fade_speed = fade_speed;
    nshvc_music_send(&c, sizeof c);
}

/* $F5 and $F6 carry the next byte as their speed's high byte (the
   original's port 3, its port 2 0). */
void SoundCommandSend(void)
{
    uint8_t command;
    uint16_t speed = 0;

    if (g.sound_queue_read == g.sound_queue_write)
        return;
    command = g.sound_queue[g.sound_queue_read % QUEUE];
    if (command == SOUND_FADE_OUT || command == SOUND_FADE_IN) {
        g.sound_queue_read = (uint8_t)((g.sound_queue_read + 1) % QUEUE);
        speed = (uint16_t)(g.sound_queue[g.sound_queue_read] << 8);
    }
    send_command(command, speed);
    g.sound_queue_read = (uint8_t)((g.sound_queue_read + 1) % QUEUE);
}

/* list `list`'s blocks into the sound data */
static void list_into(unsigned list)
{
    int n = sound_list_blocks(list), i;

    for (i = 0; i < n; i++) {
        SoundBlock b = sound_list_block(list, (unsigned)i);
        uint32_t k;

        for (k = 0; k < b.length; k++)
            data.bytes[(uint16_t)(b.to + k)] = b.bytes[k];
    }
}

/* Each entry of the sample directory as an engine sample: its BRR blocks from
   its start to the block with the end flag, looping at its loop point (the
   directory's entries all loop inside their own sample). Entries that share
   a start and loop share the sample. */
static void samples_load(void)
{
    int i, j;

    for (i = 0; i < DRIVER_SOURCES; i++) {
        uint16_t at = (uint16_t)(DATA_DIRECTORY + i * 4);
        uint16_t start = (uint16_t)(data.bytes[at] | data.bytes[at + 1] << 8);
        uint16_t loop = (uint16_t)(data.bytes[at + 2] | data.bytes[at + 3] << 8);
        uint32_t end = start;

        for (j = 0; j < i; j++) {
            uint16_t o = (uint16_t)(DATA_DIRECTORY + j * 4);
            if ((data.bytes[o] | data.bytes[o + 1] << 8) == start && (data.bytes[o + 2] | data.bytes[o + 3] << 8) == loop)
                break;
        }
        if (j < i) {
            data.source[i] = data.source[j];
            continue;
        }
        while (end + BRR_BLOCK <= 0x10000u) {
            uint8_t header = data.bytes[end];

            end += BRR_BLOCK;
            if (header & 1)
                break;
        }
        data.source[i] = nshvc_sample_load(&data.bytes[start], end - start, NSHVC_SAMPLE_BRR,
                                           loop >= start ? (uint32_t)(loop - start) : 0);
    }
}

static void send_upload(const SoundBlock *b, int n)
{
    static uint8_t m[MUSIC_MESSAGE_MAX];
    uint32_t size = 1;
    int i;

    m[0] = MUSIC_UPLOAD;
    for (i = 0; i < n; i++) {
        if (size + 4 + b[i].length > sizeof m)
            break;
        m[size] = (uint8_t)b[i].to;
        m[size + 1] = (uint8_t)(b[i].to >> 8);
        m[size + 2] = (uint8_t)b[i].length;
        m[size + 3] = (uint8_t)(b[i].length >> 8);
        memcpy(m + size + 4, b[i].bytes, b[i].length);
        size += 4 + b[i].length;
    }
    nshvc_music_send(m, size);
}

/* The original's lists 0, 3 and 6: the driver started, then each set's
   upload, which stops everything first. */
void SoundSetsLoad(void)
{
    MusicStart s;

    if (sets_loaded)
        return;
    sets_loaded = 1;
    memset(&data, 0, sizeof data);
    list_into(LIST_DRIVER);
    list_into(LIST_EFFECTS);
    list_into(LIST_SAMPLES);
    samples_load();
    memset(&s, 0, sizeof s);
    s.kind = MUSIC_START;
    s.data = &data;
    nshvc_music_send(&s, sizeof s);
    send_upload(NULL, 0);
    send_upload(NULL, 0);
}

/* The data and samples SoundSetsLoad made (for the checks). */
const SoundData *sound_data(void)
{
    return &data;
}

/* The song's list (its number, the original's index times 3 taken in a
   byte) loaded, then the song from its start: the original's command $FA. */
void SongPlay(uint8_t song)
{
    SoundBlock b[LIST_BLOCKS_MAX];
    unsigned list = (uint8_t)(song * 3) / 3u;
    int n = sound_list_blocks(list), i;

    if (n < 0)
        n = 0;
    if (n > LIST_BLOCKS_MAX)
        n = LIST_BLOCKS_MAX;
    for (i = 0; i < n; i++)
        b[i] = sound_list_block(list, (unsigned)i);
    send_upload(b, n);
    send_command(SOUND_SONG_AGAIN, 0);
}

/*
 * music.h: the messages from the game thread to the music code, and the
 * music code's entry (nshvc_music.tick, at the DSP's 32,040 Hz).
 */
#ifndef ALADDIN_MUSIC_H
#define ALADDIN_MUSIC_H

#include <stdint.h>

#include "audio/driver.h"

#define MUSIC_HZ 32040
#define MUSIC_QUEUE_BYTES 262144u   /* room for a snapshot's sound state (MUSIC_RESTORE, about 66 KB) besides the commands */
#define MUSIC_MESSAGE_MAX 8192u

enum {
    MUSIC_START = 1,    /* the driver starts on this data, kept by the game thread unchanged from then on */
    MUSIC_COMMAND,      /* a command */
    MUSIC_UPLOAD,       /* blocks into the sound data: at, length (16 bits each, low first), the bytes; repeated */
    MUSIC_RESTORE       /* a snapshot's sound state in parts: kind, last (1 on the last part), at (32 bits), the bytes */
};

typedef struct {
    uint8_t kind;
    const SoundData *data;
} MusicStart;

typedef struct {
    uint8_t kind;
    uint8_t command;
    int8_t offset;
    uint16_t fade_speed;
} MusicCommand;

void music_tick(void);

/* the snapshot (system/snapshot.c): the driver's state, `MUSIC_SNAPSHOT_BYTES` of it (the Driver and the tick phase, the
   engine's sample handles left out); save: 1 on success; load: the state is put in at the audio code's next sample and every
   voice is given its registers again (1 when posted) */
#define MUSIC_SNAPSHOT_BYTES (sizeof(Driver) + 4)
uint64_t music_state_hash(uint64_t h);   /* headless: FNV-1a of the driver's state (the developer's state hash) */
int music_snapshot_save(uint8_t *out);
/* the state is kept and sent (as messages, in order with the commands) by music_snapshot_commit, which runs at the start of the
   next update: so the driver is put in between the messages of the updates before and those of the update after, and the song
   goes on from its place as if the update after the save came next. 1 when the driver is running */
int music_snapshot_load(const uint8_t *in);
void music_snapshot_commit(void);
void music_take(const uint8_t *message, uint32_t size);    /* one message, as music_tick takes it */

#endif

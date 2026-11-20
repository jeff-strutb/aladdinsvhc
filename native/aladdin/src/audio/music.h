/*
 * music.h: the messages from the game thread to the music code, and the
 * music code's entry (nshvc_music.tick, at the DSP's 32,040 Hz).
 */
#ifndef ALADDIN_MUSIC_H
#define ALADDIN_MUSIC_H

#include <stdint.h>

#include "audio/driver.h"

#define MUSIC_HZ 32040
#define MUSIC_QUEUE_BYTES 65536u
#define MUSIC_MESSAGE_MAX 8192u

enum {
    MUSIC_START = 1,    /* the driver starts on this data, kept by the game thread unchanged from then on */
    MUSIC_COMMAND,      /* a command */
    MUSIC_UPLOAD        /* blocks into the sound data: at, length (16 bits each, low first), the bytes; repeated */
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
void music_take(const uint8_t *message, uint32_t size);    /* one message, as music_tick takes it */

#endif

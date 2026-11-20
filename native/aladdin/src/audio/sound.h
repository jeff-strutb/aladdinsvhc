/*
 * sound.h: the game's sound, as the rest of the game calls it (game thread).
 * These take the place of the original's APU port writes.
 *
 * Commands queue in a 32-entry ring, as the original's did, and one goes to
 * the driver an update (SoundCommandSend, the NMI's send). A command byte is:
 *   $00-$EF  a sound: the effect bank's entry n (taken off its 81 entries
 *            until below); most are effects, an entry of priority 0 is a
 *            song in the bank
 *   $F0      stop everything          $F1  stop the effects
 *   $F2      stop the music           $F3  pause the music
 *   $F4      the music goes on        $F5  fade the music out (a speed follows)
 *   $F6      fade the music in (a speed follows)
 *   $F7      stereo                   $F8  mono
 *   $FB      the loaded song again from its start
 *   $FC, $FD the music's tempo offset and detune, both 0 as the game sends them
 * The songs themselves ($10-$36) load with SongPlay.
 */
#ifndef ALADDIN_SOUND_H
#define ALADDIN_SOUND_H

#include <stdint.h>

#include "audio/driver.h"

enum {
    SOUND_STOP_ALL = 0xF0,
    SOUND_EFFECTS_STOP = 0xF1,
    SOUND_MUSIC_STOP = 0xF2,
    SOUND_MUSIC_PAUSE = 0xF3,
    SOUND_MUSIC_GO_ON = 0xF4,
    SOUND_FADE_OUT = 0xF5,
    SOUND_FADE_IN = 0xF6,
    SOUND_STEREO = 0xF7,
    SOUND_MONO = 0xF8,
    SOUND_SONG_AGAIN = 0xFB,
    SOUND_TEMPO_OFFSET = 0xFC,
    SOUND_DETUNE = 0xFD
};

/* The levels $14-$1E (g.level) queue no sound: SoundQueueAdd's rule. */

/* @routine 81:FA00 SoundQueueAdd: command `command` (0 queues nothing) */
void SoundQueueAdd(uint8_t command);
/* @routine 81:F9F0 SoundQueueF0, 81:F9FE SoundQueueF1, 81:F9F7 SoundQueueF2 */
void SoundQueueF0(void);
void SoundQueueF1(void);
void SoundQueueF2(void);
/* @routine 81:F9E3 SoundQueueF5WithA: fade out at `speed` (a speed of 0 is
   not queued, as the original, so the driver takes the byte after $F5 as its
   speed); 81:F9E1 SoundQueueF5With4 at speed 4 */
void SoundQueueF5WithA(uint8_t speed);
void SoundQueueF5With4(void);
/* @routine 81:F9D4 SoundQueueF6: fade in at `speed`; 81:F9D2
   SoundQueueF6With1 at speed 1 */
void SoundQueueF6(uint8_t speed);
void SoundQueueF6With1(void);

/* @routine 80:B463 SoundCommandSend: the next queued command to the driver;
   once an update, where the original's NMI sent it */
void SoundCommandSend(void);

/* @routine 80:B4AA SoundSetsLoad: the driver and both sample sets, the first
   time it is called; the game's power-on calls it */
void SoundSetsLoad(void);

/* @routine 81:FA23 SongPlay: song `song` loaded and started from its start */
void SongPlay(uint8_t song);

/* the data and samples SoundSetsLoad made, as the driver is given them */
const SoundData *sound_data(void);

/* The engine's music code (music.h): game.music.hz = MUSIC_HZ,
   game.music.tick = music_tick, game.music.queue_bytes = MUSIC_QUEUE_BYTES. */

#endif

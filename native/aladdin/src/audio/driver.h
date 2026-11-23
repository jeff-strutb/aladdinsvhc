/*
 * driver.h: Aladdin's sound driver as the game's music code: its state, all
 * on the audio thread.
 *
 * The driver plays 16 channels on the 8 voices: 8 for the music, 8 for sound
 * effects, effect channel n over voice n. A voice an effect holds is not
 * touched by its music channel, which goes on counting its notes so that it
 * can come back when the effect ends.
 *
 * The song, effect, instrument and table data keep their own format: one
 * 64 KB space in which positions are 16-bit, as the data itself stores them
 * (tracks jump by position, effects and songs are found by position). The
 * driver only reads it; a song load writes its blocks there. The samples are
 * engine samples, one for each entry of the original's sample directory.
 */
#ifndef ALADDIN_DRIVER_H
#define ALADDIN_DRIVER_H

#include <stdint.h>

#include "nshvc/nshvc.h"

#define DRIVER_VOICES 8
#define DRIVER_CHANNELS 16          /* 0-7 music, 8-15 effects */
#define DRIVER_SOURCES 78           /* entries of the sample directory */

/* where the driver's data sits in the sound data */
enum {
    DATA_LENGTHS_TRIPLET = 0x07DE,  /* note lengths by a note's top 3 bits, three tables */
    DATA_LENGTHS = 0x07E5,
    DATA_LENGTHS_DOTTED = 0x07EC,
    DATA_OCTAVES = 0x07F3,          /* a channel's octave (flags bits 0-3) as semitones */
    DATA_FIR_SETS = 0x0A83,         /* 8 coefficients a set */
    DATA_CURVE = 0x0BAF,            /* the level curve, 17 points (reads run on into the pan curve) */
    DATA_PAN_CURVE = 0x0BC0,
    DATA_PITCHES = 0x0CC2,          /* an octave's 13 pitches, words */
    DATA_ECHO_SETS = 0x0D10,        /* 8 bytes a set */
    DATA_SONG = 0x0D1F,             /* the loaded song's header (its 8 track positions follow) */
    DATA_SONG_BLOCKS = 0x0D20,      /* where a song's data is loaded */
    DATA_EFFECT_COUNT = 0x1DB0,
    DATA_EFFECT_VOLUME = 0x1DB1,    /* the music's volume, on the level curve */
    DATA_EFFECTS = 0x1DB2,          /* positions, high byte first */
    DATA_DIRECTORY = 0x3300,        /* the sample directory: start and loop words */
    DATA_INSTRUMENTS = 0x3438       /* 6 bytes: source, ADSR1, ADSR2, GAIN, pitch scale low, high */
};

/* Data and samples prepared on the game thread and handed over whole. */
typedef struct {
    uint8_t bytes[0x10000];
    nshvc_sample source[DRIVER_SOURCES];
} SoundData;

typedef struct {
    uint16_t track;             /* position of the next track byte; 0: no track */
    uint8_t flags;              /* bits 0-2 octave, 3 two octaves up, 4 dotted once, 5 the other lengths,
                                   6 tie the next note, 7 this note tied */
    uint8_t time;               /* the note's time left, in tempo steps */
    uint16_t gate;              /* the note's time until its release, in tempo units */
    uint8_t loops[4];           /* loop counters, one for each nesting command */
    uint8_t lfo_flags;          /* bit 0 a slide, bit 1 it goes up, bit 4 the vibrato restarts at key-on,
                                   bits 6-7 the vibrato's quarter */
    uint8_t vibrato_depth, tremolo_depth, lfo_rate, lfo_phase;
    uint8_t slide_rate;
    uint16_t pitch;             /* semitone in the high byte, its fraction in the low */
    uint16_t target;            /* the last note's pitch, a slide's end */
    uint8_t gate_fraction;      /* the part of a note's time it sounds, /256 */
    uint8_t transpose, fine_tune;
    uint16_t pitch_scale;       /* the instrument's tuning, 8.8 */
    uint8_t volume;
    uint8_t adsr1, gain;        /* the envelope a key-on gives the voice */
    uint8_t pan;                /* $80 the middle */
    uint8_t slot;               /* music: its instrument plus 1 (0 none); effects: the effect's priority */
    uint16_t tempo;             /* effects: the channel's own tempo */
    uint8_t tempo_count;        /* effects: its tempo's fraction carried */
} Channel;

/* What each voice was last given, as the engine has it. */
typedef struct {
    uint8_t source;
    uint8_t adsr1, adsr2, gain; /* ADSR1 bit 7: the ADSR is on, else GAIN */
    uint16_t pitch;
    int8_t left, right;
} VoiceOut;

typedef struct {
    uint8_t ready;
    uint8_t data[0x10000];      /* SoundData's bytes, with the songs loaded */
    nshvc_sample source[DRIVER_SOURCES];

    Channel ch[DRIVER_CHANNELS];
    VoiceOut out[DRIVER_VOICES];

    uint8_t half;               /* bit 7: the effects are being stepped, or a voice step is due next;
                                   bit 6 the music paused; bit 5 mono */
    uint8_t bit;                /* the voice being stepped, as a bit (voice 7 is bit 7) */
    uint8_t held;               /* during the music's step: bit 7 the voice an effect holds */
    uint8_t effect_voices;      /* voices effects hold */
    uint8_t effect_key_on, effect_key_off, effect_echo;
    uint8_t music_key_on, music_key_off, music_echo;
    uint8_t restore;            /* voices given back to the music: its instrument again */
    uint8_t step;               /* tempo steps this tick */
    uint16_t advance;           /* tempo units this tick */
    uint16_t tempo, tempo_offset;
    uint8_t tempo_count;
    uint8_t volume;             /* the music's, on the level curve */
    uint8_t transpose, detune;
    uint8_t fade;               /* $FF fading out, 1 in, 0 none */
    uint16_t fade_speed, fade_level;
    uint8_t echo_set;           /* the echo set in use, times 8 */
    uint8_t echo_wait, echo_waited;   /* the echo's settling after its delay changes */
    uint8_t echo_left, echo_right;    /* the echo volume to give once settled */

    /* the whole mix as the engine has it */
    int8_t main_left, main_right;
    int8_t evol_left, evol_right, feedback, fir[8];
    uint8_t delay;
    uint8_t echo_voices;        /* EON */
    uint8_t flg;                /* bit 5: no echo writes; 6 muted; 0-4 the noise rate */
} Driver;

extern Driver d;

/* the music code (driver.c), audio thread */
void driver_start(const SoundData *data);
void driver_tick(void);
void driver_command(uint8_t command, uint16_t fade_speed, int8_t offset);
void driver_song(uint16_t at, const uint8_t *bytes, uint16_t length);
void driver_reapply(void);          /* after a snapshot put the state in: the engine given it all again, every voice keyed off */

#endif

/*
 * driver.c: Aladdin's sound driver as the game's music code (driver.h). The
 * original ran on the SPC700; each routine is named by its @routine line.
 *
 * One tick every 256 DSP samples (timer 0 at $40: 8 ms). Ticks take turns:
 * one steps the channels (the music's at the song's tempo unless paused, then
 * each effect's at its own) and ends with the key-offs; the next brings the
 * voices up to date (echo, each changed voice's envelope) and ends with the
 * key-ons. Commands from the game run between ticks.
 *
 * Its writes to the DSP's registers are the engine's calls: a voice's sample
 * nshvc_voice_sample, its envelope nshvc_voice_adsr or nshvc_voice_gain (by
 * ADSR1's bit 7, as the DSP chooses), pitch, volume, key-on and key-off;
 * EON nshvc_voice_options; MVOL nshvc_sound_volume; EVOL, EFB, EDL and the
 * filter nshvc_echo; FLG nshvc_echo_writes, nshvc_sound_mute and
 * nshvc_sound_noise_rate. The driver's PMON and NON are 0 and stay so. ESA has
 * no call: the engine's echo buffer is its own, and the original's ($E800 up)
 * overlaps none of the sound data, which ends at $E7F9.
 */
#include <string.h>

#include "audio/driver.h"

Driver d;

static uint8_t bit_inverse;     /* the stepped voice's bit, inverted */

static uint8_t byte_at(uint16_t at) { return d.data[at]; }
static uint16_t word_at(uint16_t at) { return (uint16_t)(byte_at(at) | byte_at((uint16_t)(at + 1)) << 8); }
static int effects_now(void) { return (d.half & 0x80) != 0; }
static int voice_of(int c) { return c & 7; }

/* ---- the engine: what each register write becomes ---- */

static void envelope_send(int v)
{
    const VoiceOut *o = &d.out[v];

    if (o->adsr1 & 0x80)
        nshvc_voice_adsr((uint32_t)v, o->adsr1 & 0x0F, (o->adsr1 >> 4) & 7, o->adsr2 >> 5, o->adsr2 & 0x1F);
    else if (!(o->gain & 0x80))
        nshvc_voice_gain((uint32_t)v, NSHVC_GAIN_DIRECT, o->gain & 0x7F);
    else
        nshvc_voice_gain((uint32_t)v, (nshvc_gain_mode)(o->gain >> 5), o->gain & 0x1F);
}

static void out_gain(int v, uint8_t g) { d.out[v].gain = g; envelope_send(v); }
static void out_adsr1(int v, uint8_t a) { d.out[v].adsr1 = a; envelope_send(v); }
static void out_adsr2(int v, uint8_t a) { d.out[v].adsr2 = a; envelope_send(v); }

static void out_source(int v, uint8_t s)
{
    d.out[v].source = s;
    if (s < DRIVER_SOURCES)
        nshvc_voice_sample((uint32_t)v, d.source[s]);
}

static void out_pitch(int v, uint8_t high, uint8_t low)
{
    d.out[v].pitch = (uint16_t)((high << 8 | low) & 0x3FFF);
    nshvc_voice_pitch((uint32_t)v, d.out[v].pitch);
}

static void out_volume(int v, uint8_t left, uint8_t right)
{
    d.out[v].left = (int8_t)left;
    d.out[v].right = (int8_t)right;
    nshvc_voice_volume((uint32_t)v, d.out[v].left, d.out[v].right);
}

static void out_key_on(uint8_t mask)
{
    int v;

    for (v = 0; v < DRIVER_VOICES; v++)
        if (mask & (1u << v) && d.out[v].source < DRIVER_SOURCES)
            nshvc_voice_key_on((uint32_t)v, d.source[d.out[v].source]);
}

static void out_key_off(uint8_t mask)
{
    int v;

    for (v = 0; v < DRIVER_VOICES; v++)
        if (mask & (1u << v))
            nshvc_voice_key_off((uint32_t)v);
}

static void out_echo_voices(uint8_t mask)
{
    int v;

    for (v = 0; v < DRIVER_VOICES; v++)
        if ((mask ^ d.echo_voices) & (1u << v))
            nshvc_voice_options((uint32_t)v, 0, (mask >> v) & 1, 0);
    d.echo_voices = mask;
}

static void out_flg(uint8_t f)
{
    d.flg = f;
    nshvc_echo_writes(!(f & 0x20));
    nshvc_sound_mute((f & 0x40) != 0);
    nshvc_sound_noise_rate(f & 0x1F);
}

static void out_echo(void)
{
    nshvc_echo(d.delay & 0x0F, d.feedback, d.evol_left, d.evol_right, d.fir);
}

static void out_main_volume(uint8_t left, uint8_t right)
{
    d.main_left = (int8_t)left;
    d.main_right = (int8_t)right;
    nshvc_sound_volume(d.main_left, d.main_right);
}

/* ---- the level and pan curves ---- */

/*
 * @routine 85:89D0 CurveShift
 * The level curve at `pos` shifted right `shifts` times: the point at its
 * high byte, plus the fraction (its low byte) of the step to the next.
 */
static uint8_t curve_shift(uint16_t pos, int shifts)
{
    uint8_t i, f, base;

    pos = (uint16_t)(pos >> shifts);
    i = (uint8_t)(pos >> 8);
    f = (uint8_t)pos;
    base = byte_at((uint16_t)(DATA_CURVE + i));
    return (uint8_t)(base + (((uint8_t)(byte_at((uint16_t)(DATA_CURVE + i + 1)) - base) * f) >> 8));
}

/* @routine 85:8985 CurveAt16 */
static uint8_t curve_at16(uint16_t pos) { return curve_shift(pos, 4); }
/* @routine 85:898D CurveAt8 */
static uint8_t curve_at8(uint16_t pos) { return curve_shift(pos, 3); }
/* @routine 85:898B CurveAtY: the curve at y, with a fraction of $FF */
static uint8_t curve_at_y(uint8_t y) { return curve_shift((uint16_t)(y << 8 | 0xFF), 3); }

/* ---- a voice's envelope, instrument, pitch and volume ---- */

/*
 * @routine 85:8218 VoiceAdsr1Set
 * The channel's voice's ADSR1 from its kept value with the ADSR off, so that
 * the voice's GAIN takes its envelope over.
 */
static void adsr_off(int c)
{
    out_adsr1(voice_of(c), d.ch[c].adsr1 & 0x7F);
}

/*
 * @routine 85:8211 VoiceRelease
 * The channel's voice faded out: GAIN to an exponential decrease at rate $1E,
 * then the ADSR off.
 */
static void voice_release(int c)
{
    out_gain(voice_of(c), 0x9E);
    adsr_off(c);
}

/*
 * @routine 85:86C4 VoiceInstrumentFrom
 * Instrument n on the channel: its 6 bytes give the voice's sample, ADSR1,
 * ADSR2 and GAIN, the channel's kept ADSR1 and its tuning.
 * (85:86C1 VoiceInstrument is the same for n plus 1.)
 */
static void instrument_set(int c, uint8_t n)
{
    uint16_t at = (uint16_t)(DATA_INSTRUMENTS + n * 6);
    int v = voice_of(c);

    d.ch[c].adsr1 = byte_at((uint16_t)(at + 1));
    out_source(v, byte_at(at));
    out_adsr1(v, byte_at((uint16_t)(at + 1)));
    out_adsr2(v, byte_at((uint16_t)(at + 2)));
    out_gain(v, byte_at((uint16_t)(at + 3)));
    d.ch[c].pitch_scale = (uint16_t)(byte_at((uint16_t)(at + 5)) << 8 | byte_at((uint16_t)(at + 4)));
}

/*
 * @routine 85:8A2E VoicePitchOut
 * The voice's pitch from note `p` (semitone in the high byte): the song's
 * detune added to a music channel's; the lowest and highest notes stretched;
 * then the octave's table interpolated, shifted to the octave and scaled by
 * the instrument's tuning, to PITCHL (its low 4 bits clear) and PITCHH.
 */
static void pitch_out(int c, uint16_t p)
{
    const Channel *h = &d.ch[c];
    uint8_t high, low, n, octave, rest, step, shift, ml, mh;
    uint16_t base, w, value, t1, t2, t3, t4, sum, r;

    if (!effects_now())
        p = (uint16_t)(p + (d.detune << 8));
    high = (uint8_t)((p >> 8) - 1);
    low = (uint8_t)p;
    w = (uint16_t)(high << 8 | low);
    if (high >= 0x34)
        w = (uint16_t)(w + (uint8_t)(high - 0x34));
    else if (high < 0x13)
        w = (uint16_t)(w + (0xFF00u | (uint8_t)((uint8_t)(high - 0x13) << 1)));
    high = (uint8_t)(w >> 8);
    low = (uint8_t)w;
    /* the octave, and the semitone in it (12 words an octave) */
    n = (uint8_t)(high << 1);
    octave = (uint8_t)(n / 0x18);
    rest = (uint8_t)(n % 0x18);
    base = word_at((uint16_t)(DATA_PITCHES + rest));
    step = (uint8_t)(word_at((uint16_t)(DATA_PITCHES + rest + 2)) - base);
    value = (uint16_t)(base + ((step * low) >> 8));
    value = (uint16_t)(value << 1);
    shift = (uint8_t)(8 - octave);
    value = shift >= 16 ? 0 : (uint16_t)(value >> shift);
    /* by the tuning, 8.8, as the original's four products */
    ml = (uint8_t)h->pitch_scale;
    mh = (uint8_t)(h->pitch_scale >> 8);
    high = (uint8_t)(value >> 8);
    low = (uint8_t)value;
    t1 = (uint16_t)(mh * high);
    t2 = (uint16_t)(mh * low);
    t3 = (uint16_t)(ml * low);
    t4 = (uint16_t)(ml * high);
    sum = (uint16_t)(t3 + t1);
    r = (uint16_t)(((t4 & 0xFF) << 8 | t2 >> 8) + sum);
    out_pitch(voice_of(c), (uint8_t)(r >> 8), (uint8_t)(r & 0xF0));
}

/*
 * @routine 85:89F0 VoicePitchVibrato
 * The pitch with the vibrato: its depth times 24 by the phase (inverted in
 * its second half), added, or in its second turn taken off.
 */
static void pitch_vibrato(int c)
{
    const Channel *h = &d.ch[c];
    uint16_t depth = (uint16_t)((uint8_t)(h->vibrato_depth << 1) * 0x0C), off;
    uint8_t phase = h->lfo_phase;

    if (h->lfo_flags & 0x40)
        phase = (uint8_t)~phase;
    off = (uint16_t)((uint8_t)(depth >> 8) * phase + (((uint8_t)depth * phase) >> 8));
    pitch_out(c, h->lfo_flags & 0x80 ? (uint16_t)(h->pitch - off) : (uint16_t)(h->pitch + off));
}

/*
 * @routine 85:89E6 VoicePitchSet
 * The voice's pitch: with the vibrato when it has a depth.
 * (85:89EF VoicePitchFrom is the pitch as it is.)
 */
static void pitch_set(int c)
{
    if (d.ch[c].vibrato_depth)
        pitch_vibrato(c);
    else
        pitch_out(c, d.ch[c].pitch);
}

/*
 * @routine 85:89D3 VoicePitchUpdate
 * The pitch again when a slide or vibrato runs, or a music channel's song
 * has a detune.
 */
static void pitch_update(int c)
{
    if ((d.ch[c].lfo_flags & 0x01) || d.ch[c].vibrato_depth)
        pitch_set(c);
    else if (!effects_now() && d.detune)
        pitch_out(c, d.ch[c].pitch);
}

/*
 * @routine 85:8914 VoiceVolumeSet
 * The voice's left and right: the tremolo on the level curve; a music
 * channel's by the song's volume and the fade; by the channel's volume; then
 * each side by the pan curve ($14 steps, the middle unless stereo is off).
 */
static void volume_set(int c)
{
    const Channel *h = &d.ch[c];
    uint8_t phase = h->lfo_phase, level, scale, pan, side[2], i, base;
    uint16_t at;
    int s;

    if (!(h->lfo_flags & 0x40))
        phase = (uint8_t)~phase;
    level = curve_at8((uint16_t)(0x7E81 - h->tremolo_depth * phase));
    if (!effects_now()) {
        uint8_t music = (uint8_t)((level * d.volume) >> 8);
        uint16_t fade = d.fade & 0x80 ? d.fade_level : (uint16_t)(0xFFFF - d.fade_level);

        level = (uint8_t)((curve_at16(fade) * music) >> 8);
    }
    scale = (uint8_t)((h->volume * level) >> 8);
    pan = d.half & 0x20 ? 0x80 : h->pan;
    at = (uint16_t)(pan * 0x14);
    for (s = 1; s >= 0; s--) {
        i = (uint8_t)(at >> 8);
        base = byte_at((uint16_t)(DATA_PAN_CURVE + i));
        level = (uint8_t)(base + (((uint8_t)(byte_at((uint16_t)(DATA_PAN_CURVE + i + 1)) - base) * (uint8_t)at) >> 8));
        side[s] = (uint8_t)((level * scale) >> 8);
        at = (uint16_t)(0x1400 - at);
    }
    out_volume(voice_of(c), side[0], side[1]);
}

/*
 * @routine 85:8888 VoiceSlide
 * While a note sounds: the vibrato's phase on by its rate (each turn a
 * quarter on), the slide's pitch on by twice its rate until it reaches the
 * note; then, unless an effect holds the voice, its pitch and volume.
 */
static void voice_slide(int c)
{
    Channel *h = &d.ch[c];
    uint16_t sum = (uint16_t)(h->lfo_phase + h->lfo_rate);
    uint8_t f;

    h->lfo_phase = (uint8_t)sum;
    if (sum > 0xFF)
        h->lfo_flags = (uint8_t)(h->lfo_flags + 0x40);
    f = h->lfo_flags;
    if (f & 0x01) {
        uint8_t step = (uint8_t)(h->slide_rate << 1);
        uint16_t p = h->pitch;
        int reached;

        if (!(f & 0x02)) {
            reached = p < step;
            p = (uint16_t)(p - step);
            if (!reached)
                reached = p < h->target;
        } else {
            p = (uint16_t)(p + step);
            reached = p >= h->target;
        }
        if (reached) {
            h->lfo_flags = (uint8_t)(f - 1);
            p = h->target;
        }
        h->pitch = p;
    }
    if (d.held & 0x80)
        return;
    pitch_update(c);
    volume_set(c);
}

/* ---- channels ---- */

/*
 * @routine 85:861D TrackByte
 * The channel's next track byte.
 */
static uint8_t track_byte(int c)
{
    return byte_at(d.ch[c].track++);
}

/*
 * @routine 85:87A4 VoiceSfxMark
 * An effect's voice marked to get the music's instrument back, when its
 * music channel has one.
 */
static void voice_sfx_mark(int c)
{
    if (d.ch[c - 8].slot)
        d.restore |= d.bit;
}

/*
 * @routine 85:8799 VoiceStop
 * An effect's channel stopped: its priority cleared, its voice released and
 * keyed off, and given back to the music.
 */
static void voice_stop(int c)
{
    d.ch[c].slot = 0;
    voice_release(c);
    d.effect_key_off |= d.bit;
    voice_sfx_mark(c);
}

/*
 * @routine 85:876E VoiceFlagsSet
 * Sequencer command $04: the channel's flags bits 3, 5 and 6 from the value.
 */
static void flags_set(int c, uint8_t value)
{
    d.ch[c].flags = (uint8_t)((d.ch[c].flags & 0x97) | value);
}

/*
 * @routine 85:86FF SeqLoop
 * Commands $0E-$15 (counter (command - $0E) mod 4): $0E-$11 start the
 * counter at the value, or count it down, and jump while it runs; $12-$15
 * jump only on its last pass, setting the flags from the value. The jump is
 * the next two bytes, high first.
 * (85:8751 SeqLoopSkip steps past them.)
 */
static void seq_loop(int c, uint8_t command, uint8_t value)
{
    Channel *h = &d.ch[c];
    uint8_t *count = &h->loops[(command - 0x0E) & 3];
    uint8_t high;

    if (command < 0x12) {
        if (!*count) {
            *count = value;
        } else if (!--*count) {
            h->track = (uint16_t)(h->track + 2);
            return;
        }
    } else {
        if ((uint8_t)(*count - 1)) {
            h->track = (uint16_t)(h->track + 2);
            return;
        }
        *count = 0;
        flags_set(c, value);
    }
    high = track_byte(c);
    h->track = (uint16_t)(high << 8 | track_byte(c));
}

/*
 * Command $1A's store: setting `which` of the channel's vibrato and slide
 * settings (0 vibrato depth, 1 tremolo depth, 2 rate; past those the phase,
 * the slide's rate, the pitch and the target), addressed as the original's
 * arrays were, 8 channels' bytes apart, so that a large `which` reaches
 * another channel's.
 */
static void page_store(int c, uint8_t which, uint8_t value)
{
    int x = c < 8 ? c : 0x50 + (c - 8);
    unsigned rel = 8u + (uint8_t)(which * 8 + x), k;
    Channel *h;

    if (rel >= 0xA0)
        return;         /* past the channels' arrays: no track does it */
    h = &d.ch[rel < 0x50 ? (rel & 7) : 8 + (rel & 7)];
    k = (rel < 0x50 ? rel : rel - 0x50) >> 3;
    switch (k) {
    case 0: h->lfo_flags = value; break;
    case 1: h->vibrato_depth = value; break;
    case 2: h->tremolo_depth = value; break;
    case 3: h->lfo_rate = value; break;
    case 4: h->lfo_phase = value; break;
    case 5: h->slide_rate = value; break;
    case 6: h->pitch = (uint16_t)(value << 8 | (h->pitch & 0xFF)); break;
    case 7: h->pitch = (uint16_t)((h->pitch & 0xFF00) | value); break;
    case 8: h->target = (uint16_t)(value << 8 | (h->target & 0xFF)); break;
    case 9: h->target = (uint16_t)((h->target & 0xFF00) | value); break;
    }
}

static void echo_setup(uint8_t set);

/*
 * One sequencer command below $20 (the table at $0831); commands $04 and up
 * take a value, the next track byte. Returns 1 when the track ended.
 */
static int command_run(int c, uint8_t command)
{
    Channel *h = &d.ch[c];
    uint8_t value = 0, second;

    if (command >= 0x04)
        value = track_byte(c);
    switch (command) {
    /* @routine 85:866E SeqCmd00 */
    case 0x00: h->flags ^= 0x20; break;
    /* @routine 85:8672 SeqCmd01 */
    case 0x01: h->flags ^= 0x40; break;
    /* @routine 85:8676 SeqCmd02 */
    case 0x02: h->flags |= 0x10; break;
    /* @routine 85:867C SeqCmd03 */
    case 0x03: h->flags ^= 0x08; break;
    case 0x04: flags_set(c, value); break;
    /* @routine 85:8683 SeqCmd05: the tempo, high byte first: an effect's own, or the song's */
    case 0x05:
        second = track_byte(c);
        if (effects_now()) {
            h->tempo = (uint16_t)(value << 8 | second);
            h->tempo_count = 0;
        } else {
            d.tempo = (uint16_t)(value << 8 | second);
            d.tempo_count = 0;
        }
        break;
    /* @routine 85:869E SeqCmd06 */
    case 0x06: h->gate_fraction = value; break;
    /* @routine 85:86A2 SeqCmd07: the volume on the curve; a song's scaled by the bank's */
    case 0x07: {
        uint8_t v = curve_at_y(value);

        if (!effects_now())
            v = (uint8_t)((v * curve_at_y(byte_at(DATA_EFFECT_VOLUME))) >> 8);
        h->volume = v;
        break;
    }
    /* @routine 85:86B7 SeqCmd08: the instrument; a music channel keeps it (plus 1) */
    case 0x08:
        if (effects_now()) {
            instrument_set(c, value);
        } else {
            h->slot = (uint8_t)(value + 1);
            if (!(d.held & 0x80))
                instrument_set(c, value);
        }
        break;
    /* @routine 85:86F9 SeqCmd09 */
    case 0x09: h->flags = (uint8_t)((h->flags & 0xF8) | value); break;
    /* @routine 85:8704 SeqCmd0A */
    case 0x0A:
        if (!effects_now())
            d.transpose = value;
        break;
    /* @routine 85:870A SeqCmd0B */
    case 0x0B: h->transpose = value; break;
    /* @routine 85:870E SeqCmd0C */
    case 0x0C: h->fine_tune = value; break;
    /* @routine 85:8712 SeqCmd0D */
    case 0x0D: h->slide_rate = value; break;
    /* @routine 85:8716 SeqCmd0E, 85:871A SeqCmd0F, 85:871E SeqCmd10, 85:8722 SeqCmd11 (and $12-$15) */
    case 0x0E: case 0x0F: case 0x10: case 0x11:
    case 0x12: case 0x13: case 0x14: case 0x15:
        seq_loop(c, command, value);
        break;
    /* @routine 85:8758 SeqCmd16: a jump, high byte first */
    case 0x16:
        h->track = (uint16_t)(value << 8 | track_byte(c));
        break;
    /* @routine 85:8779 SeqCmd17: the track's end */
    case 0x17:
        if (effects_now()) {
            d.effect_voices &= bit_inverse;
            d.effect_echo &= bit_inverse;
            voice_stop(c);
            return 1;
        }
        h->track = 0;
        h->slot = 0;
        if (!(d.held & 0x80)) {
            voice_release(c);
            d.music_key_off |= d.bit;
        }
        return 1;
    /* @routine 85:87B4 SeqCmd18 */
    case 0x18: h->pan = (uint8_t)(value + 0x80); break;
    /* @routine 85:87BB SeqCmd19 */
    case 0x19:
        if (!effects_now())
            d.volume = curve_at_y(value);
        break;
    /* @routine 85:87C5 SeqCmd1A: the vibrato and slide settings; 3 the vibrato's restart */
    case 0x1A:
        second = track_byte(c);
        if (value == 3)
            h->lfo_flags = (uint8_t)((h->lfo_flags & 0xEF) | (uint8_t)(second << 4 | second >> 4));
        else
            page_store(c, value, second);
        break;
    /* @routine 85:87EB SeqCmd1B: echo set N, when it is not the one in use */
    case 0x1B: {
        uint8_t set = (uint8_t)(track_byte(c) * 8);

        if (set != d.echo_set) {
            d.echo_set = set;
            echo_setup(set);
        }
        break;
    }
    /* @routine 85:88A0 SeqCmd1C: the voice's echo on (bit 0) or off */
    case 0x1C: {
        uint8_t on = value & 0x01 ? d.bit : 0;

        if (!effects_now())
            d.music_echo = (uint8_t)((d.music_echo & bit_inverse) | on);
        else
            d.effect_echo = (uint8_t)((d.effect_echo & bit_inverse) | on);
        break;
    }
    /* @routine 85:88BA SeqCmd1D: GAIN, an exponential decrease at the value's rate */
    case 0x1D: h->gain = (uint8_t)(value | 0xA0); break;
    default: break;     /* $1E, $1F: the value is taken and nothing done */
    }
    return 0;
}

/*
 * The note byte b (its top 3 bits the length, its low 5 the semitone; 0 a
 * rest): its time, its pitch with the slide from the last note, its key-on
 * unless tied, and its gate.
 */
static void note(int c, uint8_t b)
{
    Channel *h = &d.ch[c];
    uint8_t y = (uint8_t)(((b >> 5) & 7) - 1), f = h->flags, length, semitone, high, low, lfo;
    uint16_t p, start;

    if (!(f & 0x30)) {
        length = byte_at((uint16_t)(DATA_LENGTHS + y));
    } else if (!(f & 0x10)) {
        length = byte_at((uint16_t)(DATA_LENGTHS_TRIPLET + y));
    } else {
        h->flags = (uint8_t)(f & 0xEF);
        length = byte_at((uint16_t)(DATA_LENGTHS_DOTTED + y));
    }
    h->time = (uint8_t)(h->time + length);
    if (!(b & 0x1F))
        return;
    semitone = (uint8_t)((b & 0x1F) + byte_at((uint16_t)(DATA_OCTAVES + (h->flags & 0x0F))));
    if (!effects_now())
        semitone = (uint8_t)(semitone + d.transpose);
    semitone = (uint8_t)(semitone + h->transpose);
    p = (uint16_t)((semitone << 8) + (int8_t)h->fine_tune);
    high = (uint8_t)(p >> 8);
    low = (uint8_t)p;
    if (high >= 0x61)
        high = 0x60;
    p = (uint16_t)(high << 8 | low);
    /* the slide from the last note, when the channel slides and the semitone changes */
    lfo = (uint8_t)(h->lfo_flags & 0xFE);
    start = p;
    if (h->slide_rate) {
        uint8_t last = (uint8_t)(h->target >> 8);

        if (last && last != high) {
            if (last >= high)
                lfo &= (uint8_t)~0x02;
            else
                lfo |= 0x02;
            lfo++;
            start = h->target;
        }
    }
    h->lfo_flags = lfo;
    h->target = p;
    h->pitch = start;
    /* the key-on, unless tied */
    if (!(h->flags & 0x80)) {
        if (h->lfo_flags & 0x10) {
            h->lfo_flags &= 0x3F;
            h->lfo_phase = 0;
        }
        if (effects_now()) {
            d.effect_key_on |= d.bit;
            d.effect_key_off |= d.bit;
            voice_release(c);
        } else if (!(d.held & 0x80)) {
            d.music_key_on |= d.bit;
            d.music_key_off |= d.bit;
            voice_release(c);
        }
    }
    /* a voice an effect holds is left as the effect has it */
    if (!(d.held & 0x80)) {
        pitch_set(c);
        volume_set(c);
    }
    /* the tie flag from bit 6 to bit 7; the gate, none when tied */
    f = h->flags;
    h->flags = (uint8_t)((f & 0x7F) | (f & 0x40) << 1);
    h->gate = h->flags & 0x80 ? 0 : (uint16_t)(h->time * h->gate_fraction);
}

/*
 * @routine 85:84AE VoiceTick
 * One tick of a channel: its note's gate and time by the tick's tempo; while
 * the note sounds, its slide; when its time runs out, its commands up to its
 * next note.
 */
static void voice_tick(int c)
{
    Channel *h = &d.ch[c];
    uint8_t b;

    bit_inverse = (uint8_t)~d.bit;
    if (h->time) {
        if (h->gate) {
            if (h->gate <= d.advance) {
                if (!(d.held & 0x80))
                    adsr_off(c);
                h->gate = 0;
            } else {
                h->gate = (uint16_t)(h->gate - d.advance);
            }
        }
        if (h->time > d.step) {
            h->time = (uint8_t)(h->time - d.step);
            if (h->time < 0xC1) {
                voice_slide(c);
                return;
            }
        } else {
            h->time = (uint8_t)(h->time - d.step);
        }
    }
    for (;;) {
        b = track_byte(c);
        if (b >= 0x20) {
            note(c, b);
            return;
        }
        if (command_run(c, b))
            return;
    }
}

/* ---- the echo ---- */

/*
 * @routine 85:8869 EchoFirSet
 * The echo filter's 8 coefficients from set n.
 */
static void fir_set(uint8_t n)
{
    uint8_t x = (uint8_t)(n * 8);
    int i;

    for (i = 0; i < 8; i++)
        d.fir[i] = (int8_t)byte_at((uint16_t)(DATA_FIR_SETS + (uint8_t)(x + i)));
}

/*
 * @routine 85:8850 EchoOff
 * Echo volume 0, no voice echoed, echo writes off.
 */
static void echo_off(void)
{
    d.evol_left = d.evol_right = 0;
    out_echo();
    out_echo_voices(0);
    out_flg(0x20);
}

/*
 * @routine 85:880D EchoSetup
 * Echo set `set` (times 8): the main volume, the echo volume to give once
 * settled, the echo off, the delay (the settling counted from the old
 * delay), the feedback and the filter.
 */
static void echo_setup(uint8_t set)
{
    uint8_t wait;

    out_main_volume(byte_at((uint16_t)(DATA_ECHO_SETS + set)), byte_at((uint16_t)(DATA_ECHO_SETS + set + 1)));
    d.echo_left = byte_at((uint16_t)(DATA_ECHO_SETS + set + 2));
    d.echo_right = byte_at((uint16_t)(DATA_ECHO_SETS + set + 3));
    echo_off();
    wait = (uint8_t)~((d.delay & 0x0F) + 1);
    if (d.echo_waited & 0x80)
        wait = (uint8_t)(wait + d.echo_waited);
    d.echo_waited = wait;
    d.delay = byte_at((uint16_t)(DATA_ECHO_SETS + set + 5));
    d.echo_wait = (uint8_t)(d.echo_wait + 0x20);
    d.feedback = (int8_t)byte_at((uint16_t)(DATA_ECHO_SETS + set + 6));
    fir_set(byte_at((uint16_t)(DATA_ECHO_SETS + set + 7)));
    out_echo();
}

/* ---- the commands ---- */

/*
 * @routine 85:83F6 VoicesReleaseFree
 * Each voice no effect holds released and keyed off.
 */
static void voices_release_free(void)
{
    int x;

    d.bit = 0x80;
    for (x = 7; x >= 0; x--) {
        if (!(d.effect_voices & d.bit)) {
            voice_release(x);
            d.music_key_off |= d.bit;
        }
        d.bit >>= 1;
    }
    d.music_key_on = 0;
    d.restore = 0;
}

/*
 * @routine 85:8346 SequencerReset
 * Command $F2: the music's channels cleared and their voices released.
 * (85:835C PageClear50 is the clearing.)
 */
static void sequencer_reset(void)
{
    voices_release_free();
    d.music_echo = 0;
    memset(d.ch, 0, 8 * sizeof d.ch[0]);
}

/*
 * @routine 85:8442 FadeClear
 * (85:8448 FadeSet sets the level and step.)
 */
static void fade_clear(void)
{
    d.fade_level = 0;
    d.fade_speed = 0;
    d.fade = 0;
}

/*
 * @routine 85:8410 FadeStart
 * Commands $F5 (out) and $F6 (in) at `speed`: none when it already runs that
 * way; 0 clears it; turned round, it goes on from where it had got to.
 */
static void fade_start(uint8_t way, uint16_t speed)
{
    uint8_t was;

    if (way == d.fade)
        return;
    if (!speed) {
        fade_clear();
        return;
    }
    was = (uint8_t)(d.fade << 1);
    d.fade_speed = speed;
    d.fade = way;
    if (!was)
        d.fade_level = 0;
    else
        d.fade_level = (uint16_t)(0xFFFF - d.fade_level);
}

/*
 * @routine 85:836D SongStartKeep
 * The song whose header is at `at`: the music reset, its slide, transposes
 * and fade cleared, its volume full; each of its 8 tracks given to its
 * channel (one that starts with the end command is left off).
 * (85:8364 SoundCmdFA and 85:8367 SoundCmdFB start the loaded song's.)
 */
static void song_start(uint16_t at)
{
    int x;

    sequencer_reset();
    d.tempo_offset = 0;
    d.transpose = 0;
    d.detune = 0;
    d.volume = 0xFF;
    fade_clear();
    for (x = 7; x >= 0; x--) {
        uint16_t track;

        at++;
        track = (uint16_t)(byte_at(at) << 8);
        at++;
        track |= byte_at(at);
        if (byte_at(track) != 0x17) {
            d.ch[x].track = track;
            d.ch[x].lfo_flags = 0x10;
            d.ch[x].gain = 0xB8;
            d.ch[x].pan = 0x80;
        }
    }
}

/*
 * @routine 85:825B SoundCommand (below $F0)
 * Sound `n` (taken off the bank's count until it is below it): a song when
 * its priority is 0, else an effect, whose tracks take the effect channels
 * whose own effect's priority is not above its own.
 * (85:82E1 VoiceStateClear clears a channel taken.)
 */
static void sound_start(uint8_t n, int8_t offset)
{
    uint8_t count = byte_at(DATA_EFFECT_COUNT), priority, x2;
    uint16_t at;
    int v;

    if (!count)
        return;
    while (n >= count)
        n = (uint8_t)(n - count);
    x2 = (uint8_t)(n << 1);
    at = (uint16_t)(byte_at((uint16_t)(DATA_EFFECTS + x2)) << 8 | byte_at((uint16_t)(DATA_EFFECTS + x2 + 1)));
    if (!at)
        return;
    priority = byte_at(at);
    if (!priority) {
        song_start(at);
        return;
    }
    d.bit = 0x80;
    for (v = 7; v >= 0; v--, d.bit >>= 1) {
        int c = 8 + v;
        uint16_t track;

        at++;
        track = (uint16_t)(byte_at(at) << 8);
        at++;
        track |= byte_at(at);
        if (byte_at(track) == 0x17 || priority < d.ch[c].slot)
            continue;
        d.effect_voices |= d.bit;
        d.effect_echo &= (uint8_t)~d.bit;
        d.music_key_on &= (uint8_t)~d.bit;
        memset(&d.ch[c], 0, sizeof d.ch[c]);
        voice_release(c);
        d.ch[c].slot = priority;
        d.ch[c].track = track;
        d.ch[c].lfo_flags = 0x10;
        d.ch[c].gain = 0xB8;
        d.ch[c].pan = (uint8_t)(offset + 0x80);
    }
}

/*
 * @routine 85:832D SoundCmdF1
 * Command $F1: every effect stopped.
 */
static void effects_stop(void)
{
    int v;

    d.bit = 0x80;
    for (v = 7; v >= 0; v--) {
        if (d.effect_voices & 0x80)
            voice_stop(8 + v);
        d.effect_voices = (uint8_t)(d.effect_voices << 1);
        d.bit >>= 1;
    }
    d.effect_key_on = 0;
    d.effect_echo = 0;
}

/*
 * @routine 85:832A SoundStopAll
 * Command $F0: the music and every effect stopped.
 */
static void stop_all(void)
{
    sequencer_reset();
    effects_stop();
}

/*
 * @routine 85:83E0 SoundCmdF4
 * Command $F4: the music goes on; each voice no effect holds gets its
 * instrument back. (85:83DC SoundCmdF3, command $F3, pauses it.)
 */
static void music_go_on(void)
{
    int v;

    d.half &= (uint8_t)~0x40;
    d.music_key_on = 0;
    d.bit = 0x80;
    for (v = 7; v >= 0; v--, d.bit >>= 1)
        if (!(d.effect_voices & d.bit))
            voice_sfx_mark(8 + v);
}

/*
 * @routine 85:83AE SoundCmdFC
 * Command $FC: the music's tempo offset, the signed value times 3 x $69 /
 * 256. (85:83D6 SoundCmdFD, $FD, sets the detune to the value.)
 */
static void tempo_offset_set(int8_t value)
{
    uint8_t m = (uint8_t)(value < 0 ? -value : value);
    uint16_t t = (uint16_t)(m * 3 + ((m * 0x69) >> 8));

    d.tempo_offset = value < 0 ? (uint16_t)(0 - t) : t;
}

/*
 * A command from the game: below $F0 a sound, $F0 and up the driver's own.
 * `fade_speed` is $F5's and $F6's speed; `offset` the value the original took
 * from port 1 (an effect's pan offset, $FC's and $FD's value), which the game
 * leaves 0.
 */
void driver_command(uint8_t command, uint16_t fade_speed, int8_t offset)
{
    if (!d.ready)
        return;
    if (command < 0xF0) {
        sound_start(command, offset);
        return;
    }
    switch (command) {
    case 0xF0: stop_all(); break;
    case 0xF1: effects_stop(); break;
    case 0xF2: sequencer_reset(); break;
    case 0xF3: d.half |= 0x40; voices_release_free(); break;
    case 0xF4: music_go_on(); break;
    /* @routine 85:8413 SoundCmdF5, 85:8417 SoundCmdF6 */
    case 0xF5: fade_start(0xFF, fade_speed); break;
    case 0xF6: fade_start(0x01, fade_speed); break;
    /* @routine 85:844D SoundCmdF7, 85:8450 SoundCmdF8: stereo, mono */
    case 0xF7: d.half &= (uint8_t)~0x20; break;
    case 0xF8: d.half |= 0x20; break;
    /* @routine 85:8453 SoundUpload: everything stops; the data comes as driver_song's */
    case 0xF9: stop_all(); break;
    case 0xFB: song_start(DATA_SONG); break;
    case 0xFC: tempo_offset_set(offset); break;
    case 0xFD: d.detune = (uint8_t)offset; break;
    default: break;     /* $FA comes as driver_song; $FE and $FF are no command's: the game sends neither */
    }
}

/*
 * A block of an upload into the sound data. Command $FA is an upload then
 * the loaded song from its start: music.c gives it as driver_command($F9),
 * driver_song for each block, then driver_command($FB).
 */
void driver_song(uint16_t at, const uint8_t *bytes, uint16_t length)
{
    uint32_t i;

    for (i = 0; i < length; i++)
        d.data[(uint16_t)(at + i)] = bytes[i];
}

/* ---- the start and the ticks ---- */

/*
 * @routine 85:80BD SoundDriverStart (its start)
 * The driver's state cleared; the DSP's set-up (85:8222 DspSetup: no pitch
 * modulation, no noise, the sample directory, then echo set 0).
 */
void driver_start(const SoundData *data)
{
    int v;

    memset(&d, 0, sizeof d);
    memcpy(d.data, data->bytes, sizeof d.data);
    memcpy(d.source, data->source, sizeof d.source);
    for (v = 0; v < DRIVER_VOICES; v++) {
        nshvc_voice_options((uint32_t)v, 0, 0, 0);
        out_source(v, 0);       /* the DSP's SRCN at power-on */
    }
    d.flg = 0xE0;       /* the DSP's at power-on; EchoOff sets it */
    echo_setup(0);
    d.ready = 1;
}

/*
 * driver_reapply: after a snapshot put the driver's state in (music.c): the engine's voices, echo and mix given everything
 * the driver last gave them, since the driver sends only what changes and the engine holds what some other state left.
 * Every voice is keyed off: a note that was sounding is silent until the next (FRAMERATE.md 10.2).
 */
void driver_reapply(void)
{
    int v;

    for (v = 0; v < DRIVER_VOICES; v++) {
        const VoiceOut *o = &d.out[v];

        nshvc_voice_key_off((uint32_t)v);
        nshvc_voice_options((uint32_t)v, 0, (d.echo_voices >> v) & 1, 0);
        if (o->source < DRIVER_SOURCES)
            nshvc_voice_sample((uint32_t)v, d.source[o->source]);
        envelope_send(v);
        nshvc_voice_pitch((uint32_t)v, o->pitch);
        nshvc_voice_volume((uint32_t)v, o->left, o->right);
    }
    nshvc_sound_volume(d.main_left, d.main_right);
    out_echo();
    nshvc_echo_writes(!(d.flg & 0x20));
    nshvc_sound_mute((d.flg & 0x40) != 0);
    nshvc_sound_noise_rate(d.flg & 0x1F);
}

/* The ticks that bring the voices up to date: the echo (or its settling),
   each changed voice's envelope, the key-ons. */
static void voices_update(void)
{
    uint8_t key_on;
    int x;

    if (d.echo_wait != d.echo_waited) {
        /* settling after a new delay: echo writes on, every voice echoed, at volume 0 */
        if (!(d.echo_waited & 0x80)) {
            out_flg(0x00);
            out_echo_voices(0xFF);
        }
        d.echo_waited++;
    } else {
        out_echo_voices((uint8_t)((~d.effect_voices & d.music_echo) | d.effect_echo));
        d.evol_left = (int8_t)d.echo_left;
        d.evol_right = (int8_t)d.echo_right;
        out_echo();
    }
    key_on = (uint8_t)(d.music_key_on | d.effect_key_on);
    for (x = 7; x >= 0; x--) {
        if (d.effect_key_on & 0x80) {
            out_gain(x, d.ch[8 + x].gain);
            out_adsr1(x, d.ch[8 + x].adsr1);
        } else if (d.restore & 0x80) {
            instrument_set(x, (uint8_t)(d.ch[x].slot - 1));
        } else if (d.music_key_on & 0x80) {
            out_gain(x, d.ch[x].gain);
            out_adsr1(x, d.ch[x].adsr1);
        }
        d.effect_key_on = (uint8_t)(d.effect_key_on << 1);
        d.restore = (uint8_t)(d.restore << 1);
        d.music_key_on = (uint8_t)(d.music_key_on << 1);
    }
    out_key_on(key_on);
}

/*
 * @routine 85:80BD SoundDriverStart (its loop's tick)
 * One tick of the driver's timer.
 */
void driver_tick(void)
{
    uint16_t sum;
    uint8_t key_off;
    int x;

    if (!d.ready)
        return;
    d.half = (uint8_t)(d.half + 0x80);
    if (!(d.half & 0x80)) {
        voices_update();
        return;
    }
    d.half &= 0x7F;
    if (!(d.half & 0x40)) {
        /* the music, at the song's tempo with its offset */
        d.bit = 0x80;
        d.held = d.effect_voices;
        d.advance = (uint16_t)(d.tempo + d.tempo_offset);
        sum = (uint16_t)(d.tempo_count + d.advance);
        d.step = (uint8_t)(sum >> 8);
        d.tempo_count = (uint8_t)sum;
        for (x = 7; x >= 0; x--) {
            if (d.ch[x].track)
                voice_tick(x);
            d.held = (uint8_t)(d.held << 1);
            d.bit >>= 1;
        }
        /* the fade */
        if ((uint32_t)d.fade_speed + d.fade_level > 0xFFFF) {
            d.fade_speed = 0;
            d.fade_level = 0xFFFF;
        } else {
            d.fade_level = (uint16_t)(d.fade_speed + d.fade_level);
        }
    }
    /* the effects, each at its own tempo */
    d.half |= 0x80;
    d.bit = 0x80;
    d.held = 0;
    for (x = 7; x >= 0; x--) {
        int c = 8 + x;

        if (d.effect_voices & d.bit) {
            d.advance = d.ch[c].tempo;
            sum = (uint16_t)(d.ch[c].tempo_count + d.advance);
            d.step = (uint8_t)(sum >> 8);
            d.ch[c].tempo_count = (uint8_t)sum;
            voice_tick(c);
        }
        d.bit >>= 1;
    }
    key_off = (uint8_t)(d.effect_key_off | d.music_key_off);
    d.effect_key_off = 0;
    d.music_key_off = 0;
    out_key_off(key_off);
}

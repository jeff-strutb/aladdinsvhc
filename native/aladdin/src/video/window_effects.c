/*
 * window_effects.c: the window effects some screens run as a task: a
 * per-line effect started in a free channel (4-7) from a table set, and for
 * some the line data it reads changed each frame (the waves).
 *
 * Converted so far: the task, the channel search and start, effects 2, 3
 * (with the waves), 4, 7 and 12. Effects 0, 1 and 5 are dead (video/NOTES).
 * Effects 6, 8, 9, 10 and 11 print their name once and end the task (not
 * converted yet).
 */
#include <stdio.h>

#include "data/data.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video_tasks.h"

#define LINE_DATA_AT 0xF400u        /* where g.line_data starts in the original's bank $7E */
#define WINDOW_EDGES 0x0400u        /* the window's edges per line ($7E:F800), in g.line_data */
#define WAVE_SINES 0x5600u          /* in g.unpacked (the original's $7F:5600) */
#define WAVES_TABLE 0x5300u         /* in g.unpacked: the window table while the waves are built */
#define WAVES_BYTES 0x200u

enum { NO_CHANNEL = -1 };

/* the per-line target a register set's transfer reaches */
static int target_of(uint8_t reg)
{
    switch (reg) {
    case 0x2C: return LINE_TM;
    case 0x2D: return LINE_TS;
    case 0x21: return LINE_PALETTE;
    case 0x0E: return LINE_BG1_Y;
    case 0x0F: return LINE_BG2_X;
    case 0x12: return LINE_BG3_Y;
    case 0x26: return LINE_WINDOW1;
    case 0x28: return LINE_WINDOW2;
    case 0x05: return LINE_MODE;
    case 0x1B: return LINE_M7_A;
    case 0x32: return LINE_FIXED;
    case 0x24: return LINE_WINDOW_USE_BG34;
    default: return -1;
    }
}

/*
 * @routine 82:871B HdmaChannelFree
 * WHAT IT DOES: The highest free channel of 4-7 (neither on nor kept by
 * other code), or NO_CHANNEL when all four are in use.
 */
static int channel_free(void)
{
    uint8_t used = (uint8_t)((g.line_effects_on | g.line_channels_kept) & 0xF0);

    for (int ch = 7; ch >= 4; ch--)
        if (!(used >> ch & 1))
            return ch;
    return NO_CHANNEL;
}

/*
 * @routine 82:873B HdmaStart
 * WHAT IT DOES: Starts table set `set` on register set `regs` in a free
 * channel, its table written at `place` in the line data: returns the
 * channel, or NO_CHANNEL when none is free. The channel is not turned on.
 * HOW: The set's entries copied to the table (each a line count and a word;
 * the original adds a value to each word, which it has just cleared) at
 * its place in the line data; the channel pointed at it there.
 */
static int hdma_start(uint8_t set, uint8_t regs, uint16_t place)
{
    int ch = channel_free(), target;
    LineChannel *c;
    uint8_t mode;

    if (ch == NO_CHANNEL)
        return NO_CHANNEL;
    mode = window_register_mode(regs);
    target = target_of(window_register_target(regs));
    c = &g.line_channel[ch];
    c->target = (uint8_t)(target < 0 ? LINE_TARGETS : target);
    c->direct = !(mode & 0x40);
    c->table_at = place;
    g.line_table_at[ch - 4] = place;    /* where the original's NMI points the channel each frame */
    if (target < 0)
        fprintf(stderr, "video: window register set %02X reaches no per-line target\n", regs);
    for (unsigned i = 0;; i++) {
        WindowTableEntry e = window_table_entry(set, i);
        uint32_t at = place + 3u * i;

        if (at < LINE_DATA_SIZE)
            g.line_data[at] = e.count;
        if (!e.count)
            break;
        if (at + 2 < LINE_DATA_SIZE) {
            g.line_data[at + 1] = (uint8_t)e.word;
            g.line_data[at + 2] = (uint8_t)(e.word >> 8);
        }
    }
    return ch;
}

static void channel_on(int ch)
{
    g.line_effects_on |= (uint8_t)(1u << ch);
}

/*
 * @routine 82:82AB HalfSineBuild
 * WHAT IT DOES: Builds at $7F:5600 (in g.unpacked, as the original) a half
 * turn of sines as bytes, 64 rising then the same falling, and from $5680
 * their complements.
 */
static void half_sine_build(void)
{
    for (unsigned x = 0; x < 0x40; x++) {
        uint8_t v = quarter_sine_step(x);

        g.unpacked[WAVE_SINES + x] = v;
        g.unpacked[WAVE_SINES + 0x80 + x] = (uint8_t)~v;
        g.unpacked[WAVE_SINES + 0x40 + (x ^ 0x3F)] = v;
        g.unpacked[WAVE_SINES + 0xC0 + (x ^ 0x3F)] = (uint8_t)~v;
    }
}

/*
 * @routine 82:8226 WindowColoursWait
 * WHAT IT DOES: Sets the windows and colour maths the waves use (BG1 and BG2
 * masked by the windows on the main screen; the colour window window 1, on
 * and inverted; colour maths kept out of it; the fixed colour, red 6, green
 * 3, blue 22, added to the backdrop), then waits a frame.
 */
static TaskRun colours_wait(Task *t)
{
    g.window_main = 0x03;
    g.window_sub = 0x00;
    g.window_use[4] = 0x0;              /* the original's $30: the sprites' nibble 0, the colour's 3 */
    g.window_use[5] = 0x3;
    g.blend_clip = 0;                   /* the original's $20: prevent 2 (inside the colour window) */
    g.blend_prevent = 2;
    g.blend_use_sub = 0;
    g.blend_direct_colour = 0;
    g.blend_subtract = 0;               /* the original's $20: add, on the backdrop */
    g.blend_half = 0;
    g.blend_layers = 0x20;
    g.fixed_red = 0x06;
    g.fixed_green = 0x03;
    g.fixed_blue = 0x16;
    return task_wait(t, 1);
}

/*
 * @routine 82:812C WindowWavesLoop
 * WHAT IT DOES: The waves the wave effects end in, for good: each frame the
 * window's lines are the shape's edges moved by a sine down the screen, the
 * sine starting one step on each frame.
 * HOW: As the original, the window table is built at $7F:5300 and copied
 * back to $7E:F800 each frame; the sine is read at $7F:5600 plus an index
 * whose high byte, past the first pair of offset $60 or more, is $FF (the
 * block move before the loop leaves A $FFFF and the index takes A's high
 * byte), so those pairs read $7F:55xx: in the original the bytes Mode 7's
 * set-up left there.
 */
static TaskRun waves_frame(Task *t)
{
    uint16_t y, x = window_shape_last();

    t->own.waves.count = t->own.waves.step;
    t->own.waves.phase = (uint16_t)((t->own.waves.phase & 0xFF00) | (uint8_t)(t->own.waves.phase + 1));
    y = t->own.waves.phase;
    for (;;) {
        uint16_t at = (uint16_t)(WAVE_SINES + y);
        uint8_t sine = at < sizeof g.unpacked ? g.unpacked[at] : 0;

        if ((uint32_t)WAVES_TABLE + x + 1 < sizeof g.unpacked) {
            g.unpacked[WAVES_TABLE + x] = (uint8_t)(sine + window_shape_byte(x));
            g.unpacked[WAVES_TABLE + x + 1] = (uint8_t)(sine + window_shape_byte((uint16_t)(x + 1)));
        }
        if (x >= 0x60)
            y = (uint16_t)(0xFF00 | (uint8_t)(y - 1));
        x = (uint16_t)(x - 2);
        if (!x)
            break;
        if (--t->own.waves.count)
            continue;
        t->own.waves.step++;
        if (x < 0x60)
            t->own.waves.step = 0xFF;
        break;
    }
    for (unsigned i = 0; i < WAVES_BYTES; i++)
        g.line_data[WINDOW_EDGES + i] = g.unpacked[WAVES_TABLE + i];
    return colours_wait(t);
}

/*
 * @routine 82:8000 WindowEffectTask
 * WHAT IT DOES: Runs window effect `param` / 2, retrying each frame while
 * no channel is free.
 * HOW: The effects, by the task's parameter:
 *   effect 2 (82:80E2): table set 4 on register set 6 from the base; ends.
 *   effect 3 (82:80F6): set $0A on register set 6 at $30; the half sine
 *     built; the shape's pairs from the last down to the second to the
 *     window edges; the channel on; the waves for good.
 *   effect 4 (82:82DB): set 6 on register set 8 at $20; the line data's
 *     $80 = 7 and $81 = 0; the channel on; a 2-frame wait for good.
 *   effect 7 (82:8307): set $0C on register set 8 at $20; the line data's
 *     $80 = 1 and $81 = 7; the channel on; a 2-frame wait for good.
 *   effect 12 (82:86CF): set $14 on register set 0 from the base; the line
 *     data's $80 = the main screen's layers and $81 their BG bits; the
 *     channel on; the sprites masked by window 1 on both screens (inverted),
 *     window 1 from $38 to $C7, the BG layers' window use 0; ends.
 */
TaskRun WindowEffectTask(Task *t)
{
    uint16_t *at = task_enter(t);
    int ch;

    switch (*at) {
    case 20:                            /* the waves, each frame */
        return waves_frame(t);
    case 10:                            /* effect 4: waiting for good */
        return task_wait(t, 2);
    default:
        break;
    }
    switch (t->param) {
    case 0x04:
        if ((ch = hdma_start(0x04, 0x06, 0x0000)) == NO_CHANNEL)
            return task_wait(t, 1);
        channel_on(ch);
        return task_exit(t);
    case 0x06:
        if ((ch = hdma_start(0x0A, 0x06, 0x0030)) == NO_CHANNEL)
            return task_wait(t, 1);
        half_sine_build();
        for (uint16_t x = window_shape_last(); x; x = (uint16_t)(x - 2)) {
            g.line_data[WINDOW_EDGES + x] = window_shape_byte(x);
            g.line_data[WINDOW_EDGES + x + 1] = window_shape_byte((uint16_t)(x + 1));
        }
        channel_on(ch);
        for (unsigned i = 0; i < WAVES_BYTES; i++)
            g.unpacked[WAVES_TABLE + i] = g.line_data[WINDOW_EDGES + i];
        t->own.waves.phase = 0;
        t->own.waves.step = 1;
        *at = 20;
        return colours_wait(t);
    case 0x08:
        if ((ch = hdma_start(0x06, 0x08, 0x0020)) == NO_CHANNEL)
            return task_wait(t, 1);
        g.line_data[0x80] = 0x07;
        g.line_data[0x81] = 0x00;
        channel_on(ch);
        *at = 10;
        return task_wait(t, 2);
    case 0x0E:
        if ((ch = hdma_start(0x0C, 0x08, 0x0020)) == NO_CHANNEL)
            return task_wait(t, 1);
        g.line_data[0x80] = 0x01;
        g.line_data[0x81] = 0x07;
        channel_on(ch);
        *at = 10;
        return task_wait(t, 2);
    case 0x18:
        if ((ch = hdma_start(0x14, 0x00, 0x0000)) == NO_CHANNEL)
            return task_wait(t, 1);
        g.line_data[0x80] = g.main_layers;
        g.line_data[0x81] = (uint8_t)(g.main_layers & 0x0F);
        channel_on(ch);
        g.window_main = 0x10;
        g.window_sub = 0x10;
        g.window_edges[0] = 0x38;
        g.window_edges[1] = 0xC7;
        for (int i = 0; i < 4; i++)
            g.window_use[i] = 0;        /* the BG layers' nibbles ($03AA, $03AB) */
        g.window_use[4] = 0x3;          /* the original's 3: the sprites' window 1, on and inverted */
        g.window_use[5] = 0x0;
        return task_exit(t);
    default: {
        static int told[16];

        if (!told[t->param / 2 & 15]) {
            told[t->param / 2 & 15] = 1;
            fprintf(stderr, "unconverted: window effect %u (WindowEffectTask)\n", t->param / 2);
        }
        return task_exit(t);
    }
    }
}

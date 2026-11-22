/*
 * window_effects.c: the window effects some screens run as a task: a
 * per-line effect started in a free channel (4-7) from a table set, and for
 * some the line data it reads changed each frame (the waves).
 *
 * Converted so far: the task, the channel search and start, effects 2, 3
 * and 6 (with the waves), 4, 7, 8, 9, 10, 11 and 12. Effects 0, 1 and 5 are
 * dead (video/NOTES).
 *
 * In a level (video_level_view) the circle (effect 2) and effects 8, 10 and
 * 11 are in the view's columns (VIEW.md 6.3): their windows are the
 * LINE_WINDOW*_WIDE channels, a table of 5 bytes an entry in g.wide_data (a
 * line count, then the left and right edges, 16 bits each), built here from
 * the places in the level and the view's width and height. The edges the
 * original gave as 0 and $FF are 0 and the view's last column; its lines are
 * the view's lines. At 256 x 224 each is the original's window.
 */
#include <stdio.h>

#include "data/data.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video_internal.h"
#include "video/video_tasks.h"
#include "video/view.h"

#define LINE_DATA_AT 0xF400u        /* where g.line_data starts in the original's bank $7E */
#define WINDOW_EDGES 0x0400u        /* the window's edges per line ($7E:F800), in g.line_data */
#define WAVE_SINES 0x5600u          /* in g.unpacked (the original's $7F:5600) */
#define WAVES_TABLE 0x5300u         /* in g.unpacked: the window table while the waves are built */
#define WAVES_BYTES 0x200u

enum { NO_CHANNEL = -1 };

/* a wide window table's entries: `lines` lines of the window left..right, in runs of at most 127, from byte `at` of
   g.wide_data, none past `end`; returns where the next goes */
static unsigned wide_run(unsigned at, unsigned end, int32_t lines, uint16_t left, uint16_t right)
{
    while (lines > 0 && at + 5 <= end) {
        unsigned n = lines > 127 ? 127u : (unsigned)lines;

        g.wide_data[at] = (uint8_t)n;
        g.wide_data[at + 1] = (uint8_t)left;
        g.wide_data[at + 2] = (uint8_t)(left >> 8);
        g.wide_data[at + 3] = (uint8_t)right;
        g.wide_data[at + 4] = (uint8_t)(right >> 8);
        at += 5;
        lines -= (int32_t)n;
    }
    return at;
}

/* the table's end: a count of 0, from where the last entry ended */
static void wide_end(unsigned at)
{
    g.wide_data[at] = 0;
}

/* a window effect's channel turned to a wide window (window 1 or 2) with its table at `at` of g.wide_data */
static void wide_channel(int ch, LineTarget target, unsigned at)
{
    LineChannel *c = &g.line_channel[ch];

    c->target = (uint8_t)target;
    c->direct = 1;
    c->table_at = (uint16_t)at;
    c->cart.at = 0;
    g.line_table_at[ch - 4] = (uint16_t)at;
}

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
    c->cart.at = 0;
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

/* WindowWavesLoop's start: the window table moved to $7F:5300 (g.unpacked),
   the phase 0 and the step 1, then the first frame's colours and wait */
static TaskRun waves_start(Task *t, uint16_t *at)
{
    for (unsigned i = 0; i < WAVES_BYTES; i++)
        g.unpacked[WAVES_TABLE + i] = g.line_data[WINDOW_EDGES + i];
    t->own.waves.phase = 0;
    t->own.waves.step = 1;
    *at = 20;
    return colours_wait(t);
}

/*
 * @routine 82:8253 WindowLinesFromShape
 * WHAT IT DOES: Writes the window's left and right edges for up to `lines`
 * lines, from the bottom up: the shape ($82:BBAE) narrowed to at most
 * `half` pixels either side of each line's middle, moved right by `shift`.
 * HOW: From the shape's last pair and the edges at `edge_at`, back a pair a
 * line: the middle of the shape's left and right; the left edge the larger
 * of the shape's left and the middle less `half`, the right the smaller of
 * its right and the middle plus `half` (each sum a byte, as the original's),
 * each plus `shift`. Ends when the edges' offset goes below 0, the shape's
 * reaches 0 or the lines run out.
 */
static void lines_from_shape(uint16_t edge_at, uint8_t lines, uint8_t half, uint8_t shift)
{
    uint16_t y = window_shape_last();
    int32_t x = edge_at;

    for (;;) {
        uint8_t left = window_shape_byte(y), right = window_shape_byte((uint16_t)(y + 1));
        uint8_t mid = (uint8_t)((uint8_t)(right - left) / 2 + left);
        uint8_t l = (uint8_t)(mid - half), r = (uint8_t)(mid + half);

        if (l < left)
            l = left;
        if (r >= right)
            r = right;
        if (WINDOW_EDGES + x + 1 < LINE_DATA_SIZE) {
            g.line_data[WINDOW_EDGES + x] = (uint8_t)(l + shift);
            g.line_data[WINDOW_EDGES + x + 1] = (uint8_t)(r + shift);
        }
        x -= 2;
        if (x < 0)
            break;
        y = (uint16_t)(y - 2);
        if (!y)
            break;
        if (!--lines)
            break;
    }
}

/* window effect 6's states (WindowEffectTask's resume points) */
enum { OPENING_STEP = 30, OPENING_PAST_NEXT };

#define OPENING_EDGES_AT 0x0100     /* the edges of the line the opening works up from */

/* effect 6, then past the shape: a pair a frame, the half-height up by one
   (to $80) and halved, until the edges reach the shape's last pair */
static TaskRun opening_past(Task *t, uint16_t *at)
{
    uint8_t half = (uint8_t)(t->own.opening.half + 1);

    if (half >= 0x80)
        half = 0x80;
    t->own.opening.half = half;
    lines_from_shape(t->own.opening.edge_at, t->own.opening.lines, (uint8_t)(half >> 1), 0);
    *at = OPENING_PAST_NEXT;
    return colours_wait(t);
}

/* effect 6, opening from the middle: 3 more lines a frame while under $80 */
static TaskRun opening_step(Task *t, uint16_t *at)
{
    t->own.opening.lines = (uint8_t)(t->own.opening.lines + 3);
    if (t->own.opening.lines >= 0x80) {
        t->own.opening.edge_at = OPENING_EDGES_AT;
        return opening_past(t, at);
    }
    lines_from_shape(OPENING_EDGES_AT, t->own.opening.lines, t->own.opening.half, 0);
    *at = OPENING_STEP;
    return colours_wait(t);
}

/*
 * @routine 82:819A HdmaEffect6
 * WHAT IT DOES: Window effect 6: a window that opens from the middle of the
 * screen (3 lines a frame), then grows past the shape line by line, then
 * the waves (WindowWavesLoop) for good.
 * HOW: Table set $0A on register set 6 at $30 in the line data; window 1's
 * edges 0; the half sine built; the window table ($7E:F7FE on, $200 bytes)
 * filled with no window ($FF, 0 pairs); the channel on; lines 0, the
 * half-height 4; then opening_step each frame, opening_past, the waves.
 */
static TaskRun effect_6(Task *t, uint16_t *at)
{
    int ch;

    if ((ch = hdma_start(0x0A, 0x06, 0x0030)) == NO_CHANNEL)
        return task_wait(t, 1);
    g.window_edges[0] = 0;
    g.window_edges[1] = 0;
    half_sine_build();
    for (unsigned x = 0; x < WAVES_BYTES; x += 2) {
        g.line_data[WINDOW_EDGES - 2 + x] = 0xFF;
        g.line_data[WINDOW_EDGES - 1 + x] = 0x00;
    }
    channel_on(ch);
    t->own.opening.lines = 0;
    t->own.opening.half = 4;
    return opening_step(t, at);
}

/*
 * @routine 82:870D HdmaTableFill
 * WHAT IT DOES: Fills `words` words of the per-line tables with `word`, from
 * byte `at` of them on.
 */
void HdmaTableFill(uint16_t at, uint16_t word, unsigned words)
{
    for (unsigned i = 0; i < words; i++, at = (uint16_t)(at + 2)) {
        if ((unsigned)at + 1 < LINE_DATA_SIZE) {
            g.line_data[at] = (uint8_t)word;
            g.line_data[at + 1] = (uint8_t)(word >> 8);
        }
    }
}

/* the play field's scroll, whole pixels */
static uint16_t view_x(void) { return (uint16_t)(g.layers[0].scroll_x >> 8); }
static uint16_t view_y(void) { return (uint16_t)(g.layers[0].scroll_y >> 8); }

/*
 * @routine 82:8417 WindowBoxAt
 * WHAT IT DOES: Adds to the wide table being built from byte `at` (to `end`)
 * the lines of a window 128 pixels wide and up to 96 lines tall whose top left
 * is the level point x, y; returns where the table now ends, `at` itself when
 * the box is off the view (the original's C set, nothing added).
 * HOW: The point less the play field's scroll (across, down) is off when
 * across is outside -128 to the view's width less 1 or down outside -96 to
 * its height less 1 (the original's 255 and 223). Entries of a line count and the
 * window's left and right (16 bits): lines above the box (down above 0) as
 * entries of no window; then the box's lines (96, fewer at the view's bottom,
 * or down + 97 when it starts above it) with the window from across to
 * across + 128, clipped to 0 and the view's last column. (82:8415
 * WindowBoxOff, the original's way off, is this return.)
 */
static unsigned box_at(unsigned at, unsigned end, int32_t x, int32_t y)
{
    int32_t across = (int16_t)(uint16_t)(x - view_x()), down = (int16_t)(uint16_t)(y - view_y()), lines;
    uint16_t left, right;

    if (across < -128 || across >= view.w)
        return at;
    if (down < -96 || down >= view.h)
        return at;
    if (down <= 0) {
        lines = down + 0x61;
    } else {
        at = wide_run(at, end, down, WIDE_NONE, 0);
        lines = view.h - down;
        if (lines >= 0x60)
            lines = 0x60;
    }
    if (across < 0) {
        left = 0;
        right = (uint16_t)(across + 0x80);
    } else if (view.w - across >= 0x81) {
        left = (uint16_t)across;
        right = (uint16_t)(across + 0x80);
    } else {
        left = (uint16_t)across;
        right = (uint16_t)(view.w - 1);
    }
    return wide_run(at, end, lines, left, right);
}

/* the resume points of effects 8-11 (WindowEffectTask's) */
enum { BOX_FRAME = 40, LOWER_FRAME, SPOT_FRAME, SPOT_CLOSE, NARROW_FRAME };

/* window effect 8's boxes' places in the level (the two spots it windows) */
#define BOX1_X 0x0E40
#define BOX1_Y 0x00F8
#define BOX2_X 0x0D60
#define BOX2_Y 0x01D8

/* effect 8: each frame the table in the half not being read is rebuilt: the
   boxes' entries, an entry of 1 line of no window, the end, and window 1's
   edges from its first entry */
static TaskRun box_frame(Task *t, uint16_t *at)
{
    unsigned table = WIDE_BOXES + (t->own.box.flip ? 0x200u : 0u), end = table + 0x200, x = table;

    t->own.box.flip ^= 1;
    g.line_table_at[t->own.box.channel - 4] = (uint16_t)table;
    x = box_at(x, end, BOX1_X, BOX1_Y);
    x = box_at(x, end, BOX2_X, BOX2_Y);
    x = wide_run(x, end, 1, WIDE_NONE, 0);
    wide_end(x);
    g.window_edges[0] = (uint16_t)(g.wide_data[table + 1] | g.wide_data[table + 2] << 8);
    g.window_edges[1] = (uint16_t)(g.wide_data[table + 3] | g.wide_data[table + 4] << 8);
    *at = BOX_FRAME;
    return task_wait(t, 1);
}

/* effect 9: the window below the line $0200 of the level, kept each frame */
#define LOWER_WINDOW_LINE 0x0200
#define LOWER_WINDOW_COUNT 0x10     /* its table's count while shown */

static TaskRun lower_frame(Task *t, uint16_t *at)
{
    uint16_t lines = (uint16_t)(LOWER_WINDOW_LINE - view_y());

    if (LOWER_WINDOW_LINE >= view_y()) {
        if (lines >= 0x00F0)
            lines = 0x00F0;
        lines >>= 1;
        if (lines) {
            g.line_data[0x00] = (uint8_t)lines;
            g.line_data[0x03] = (uint8_t)lines;
            g.line_data[0x100] = LOWER_WINDOW_COUNT;
            *at = LOWER_FRAME;
            return task_wait(t, 1);
        }
    }
    g.line_data[0x100] = 0x00;
    *at = LOWER_FRAME;
    return task_wait(t, 1);
}

/* effect 11: its wide table from the line data the original keeps: the lines of no window the byte at $F480 counts
   (line_data[0x80], which steps up each frame from 1), then twice the count at $F483 (line_data[0x83], which steps
   down) lines of the window from the left edge (line_data[0x92], which steps up) to the view's last column less it,
   then one line of none */
static void narrow_table(void)
{
    unsigned at = WIDE_NARROW, end = WIDE_NARROW + 0x200;
    uint16_t left = g.line_data[0x92];

    at = wide_run(at, end, g.line_data[0x80], WIDE_NONE, 0);
    at = wide_run(at, end, 2 * g.line_data[0x83], left, (uint16_t)(view.w - 1 - left));
    at = wide_run(at, end, 1, WIDE_NONE, 0);
    wide_end(at);
}

/* effect 11: a window narrowing over the lines above $00F8 of the level; its table's edges and line count step each frame */
static TaskRun narrow_frame(Task *t, uint16_t *at)
{
    g.line_data[0x80]++;
    g.line_data[0x92]++;
    g.line_data[0x93] = (uint8_t)~g.line_data[0x92];
    g.line_data[0x83]--;
    g.line_data[0x86] = g.line_data[0x83];
    narrow_table();
    if (!g.line_data[0x83])
        return task_exit(t);
    *at = NARROW_FRAME;
    return task_wait(t, 1);
}

/* effect 10: window and colour maths round a spot of the level */
#define SPOT_X 0x0E78
#define SPOT_Y 0x0144
#define SPOT_TABLE 0x0100           /* its table (the original's $7E:F500), 32 bytes, kept at +$20 */
#define SPOT_KEPT 0x0120
#define SPOT_LINES (WIDE_SPOT + 0x1FE)  /* in g.wide_data: the lines the spot's window covers, 16 bits (the original's two counts, a byte each) */

/* effect 10's wide table: the window from the spot's left edge to its right (window 2's edges) for the lines
   the spot covers, then one line of none; nothing at all (the table at its end) while the spot is off the view */
static void spot_table(void)
{
    unsigned at = WIDE_SPOT, end = WIDE_SPOT + 0x1FE;
    int32_t lines = g.wide_data[SPOT_LINES] | g.wide_data[SPOT_LINES + 1] << 8;

    if (lines) {
        at = wide_run(at, end, lines, g.window_edges[2], g.window_edges[3]);
        at = wide_run(at, end, 1, WIDE_NONE, 0);
    }
    wide_end(at);
}

/* the colour maths and fixed colour effect 10 leaves off */
static void spot_colours_off(void)
{
    g.fixed_red = 0;
    g.fixed_green = 0;
    g.fixed_blue = 0;
    g.blend_prevent = 0;
    g.blend_clip = 0;
    g.blend_use_sub = 1;                /* the sub screen, not the fixed colour */
    g.blend_direct_colour = 0;
    g.blend_layers = 0x13;              /* layers 1, 2 and sprites, halved */
    g.blend_half = 1;
    g.blend_subtract = 0;
}

/* effect 10, closing: the window's edges a step in each frame until they cross; then the colour maths off
   and the channel off, and the task ends */
static TaskRun spot_closing(Task *t, uint16_t *at)
{
    *at = SPOT_CLOSE;
    return task_wait(t, 1);
}

static TaskRun spot_closed_step(Task *t, uint16_t *at)
{
    g.window_edges[2]++;
    g.line_data[SPOT_TABLE + 0x12] = (uint8_t)g.window_edges[2];
    g.window_edges[3]--;
    g.line_data[SPOT_TABLE + 0x13] = (uint8_t)g.window_edges[3];
    spot_table();
    if ((int16_t)g.window_edges[3] >= (int16_t)g.window_edges[2])
        return spot_closing(t, at);
    spot_colours_off();
    g.line_effects_on &= (uint8_t)~(1u << t->own.box.channel);
    return task_exit(t);
}

static TaskRun spot_frame(Task *t, uint16_t *at)
{
    int32_t down = (int16_t)(uint16_t)(SPOT_Y - view_y() - g.layer_offset_y);

    if (down >= 0 && down < view.h) {
        int32_t across = (int16_t)(uint16_t)(SPOT_X - view_x() - g.bg1_x_shift);

        if (across >= 0 && across < view.w) {
            int32_t right = across + 0x10, lines;

            if (right > view.w - 1)
                right = view.w - 1;
            g.line_data[SPOT_KEPT + 0x13] = (uint8_t)right;
            g.window_edges[3] = (uint16_t)right;
            g.line_data[SPOT_KEPT + 0x12] = (uint8_t)across;
            g.window_edges[2] = (uint16_t)across;
            g.line_data[SPOT_KEPT + 0x00] = g.line_data[SPOT_KEPT + 0x03] = (uint8_t)((down >> 1) + (down & 1));
            lines = 2 * ((down >> 1) + (down & 1));
            g.wide_data[SPOT_LINES] = (uint8_t)lines;
            g.wide_data[SPOT_LINES + 1] = (uint8_t)(lines >> 8);
            g.fixed_red = 0x08;
            g.fixed_green = 0x08;
            g.fixed_blue = 0x10;
            g.blend_prevent = 2;                /* the colour maths kept out of the colour window's inside */
            g.blend_clip = 0;
            g.blend_use_sub = 0;
            g.blend_direct_colour = 0;
            g.blend_layers = 0x33;              /* layers 1, 2, sprites and the backdrop, added */
            g.blend_half = 0;
            g.blend_subtract = 0;
            /* the kept table back over the live one (the original queues the move for the NMI) */
            for (unsigned i = 0; i < 0x20; i++)
                g.line_data[SPOT_TABLE + i] = g.line_data[SPOT_KEPT + i];
            spot_table();
            if (g.scene_cue_close)
                return spot_closing(t, at);
            *at = SPOT_FRAME;
            return task_wait(t, 1);
        }
    }
    g.line_data[SPOT_TABLE] = 0x00;
    g.wide_data[SPOT_LINES] = g.wide_data[SPOT_LINES + 1] = 0;
    g.fixed_red = 0;
    g.fixed_green = 0;
    g.fixed_blue = 0;
    g.window_edges[2] = 0x00;
    g.blend_prevent = 0;                        /* the colour maths off as spot_colours_off leaves it */
    g.blend_clip = 0;
    g.blend_use_sub = 1;
    g.blend_layers = 0x13;
    g.blend_half = 1;
    g.blend_subtract = 0;
    spot_table();
    *at = SPOT_FRAME;
    return task_wait(t, 1);
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
 *   effect 6 (82:819A): effect_6.
 *   effects 8, 9, 10, 11 (82:8388, 82:867B, 82:84CB, 82:860B): window boxes, the
 *     window below $0200, the spot window with colour maths, the narrowing window.
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
    case BOX_FRAME:
        return box_frame(t, at);
    case LOWER_FRAME:
        return lower_frame(t, at);
    case SPOT_FRAME:
        return spot_frame(t, at);
    case SPOT_CLOSE:
        return spot_closed_step(t, at);
    case NARROW_FRAME:
        return narrow_frame(t, at);
    case OPENING_STEP:
        return opening_step(t, at);
    case OPENING_PAST_NEXT:
        t->own.opening.lines++;
        t->own.opening.edge_at = (uint16_t)(t->own.opening.edge_at + 2);
        if (t->own.opening.edge_at < window_shape_last())
            return opening_past(t, at);
        return waves_start(t, at);
    default:
        break;
    }
    switch (t->param) {
    case 0x04:
        if ((ch = hdma_start(0x04, 0x06, 0x0000)) == NO_CHANNEL)
            return task_wait(t, 1);
        if (video_level_view())         /* a level's circle: its edges in the view's columns (circle_wipe.c) */
            wide_channel(ch, LINE_WINDOW1_WIDE, WIDE_CIRCLE);
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
        return waves_start(t, at);
    case 0x0C:
        return effect_6(t, at);
    case 0x10:                          /* effect 8: windows round two places in the level */
        if ((ch = hdma_start(0x0E, 0x0E, 0x0000)) == NO_CHANNEL)
            return task_wait(t, 1);
        g.scene_cue_first++;
        wide_channel(ch, LINE_WINDOW1_WIDE, WIDE_BOXES);
        wide_end(WIDE_BOXES);
        channel_on(ch);
        t->own.box.channel = (uint8_t)ch;
        t->own.box.flip = 0;
        *at = BOX_FRAME;
        return task_wait(t, 1);
    case 0x12:                          /* effect 9: a window below the line $0200 of the level */
        if ((ch = hdma_start(0x16, 0x10, 0x0000)) == NO_CHANNEL)
            return task_wait(t, 1);
        g.line_data[0x100] = LOWER_WINDOW_COUNT;
        g.line_data[0x101] = 0x00;
        channel_on(ch);
        *at = LOWER_FRAME;
        return task_wait(t, 1);
    case 0x14:                          /* effect 10: window and colour maths round a spot of the level */
        if ((ch = hdma_start(0x10, 0x0C, 0x0100)) == NO_CHANNEL)
            return task_wait(t, 1);
        wide_channel(ch, LINE_WINDOW2_WIDE, WIDE_SPOT);
        g.wide_data[SPOT_LINES] = g.wide_data[SPOT_LINES + 1] = 0;
        spot_table();
        channel_on(ch);
        t->own.box.channel = (uint8_t)ch;
        g.line_data[0x110] = 0xFF;
        g.line_data[0x111] = 0x00;
        g.window_main = 0;
        g.window_sub = 0;
        for (int i = 0; i < 4; i++)
            g.window_use[i] = 0;
        g.window_use[4] = 0x0;          /* the original's WOBJSEL $C0: the colour window's window 2, on and inverted */
        g.window_use[5] = 0xC;
        for (unsigned i = 0; i < 0x20; i++)
            g.line_data[SPOT_KEPT + i] = g.line_data[SPOT_TABLE + i];
        return spot_frame(t, at);
    case 0x16:                          /* effect 11: a window narrowing over the lines above $00F8 of the level */
        if ((ch = hdma_start(0x12, 0x0C, 0x0080)) == NO_CHANNEL)
            return task_wait(t, 1);
        wide_channel(ch, LINE_WINDOW2_WIDE, WIDE_NARROW);
        channel_on(ch);
        g.window_edges[2] = WIDE_NONE;
        g.window_edges[3] = 0x00;
        g.line_data[0x90] = 0xFF;
        g.line_data[0x91] = 0x00;
        g.line_data[0x92] = 0x00;
        g.line_data[0x93] = 0xFF;
        g.line_data[0x83] = g.line_data[0x86] = (uint8_t)((uint8_t)(0x00F8 - view_y()) + 0x30);
        narrow_table();
        *at = NARROW_FRAME;
        return task_wait(t, 1);
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

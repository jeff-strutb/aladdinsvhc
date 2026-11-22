/*
 * colour_bands.c: the colour band task: the fixed colour set line by line
 * (per-line channel 7), its lower bands pulsing dimmer and back.
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video_tasks.h"

#define BAND_CHANNEL 7
#define BAND_TABLES 0x0100u         /* the two buffers, from $7E:F500 (g.line_data's $100) */
#define DIMMED_FROM 0x22            /* the 18th pair on is dimmed */

/*
 * @routine 81:A08A ColourBandsWrite
 * WHAT IT DOES: Writes the colour bands into the task's spare buffer, the
 * bands from the 18th on dimmed by its step (not below 0), points the
 * channel at it, and flips the buffer.
 * HOW: The table (a line count and a value a pair, direct) is written into
 * the line data as the original's, and the channel pointed at it.
 */
static void colour_bands_write(Task *t)
{
    LineChannel *c = &g.line_channel[BAND_CHANNEL];
    unsigned at = BAND_TABLES + colour_band_buffer(t->own.bands.buffer);
    unsigned n = colour_band_count();

    c->target = LINE_FIXED;
    c->direct = 1;
    c->table_at = (uint16_t)at;
    c->cart.at = 0;
    g.line_table_at[BAND_CHANNEL - 4] = (uint16_t)at;
    for (unsigned i = 0; i < n; i++) {
        uint8_t lines = colour_band_byte(2 * i), value = colour_band_byte(2 * i + 1);

        if (2 * i >= DIMMED_FROM) {
            value = (uint8_t)(value - t->own.bands.step);
            if (value & 0x80)
                value = 0;
        }
        if (at + 2 * i + 1 < LINE_DATA_SIZE) {
            g.line_data[at + 2 * i] = lines;
            g.line_data[at + 2 * i + 1] = value;
        }
    }
    t->own.bands.buffer ^= 1;
}

/*
 * @routine 81:A00C ColourBandTask
 * WHAT IT DOES: A task that shows colour bands down the screen through the
 * fixed colour, its lower bands pulsing for good: dimmed a step every 4
 * frames to 16, held $1F frames, brightened back the same way, held again.
 * HOW: The fixed colour's red 12; colour maths adding to BG2 (the original's
 * 2 and 0); the colour window's window 2 on (its $80); the bands
 * written and channel 7 on, then a frame's wait.
 */
TaskRun ColourBandTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        t->own.bands.step = 0;
        t->own.bands.buffer = 0;
        g.fixed_red = 0x0C;
        g.blend_subtract = 0;
        g.blend_half = 0;
        g.blend_layers = 0x02;
        g.blend_prevent = 0;
        g.blend_clip = 0;
        g.blend_use_sub = 0;
        g.blend_direct_colour = 0;
        g.window_use[4] = 0x0;
        g.window_use[5] = 0x8;
        colour_bands_write(t);
        g.line_effects_on |= 1u << BAND_CHANNEL;
        *at = 1;
        return task_wait(t, 1);
    case 1:                             /* dimming */
        t->own.bands.step++;
        colour_bands_write(t);
        *at = t->own.bands.step == 0x10 ? 2 : 1;
        return task_wait(t, 4);
    case 2:                             /* held dim */
        *at = 3;
        return task_wait(t, 0x1F);
    case 3:                             /* brightening */
        t->own.bands.step--;
        colour_bands_write(t);
        *at = t->own.bands.step == 0 ? 4 : 3;
        return task_wait(t, 4);
    case 4:                             /* held bright */
    default:
        *at = 1;
        return task_wait(t, 0x1F);
    }
}

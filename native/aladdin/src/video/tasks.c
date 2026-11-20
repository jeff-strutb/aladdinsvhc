/*
 * tasks.c: video/'s tasks: the colour fades, the palette sequence, the
 * brightness steps and the Mode 7 zoom. Each wait is a state (system/task.h).
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

/* ---- the colour fades (video/colour_fade.c) ---- */

/* the fade's colour maths: subtract the fixed colour from every layer and the sprites */
#define FADE_LAYERS 0x1F

/* the frames between steps: the task's parameter halved, at least 1 */
static void steps_halved(Task *t)
{
    t->param = (uint8_t)(t->param >> 1);
    if (t->param == 0)
        t->param = 1;
}

/*
 * @routine 81:9F66 ColourFadeBegin
 * WHAT IT DOES: Sets colour maths up for a fade by subtraction on every
 * layer and the sprites (ColourFadeBeginWith 81:9F68 and ColourFadeBeginKeep
 * 81:9F6B, folded in): the fixed colour taken away everywhere, the colour
 * window 2 on; and halves the task's frames between steps.
 */
static void fade_begin(Task *t)
{
    g.blend_subtract = 1;
    g.blend_half = 0;
    g.blend_layers = FADE_LAYERS;
    g.blend_prevent = 0;
    g.blend_clip = 0;
    g.blend_use_sub = 0;
    g.blend_direct_colour = 0;
    g.window_use[4] = 0;                /* the sprites' window use nibble 0, the colour's 8 */
    g.window_use[5] = 0x8;
    steps_halved(t);
}

/*
 * @routine 81:9F7E ColourFadeStep
 * WHAT IT DOES: Sets the fade's grey to `level` (0-31) and waits the
 * task's frames between steps.
 */
static TaskRun fade_step(Task *t, uint16_t *at, uint16_t next, int8_t level)
{
    g.fixed_red = g.fixed_green = g.fixed_blue = (uint8_t)level;
    *at = next;
    return task_wait(t, t->param);
}

/* the end of both fades (81:9F5F): the sprite and colour windows off */
static TaskRun fade_end(Task *t)
{
    g.window_use[4] = 0;
    g.window_use[5] = 0;
    return task_exit(t);
}

/*
 * @routine 81:9F0E ColourFadeInTask
 * WHAT IT DOES: A task that fades the screen in from black by colour maths,
 * then ends: the grey at 31 for a step, a frame's wait, the screen at full
 * brightness, then the grey from 31 down to 0, a step each wait.
 */
TaskRun ColourFadeInTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        fade_begin(t);
        return fade_step(t, at, 1, 31);
    case 1:
        *at = 2;
        return task_wait(t, 1);
    case 2:
        ScreenFullBright();
        t->own.video.step = 31;
        return fade_step(t, at, 3, 31);
    case 3:
        if (--t->own.video.step >= 0)
            return fade_step(t, at, 3, t->own.video.step);
        break;
    }
    return fade_end(t);
}

/*
 * @routine 81:9F52 ColourFadeOutTask
 * WHAT IT DOES: A task that fades the screen out to black by colour maths:
 * the grey from 0 up to 31, a step each wait; then ends.
 */
TaskRun ColourFadeOutTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        fade_begin(t);
        t->own.video.step = 0;
        return fade_step(t, at, 1, 0);
    case 1:
        if (++t->own.video.step < 32)
            return fade_step(t, at, 1, t->own.video.step);
        break;
    }
    return fade_end(t);
}

/* ColourFadeInSprites, ColourFadeInBg1, ColourFadeInBg2, ColourFadeInBg3,
   ColourFadeInBg1Sprites, ColourFadeInBg1Lit, ColourFadeInKeep,
   ColourFadeOutSprites, ColourFadeOutBg1, ColourFadeOutBg2,
   ColourFadeOutBg3, ColourFadeOutBg1Sprites (81:9EDD-81:9F4B): dead, no
   code creates them (the record: each runs only where a unit script
   creates it); see video/NOTES. */

/* ---- the palette sequence (video/palette.c) ---- */

/*
 * @routine 81:9E75 PaletteSequenceTask
 * WHAT IT DOES: A task that shows six palette sets in turn, each for its
 * own frames (the sixth entry first), then ends.
 */
TaskRun PaletteSequenceTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        t->own.video.step = 5;
        break;
    case 1:
        if (--t->own.video.step < 0)
            return task_exit(t);
        break;
    }
    PaletteLoad(palette_sequence_set((unsigned)t->own.video.step));
    *at = 1;
    return task_wait(t, palette_sequence_frames((unsigned)t->own.video.step));
}

/* ---- the brightness steps (video/screen_tasks.c) ---- */

/*
 * @routine 81:9E8E BrightnessUpTask
 * WHAT IT DOES: A task that raises the screen's brightness a step at a
 * time to full, then ends: the screen on (the blank bit cleared), and at
 * full already it ends; else each step waits the parameter halved (at
 * least 1).
 */
TaskRun BrightnessUpTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        g.blank = 0;
        if (g.brightness == 15)
            return task_exit(t);
        steps_halved(t);
        *at = 1;
        return task_wait(t, t->param);
    case 1:
        if (++g.brightness != 15)
            return task_wait(t, t->param);
        break;
    }
    return task_exit(t);
}

/*
 * @routine 81:9EB9 BrightnessDownTask
 * WHAT IT DOES: A task that lowers the screen's brightness a step at a
 * time to 0, then ends, as BrightnessUpTask.
 */
TaskRun BrightnessDownTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        g.blank = 0;
        if (g.brightness == 0)
            return task_exit(t);
        steps_halved(t);
        *at = 1;
        return task_wait(t, t->param);
    case 1:
        if (--g.brightness != 0)
            return task_wait(t, t->param);
        break;
    }
    return task_exit(t);
}

/* ---- the Mode 7 zoom (video/mode7_rotate.c) ---- */

#define ZOOM_ENDED 0x80         /* g.screen_signal: the zoom has ended (the title waits for it) */

/* One frame of the zoom: the centre's y (its low byte) moved by `dy`, the
   scale's low byte down $20; returns whether that borrowed. */
static int zoom_step(int8_t dy)
{
    uint16_t a = (uint16_t)g.m7_matrix[0], y = g.m7_centre_y;
    uint8_t lo = (uint8_t)a;
    int borrow = lo < 0x20;

    g.m7_centre_y = (uint16_t)((y & 0xFF00) | (uint8_t)(y + dy));
    lo = (uint8_t)(lo - 0x20);
    g.m7_matrix[0] = (int16_t)((a & 0xFF00) | lo);
    g.m7_matrix[3] = (int16_t)(((uint16_t)g.m7_matrix[3] & 0xFF00) | lo);
    return borrow;
}

/*
 * @routine 81:B157 Mode7ZoomInTask
 * WHAT IT DOES: A task that zooms the Mode 7 picture in from a speck to
 * full size, the centre drifting up and back, then ends: after 24 frames
 * the scale (the matrix's a and d) $0FF0 and the centre's y $80; each frame
 * the scale down $20 and the centre up a line until the scale's high byte
 * is 1; then down $20 and the centre down a line until it is below $0100;
 * then the scale $0100, the centre's y $40, g.screen_signal $80.
 */
TaskRun Mode7ZoomInTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        *at = 1;
        return task_wait(t, 0x18);
    case 1:
        g.m7_matrix[0] = g.m7_matrix[3] = 0x0FF0;
        g.m7_centre_y = 0x0080;
        *at = 2;
        return task_wait(t, 1);
    case 2:
        if (zoom_step(-1)) {
            uint8_t high = (uint8_t)(((uint16_t)g.m7_matrix[0] >> 8) - 1);

            g.m7_matrix[0] = (int16_t)((uint16_t)high << 8 | ((uint16_t)g.m7_matrix[0] & 0xFF));
            g.m7_matrix[3] = (int16_t)((uint16_t)high << 8 | ((uint16_t)g.m7_matrix[3] & 0xFF));
            if (high == 0x01)
                *at = 3;
        }
        return task_wait(t, 1);
    case 3:
        if (!zoom_step(1))
            return task_wait(t, 1);
        break;
    }
    g.m7_matrix[0] = (int16_t)((uint16_t)g.m7_matrix[0] & 0xFF00);
    g.m7_matrix[3] = (int16_t)((uint16_t)g.m7_matrix[3] & 0xFF00);
    g.m7_centre_y = (uint16_t)((g.m7_centre_y & 0xFF00) | 0x40);
    g.screen_signal = ZOOM_ENDED;
    return task_exit(t);
}

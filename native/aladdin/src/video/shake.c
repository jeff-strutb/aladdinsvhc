/*
 * shake.c: shaking a layer up and down, and wobbling layer 1: tasks.
 */
#include <stdio.h>

#include "data/video_tables.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video_tasks.h"

/* the layer's vertical shift a shake moves, by the shake's layer byte (an
   offset from $1A1C in the original): 0 BG1's, 8 BG2's */
static uint16_t *shake_shift(uint8_t layer)
{
    switch (layer) {
    case 0:
        return &g.layer_offset_y;
    case 8:
        return &g.bg2_y_shift;
    default: {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: a shake of layer byte %02X, which no shake of the record has; left still\n", layer);
        }
        return NULL;
    }
    }
}

/*
 * @routine 81:A1D5 ShakeTask
 * WHAT IT DOES: A task that shakes a layer up and down, the shake its
 * parameter picks (a byte offset in the shake table: the layer, the sways,
 * the frames of a half-sway, the pixels a frame), then ends.
 * HOW: Each sway ShakeUp (81:A1FE), then ShakeDown (81:A21D): each frame of
 * a half-sway the layer's vertical shift moved by the step, then a frame's
 * wait. The record's comment names DB $81 for the table; tasks run with DB
 * $80, and $80:BAA3 is the table.
 */
TaskRun ShakeTask(Task *t)
{
    uint16_t *at = task_enter(t), *shift;
    ShakeEntry e;

    switch (*at) {
    case 0:
        e = shake_entry(t->param);
        t->own.shake.layer = e.layer;
        t->own.shake.sways = e.sways;
        t->own.shake.frames = e.frames;
        t->own.shake.step = e.step;
        *at = 1;                        /* a sway's upward half */
        t->own.shake.left = t->own.shake.frames;
        break;
    case 1:
    case 2:
        if (--t->own.shake.left)
            break;
        if (*at == 1) {
            *at = 2;                    /* the downward half */
        } else {
            if (--t->own.shake.sways == 0)
                return task_exit(t);
            *at = 1;
        }
        t->own.shake.left = t->own.shake.frames;
        break;
    }
    /* this frame of the half-sway: the step, then a frame's wait */
    if ((shift = shake_shift(t->own.shake.layer)) != NULL)
        *shift = (uint16_t)(*at == 2 ? *shift + t->own.shake.step : *shift - t->own.shake.step);
    return task_wait(t, 1);
}

/*
 * @routine 81:9F98 Bg1WobbleTask
 * WHAT IT DOES: A task that wobbles layer 1: its x and y shifts move by a
 * step each frame, the steps turned back after each swing, for the frames
 * the wobble its parameter picks lasts; then the shifts are 0 again and it
 * ends. The record says no code creates it; the cartridge creates it at
 * $83:B2AD (slot 4, parameter 4).
 */
TaskRun Bg1WobbleTask(Task *t)
{
    uint16_t *at = task_enter(t);
    WobbleEntry e;

    switch (*at) {
    case 0:
        e = wobble_entry(t->param);
        t->own.wobble.step_x = e.step_x;
        t->own.wobble.step_y = e.step_y;
        t->own.wobble.swing = e.swing;
        t->own.wobble.left = e.frames;
        t->own.wobble.swing_left = e.swing;
        *at = 1;
        return task_wait(t, 1);
    case 1:
        g.bg1_x_shift = (uint16_t)(g.bg1_x_shift + (uint16_t)t->own.wobble.step_x);
        g.layer_offset_y = (uint16_t)(g.layer_offset_y + (uint16_t)t->own.wobble.step_y);
        if (--t->own.wobble.left == 0) {
            g.bg1_x_shift = 0;
            g.layer_offset_y = 0;
            return task_exit(t);
        }
        if (--t->own.wobble.swing_left == 0) {
            t->own.wobble.step_x = (int16_t)-t->own.wobble.step_x;
            t->own.wobble.step_y = (int16_t)-t->own.wobble.step_y;
            t->own.wobble.swing_left = t->own.wobble.swing;
        }
        return task_wait(t, 1);
    }
    return task_exit(t);
}

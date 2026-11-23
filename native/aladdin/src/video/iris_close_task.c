/*
 * iris_close_task.c: a task that closes the screen down to a circle round
 * a point, by the windows: started by actor kind $24 and by the scene at
 * $80:B04E.
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define WINDOW_EFFECT_SLOT 6        /* the slot of the window effect the iris draws through */
#define WINDOW_EFFECT_CIRCLE 4      /* its parameter: effect 2, the table the circle writes */

/*
 * @routine 80:B263 IrisCloseTask
 * WHAT IT DOES: Closes the screen down to nothing round a point: starts
 * the window effect, masks layers 1 and 2 by the windows, and draws a
 * circle round the point of the iris its parameter names (the original's $00BD, slot 3's parameter byte: 0 the full screen's, 1 the half) whose radius
 * (g.scene_cue) shrinks by one each frame from its start until it is 0,
 * then ends.
 * HOW: The window effect in slot 6 (parameter 4); the main and sub screens'
 * window masks 3, window 1 from 0 to $FF, layers 1 and 2 using it (on and
 * inverted), the rest not. The radius and centre from the iris's tables.
 * Each frame: CircleWipeStep, wait 1, the radius down one; at 0 the task
 * ends.
 */
TaskRun IrisCloseTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        TaskCreate(WINDOW_EFFECT_SLOT, WINDOW_EFFECT_CIRCLE, TASK_ENTRY_WINDOW_EFFECT);
        g.window_main = 0x03;
        g.window_sub = 0x03;
        g.window_edges[0] = 0x00;
        g.window_edges[1] = 0xFF;
        g.window_use[0] = 0x3;
        g.window_use[1] = 0x3;
        for (int i = 2; i < 6; i++)
            g.window_use[i] = 0;
        g.scene_cue = iris_radius(t->param);
        iris_centre(t->param, &t->own.iris.x, &t->own.iris.y);
        break;
    case 1:
        if (!--g.scene_cue)
            return task_exit(t);
        break;
    }
    (void)CircleWipeStep(g.scene_cue, t->own.iris.x, t->own.iris.y);
    *at = 1;
    return task_wait(t, 1);
}

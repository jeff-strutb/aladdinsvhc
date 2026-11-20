/*
 * frame.c: a task's frame ends: fixed waits, and the play frame's update
 * followed by a frame's wait, once or many times. Each is a routine that
 * waits (task.h): its resume point says how far it has got.
 */
#include "game/game.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

/*
 * @routine 80:AEB9 Wait64Frames
 * WHAT IT DOES: Makes the running task wait 64 frames (a little over a
 * second), letting the other tasks run.
 */
TaskRun Wait64Frames(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        *at = 1;
        return task_wait(t, 64);
    }
    return task_return(t);
}

/*
 * @routine 80:AE9A FrameEnd
 * WHAT IT DOES: Ends a task's frame: the frame's update (FrameStep), then
 * the wait for the next frame.
 */
TaskRun FrameEnd(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        FrameStep();
        *at = 1;
        return task_wait(t, 1);
    }
    return task_return(t);
}

/*
 * @routine 80:B2D3 FrameEndTimes
 * WHAT IT DOES: Lets `frames` frames go by, with the frame's update after
 * each wait.
 * HOW: The resume point is the count of frames still to go, so nothing else
 * is kept across the waits.
 */
TaskRun FrameEndTimes(Task *t, unsigned frames)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        *at = (uint16_t)frames;
        return task_wait(t, 1);
    }
    FrameStep();
    if (--*at == 0)
        return task_return(t);
    return task_wait(t, 1);
}

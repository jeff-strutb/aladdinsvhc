/*
 * brightness_wait.c: the screen brightened or darkened by a task of its own
 * in slot 1, the caller's task waiting until it has ended.
 */
#include "system/state.h"
#include "system/task.h"
#include "video/video_tasks.h"

#define BRIGHTNESS_SLOT 1
#define BRIGHTNESS_STEP_FRAMES 4    /* the task's parameter: a step every 2 frames once halved */

/* the task started, then a frame at a time until its slot is free */
static TaskRun brightness_and_wait(Task *t, TaskEntry entry)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case 0:
        TaskCreate(BRIGHTNESS_SLOT, BRIGHTNESS_STEP_FRAMES, entry);
        *at = 1;
        return task_wait(t, 1);
    case 1:
        if (task_busy(BRIGHTNESS_SLOT))
            return task_wait(t, 1);
        break;
    }
    return task_return(t);
}

/*
 * @routine 81:F311 BrightnessUpAndWait
 * WHAT IT DOES: Brings the screen up to full brightness, a step every 2
 * frames (BrightnessUpTask in slot 1, parameter 4), and waits until it is
 * done: TASK_GOES_ON then.
 */
TaskRun BrightnessUpAndWait(Task *t)
{
    return brightness_and_wait(t, TASK_ENTRY_BRIGHTNESS_UP);
}

/*
 * @routine 81:F31C BrightnessDownAndWait
 * WHAT IT DOES: Takes the screen down to dark the same way
 * (BrightnessDownTask), and waits until it is done.
 */
TaskRun BrightnessDownAndWait(Task *t)
{
    return brightness_and_wait(t, TASK_ENTRY_BRIGHTNESS_DOWN);
}

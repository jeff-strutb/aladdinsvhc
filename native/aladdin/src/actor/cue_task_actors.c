/*
 * cue_task_actors.c: actor kinds $24 and $9C, which wait for a cue and then
 * start a task (and show a pose, a sound, a palette cycle).
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "system/task.h"
#include "video/video.h"

enum {
    CUE_D7 = ACTOR_RESUME_OWN,  /* kind $24: each frame until its cue */
    CUE_B7,                     /* then until the second cue */
    STAY,                       /* for good */
    FIRST_WAIT,                 /* kind $9C: after 16 frames, a frame more */
    WAIT_FREE,                  /* each frame until the story's text is free */
    WAIT_LONG                   /* then after 128 frames */
};

/*
 * @routine 84:8000 Kind24Start
 * WHAT IT DOES: Actor kind $24: shown facing right in its first pose until
 * the scene's cue g.genie_cue_d7 reaches $18; then it starts the iris task
 * (slot 3, parameter 1, IrisCloseTask), and once g.genie_cue_b7 is 0 shows
 * its second pose with sound $1C (and the main screen's layers $10), and
 * stays.
 */
int Kind24Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0xB0;
        a->facing = a->flip = 0;
        AnimStart(a, 0);
        a->resume = CUE_D7;
        return 1;
    case CUE_D7:
        if (g.genie_cue_d7 < 0x18)
            return 1;
        TaskCreate(3, 1, TASK_ENTRY_IRIS_CLOSE);
        a->resume = CUE_B7;
        return 1;
    case CUE_B7:
        if (g.genie_cue_b7)
            return 1;
        g.main_layers = 0x10;
        AnimStart(a, 1);
        SoundQueueAdd(0x1C);
        a->resume = STAY;
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 84:A7E5 Kind9CStart
 * WHAT IT DOES: Actor kind $9C: after 16 frames, waits until the story's
 * text task (slot 4) is free, then 128 frames more; it starts the story
 * text's task with scene 5, palette cycle $24 and its pose, and stays.
 */
int Kind9CStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x10;
        a->resume = FIRST_WAIT;
        return 0x10;
    case FIRST_WAIT:
        a->resume = WAIT_FREE;
        return 1;
    case WAIT_FREE:
        if (task_busy(4))
            return 1;
        a->resume = WAIT_LONG;
        return 0x80;
    case WAIT_LONG:
        TaskCreate(4, 5, TASK_ENTRY_STORY_TEXT1);
        PaletteCycleStart(0, 0x24);
        a->flags |= 0x80;
        AnimStart(a, 0);
        a->resume = STAY;
        return 1;
    default:
        return 1;
    }
}

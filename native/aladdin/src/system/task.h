/*
 * task.h: the game's cooperative tasks (docs/aladdin/TASKS.md), each wait an
 * explicit state (plan decision 1).
 *
 * Eight slots, g.tasks[]. A task's entry is a function of its task. It runs
 * on its turn until it waits, starts again elsewhere or ends, and says which
 * by what it returns. A routine it calls that can wait (one to five calls
 * down) returns the same way: TASK_GOES_ON when it has finished and its
 * caller goes on, anything else is passed up the chain unchanged. Each
 * routine on the chain keeps where it stopped in the task (task_enter); the
 * next turn runs the chain from its entry again, and each routine goes on
 * from its own resume point. Values a routine keeps across a wait live in
 * the task's own bytes (Task.own), never in locals.
 *
 * The shape of a routine that waits:
 *
 *     TaskRun Something(Task *t)
 *     {
 *         uint16_t *at = task_enter(t);
 *
 *         switch (*at) {
 *         case 0:
 *             ...;
 *             *at = 1;
 *             return task_wait(t, 60);
 *         case 1:
 *             ...;
 *         }
 *         return task_return(t);
 *     }
 *
 * and a call of one: `r = Something(t); if (r != TASK_GOES_ON) return r;`.
 */
#ifndef ALADDIN_TASK_H
#define ALADDIN_TASK_H

#include <stdint.h>

#include "nshvc/nshvc.h"
#include "system/state.h"

/* how a task's turn ended, or a call on its chain */
typedef enum {
    TASK_GOES_ON,               /* the call finished: its caller goes on in the same turn */
    TASK_WAITS,                 /* the task waits (task_wait) */
    TASK_EXECS,                 /* the task starts again at another entry (task_exec) */
    TASK_ENDS                   /* the task ended; its slot is free (task_exit) */
} TaskRun;

/* Every task entry, by what it is (Task.entry). An entry of another area is
   that area's function; system/task.c calls it by its id. Add an entry here
   (ask system/ for it) before creating a task at it. */
typedef enum {
    TASK_ENTRY_NONE,
    TASK_ENTRY_BOOT,                /* 81:8000 BootTask, then TitleStartTask and OpeningStory (system/boot.c) */
    TASK_ENTRY_TITLE,               /* 80:9C91 TitleTask (game/title_task.c) */
    TASK_ENTRY_TITLE_AFTER_FADE_OUT,/* 80:9C79 TitleAfterFadeOut (game/opening_story.c) */
    TASK_ENTRY_COLOUR_FADE_IN,      /* 81:9F0E ColourFadeInTask (video/) */
    TASK_ENTRY_COLOUR_FADE_OUT,     /* 81:9F52 ColourFadeOutTask (video/) */
    TASK_ENTRY_PALETTE_SEQUENCE,    /* 81:9E75 PaletteSequenceTask (video/) */
    TASK_ENTRY_BRIGHTNESS_UP,       /* 81:9E8E BrightnessUpTask (video/) */
    TASK_ENTRY_BRIGHTNESS_DOWN,     /* 81:9EB9 BrightnessDownTask (video/) */
    TASK_ENTRY_MODE7_ZOOM_IN,       /* 81:B157 Mode7ZoomInTask (video/) */
    TASK_ENTRY_WINDOW_EFFECT,       /* 82:8000 WindowEffectTask (video/) */
    TASK_ENTRY_STORY_TEXT,          /* 81:F332 StoryTextTask (game/story_text.c) */
    TASK_ENTRY_OPTIONS,             /* 83:8979 OptionsTask (stage 5) */
    TASK_ENTRY_PASSWORD,            /* 84:AD50 PasswordScreen (stage 5) */
    TASK_ENTRY_GAME_START,          /* 81:8095 GameStartTask (stage 5) */
    TASK_ENTRY_DEMO,                /* 81:8031 DemoTask (stage 5) */
    TASK_ENTRIES
} TaskEntry;

#define TASK_SLOTS 8

/* For the running task's chain (from its entry and from routines it calls). */
uint16_t *task_enter(Task *t);          /* the calling routine's resume point; 0 on its first entry */
TaskRun task_return(Task *t);           /* the calling routine has finished: its point back to 0; TASK_GOES_ON */
TaskRun task_wait(Task *t, uint8_t frames); /* 81:BD31 TaskWait: its next turn after `frames` (0: 256) */
TaskRun task_exec(Task *t, TaskEntry entry);/* 81:BCF6 TaskExec: from its next turn at `entry` */
TaskRun task_exit(Task *t);             /* 81:BD20 TaskExit: the slot freed */

/* For any code. */
void TaskCreate(uint8_t slot, uint8_t param, TaskEntry entry);  /* 81:BCB8 */
void TaskSlotFree(uint8_t slot);        /* 81:BD4D */
void TaskSlotsFree1To6(void);           /* 81:9626 */
int task_busy(uint8_t slot);            /* the slot holds a task (its state not free) */

/* For the loop (system/main.c). */
void tasks_init(void);                  /* PowerOn's part: the slots, and the first task */
void tasks_count_down(void);            /* the NMI's part: each waiting task's count, at 0 ready */
void TaskScheduler(const nshvc_input *input);   /* 81:BC6D: every ready task's turn */

#endif

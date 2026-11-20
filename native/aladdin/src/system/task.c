/*
 * task.c: the game's cooperative tasks and their scheduler (task.h,
 * docs/aladdin/TASKS.md). The original gave each task a stack of its own and
 * switched stacks at each wait; here each wait is a state of the routines on
 * the task's chain, and a turn is a call of the task's entry.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/game.h"
#include "level/level.h"
#include "video/video_tasks.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

#define TASK_CHAIN_DEPTH 6      /* the entry and five calls below it */

/* the running task's chain: how many routines on it have entered this turn */
static unsigned chain_depth;

/* ALADDIN_TASK_TRACE=1: each task created, started again and ended, with
   its update, on stderr (a check's sight of the boot; the game is the same) */
static void trace(const char *what, unsigned slot, unsigned entry)
{
    static int on = -1;

    if (on < 0) {
        const char *e = getenv("ALADDIN_TASK_TRACE");

        on = e && *e == '1';
    }
    if (on)
        fprintf(stderr, "update %llu: slot %u %s entry %u\n", (unsigned long long)nshvc_updates(), slot, what, entry);
}

uint16_t *task_enter(Task *t)
{
    unsigned d = chain_depth++;

    if (d == 0)
        return &t->resume;
    if (d >= TASK_CHAIN_DEPTH) {
        fprintf(stderr, "task %u: a chain deeper than %u calls\n", (unsigned)g.task_current, TASK_CHAIN_DEPTH - 1);
        nshvc_quit();
        d = TASK_CHAIN_DEPTH - 1;
    }
    return &t->calls[d - 1];
}

TaskRun task_return(Task *t)
{
    unsigned d = --chain_depth;

    if (d == 0)
        t->resume = 0;
    else
        t->calls[d - 1] = 0;
    return TASK_GOES_ON;
}

/*
 * @routine 81:BD31 TaskWait
 * WHAT IT DOES: Makes the running task wait `frames` frames (its next turn
 * comes once the NMI has counted them down), letting the other tasks run.
 * The routines on its chain have kept where they stopped (task_enter); the
 * chain returns TASK_WAITS up to the scheduler.
 */
TaskRun task_wait(Task *t, uint8_t frames)
{
    t->wait = frames;
    t->state = TASK_WAITING;
    return TASK_WAITS;
}

/*
 * @routine 81:BCF6 TaskExec
 * WHAT IT DOES: Starts the running task again at another entry, from its
 * next turn, every routine on its chain left.
 */
TaskRun task_exec(Task *t, TaskEntry entry)
{
    trace("exec", (unsigned)(t - g.tasks), (unsigned)entry);
    t->entry = (uint16_t)entry;
    t->state = TASK_NEW;
    return TASK_EXECS;
}

/* the entry an actor's TaskExec named, until the chain takes it; 0 none */
static uint16_t exec_pending;

void TaskExecRunning(TaskEntry entry)
{
    trace("exec from an actor", (unsigned)g.task_current, (unsigned)entry);
    exec_pending = (uint16_t)entry;
}

bool task_exec_pending(void)
{
    return exec_pending != TASK_ENTRY_NONE;
}

TaskRun task_exec_take(Task *t)
{
    TaskEntry entry = (TaskEntry)exec_pending;

    exec_pending = TASK_ENTRY_NONE;
    return task_exec(t, entry);
}

/*
 * @routine 81:BD20 TaskExit
 * WHAT IT DOES: Ends the running task for good: its slot is freed.
 */
TaskRun task_exit(Task *t)
{
    trace("ends", (unsigned)(t - g.tasks), t->entry);
    TaskSlotFree((uint8_t)(t - g.tasks));
    return TASK_ENDS;
}

/*
 * @routine 81:BCB8 TaskCreate
 * WHAT IT DOES: Starts a new task in a slot: it runs from its entry at its
 * first turn, with the parameter byte its creator gives it.
 */
void TaskCreate(uint8_t slot, uint8_t param, TaskEntry entry)
{
    Task *t = &g.tasks[slot];

    trace("create", slot, (unsigned)entry);
    t->state = TASK_NEW;
    t->param = param;
    t->entry = (uint16_t)entry;
}

/*
 * @routine 81:BD4D TaskSlotFree
 * WHAT IT DOES: Frees a task slot: the scheduler passes over it until a task
 * is created there again.
 */
void TaskSlotFree(uint8_t slot)
{
    g.tasks[slot].state = TASK_FREE;
}

/*
 * @routine 81:9626 TaskSlotsFree1To6
 * WHAT IT DOES: Ends every task but those in slots 0 and 7.
 */
void TaskSlotsFree1To6(void)
{
    uint8_t slot;

    for (slot = 1; slot <= 6; slot++)
        TaskSlotFree(slot);
}

int task_busy(uint8_t slot)
{
    return g.tasks[slot].state != TASK_FREE;
}

/* The task's entry, by its id. */
static TaskRun task_entry_run(Task *t)
{
    switch ((TaskEntry)t->entry) {
    case TASK_ENTRY_BOOT:               return BootTask(t);
    case TASK_ENTRY_TITLE:              return TitleTask(t);
    case TASK_ENTRY_TITLE_AFTER_FADE_OUT: return TitleAfterFadeOut(t);
    case TASK_ENTRY_COLOUR_FADE_IN:     return ColourFadeInTask(t);
    case TASK_ENTRY_COLOUR_FADE_OUT:    return ColourFadeOutTask(t);
    case TASK_ENTRY_PALETTE_SEQUENCE:   return PaletteSequenceTask(t);
    case TASK_ENTRY_BRIGHTNESS_UP:      return BrightnessUpTask(t);
    case TASK_ENTRY_BRIGHTNESS_DOWN:    return BrightnessDownTask(t);
    case TASK_ENTRY_MODE7_ZOOM_IN:      return Mode7ZoomInTask(t);
    case TASK_ENTRY_WINDOW_EFFECT:      return WindowEffectTask(t);
    case TASK_ENTRY_STORY_TEXT:         return StoryTextTask(t);
    case TASK_ENTRY_OPTIONS:            return OptionsTask(t);
    case TASK_ENTRY_PASSWORD:           return PasswordScreen(t);
    case TASK_ENTRY_GAME_START:         return GameStartTask(t);
    case TASK_ENTRY_DEMO:               return DemoTask(t);
    case TASK_ENTRY_IRIS_CLOSE:         return IrisCloseTask(t);
    case TASK_ENTRY_TEXT_BLINK:         return TextBlinkTask(t);
    case TASK_ENTRY_LEVEL_PLAY_START:   return LevelPlayStartTask(t);
    case TASK_ENTRY_NEXT_PART:          return NextPartTask(t);
    case TASK_ENTRY_PLAY_LOOP_START:    return PlayLoopStart(t);
    case TASK_ENTRY_TITLE_START:        return TitleStartTask(t);
    case TASK_ENTRY_LEVEL_END:          return LevelEndTask(t);
    case TASK_ENTRY_ENDING_STORY:       return EndingStory(t);
    case TASK_ENTRY_STORY_CARD:         return StoryCardTask(t);
    case TASK_ENTRY_COLOUR_BAND:        return ColourBandTask(t);
    case TASK_ENTRY_CAMERA_TRIGGER:     return CameraTriggerTask(t);
    case TASK_ENTRY_LEVEL03_SCENE:      return Level03Scene(t);
    case TASK_ENTRY_LEVEL05_END_SCENE:  return Level05EndScene(t);
    case TASK_ENTRY_LEVEL0D_SCENE:      return Level0DScene(t);
    case TASK_ENTRY_LEVEL10_SCENE:      return Level10Scene(t);
    case TASK_ENTRY_BANDS_SPEED_UP:     return BandsSpeedUpTask(t);
    case TASK_ENTRY_GREY_PAST:          return GreyPastTask(t);
    case TASK_ENTRY_RED_OUT:            return RedOutTask(t);
    case TASK_ENTRY_CLIMB_TOP:          return ClimbTopTask(t);
    case TASK_ENTRY_BG1_WOBBLE:         return Bg1WobbleTask(t);
    case TASK_ENTRY_SHAKE:              return ShakeTask(t);
    case TASK_ENTRY_SOUND_WHILE_SLOT1:  return SoundWhileSlot1Task(t);
    case TASK_ENTRY_RISER_COURSES:      return RiserCoursesTask(t);
    case TASK_ENTRY_SOUND_LATER:        return SoundLaterTask(t);
    case TASK_ENTRY_CONTINUE_SCREEN:    return ContinueScreenTask(t);
    case TASK_ENTRY_GENIE_SCREEN:       return GenieScreen(t);
    case TASK_ENTRY_STORY_TEXT1:        return StoryTextTask1(t);
    case TASK_ENTRY_SCENE_SPIN_ZOOM:    return SceneSpinZoomTask(t);
    case TASK_ENTRY_ENDING_STAGES:      return EndingStagesTask(t);
    case TASK_ENTRY_NONE:
    case TASK_ENTRIES:
        break;
    }
    fprintf(stderr, "task %u: no entry %u\n", (unsigned)g.task_current, (unsigned)t->entry);
    return task_exit(t);
}

/*
 * PowerOn's part (80:8000): the slots cleared, the scheduler scanning all 8,
 * the first task (slot 0, parameter 0, BootTask) created.
 */
void tasks_init(void)
{
    memset(g.tasks, 0, sizeof g.tasks);
    g.task_slots = TASK_SLOTS;
    TaskCreate(0, 0, TASK_ENTRY_BOOT);
}

/*
 * The NMI's countdown (80:830B): each waiting task, slot 7 down to 0, a
 * frame less to wait; at 0 it is ready.
 */
void tasks_count_down(void)
{
    int slot;

    for (slot = TASK_SLOTS - 1; slot >= 0; slot--) {
        Task *t = &g.tasks[slot];

        if (t->state == TASK_WAITING && --t->wait == 0)
            t->state = TASK_READY;
    }
}

/*
 * @routine 81:BC6D TaskScheduler
 * @routine 81:BC7F TaskSchedulerNext
 * WHAT IT DOES: Gives every task that is ready or new its turn, scanning
 * the slots from the first: a new task starts at its entry, a waiting one
 * goes on where its chain stopped. Slot 0's turn reads the pads first. A
 * task created or started again in a slot the scan has passed runs when the
 * scan comes round again; the scans go on until one finds no task ready, as
 * the original's scheduler scans until the NMI makes one ready.
 * HOW: The original's restart of the scan at the NMI ($032A) is the next
 * update's: its countdown runs before this.
 */
void TaskScheduler(const nshvc_input *input)
{
    unsigned scans;

    for (scans = 0;; scans++) {
        int ran = 0;
        uint8_t slot;

        if (scans == 1000) {
            fprintf(stderr, "task scheduler: tasks still ready after %u scans of one frame\n", scans);
            nshvc_quit();
            return;
        }
        for (slot = 0; slot < g.task_slots; slot++) {
            Task *t = &g.tasks[slot];

            if (t->state < TASK_READY)
                continue;
            g.task_current = slot;
            if (slot == 0)
                InputRead(input);
            if (t->state == TASK_NEW) {
                t->resume = 0;
                memset(t->calls, 0, sizeof t->calls);
            }
            t->state = TASK_RUNNING;
            chain_depth = 0;
            if (task_entry_run(t) == TASK_GOES_ON && !task_exec_pending()) {
                /* the original's RTL off the task's stack: no entry returns */
                fprintf(stderr, "task %u: entry %u returned\n", (unsigned)slot, (unsigned)t->entry);
                TaskSlotFree(slot);
            }
            if (task_exec_pending()) {
                /* a chain that did not pass an actor's TaskExec up: the exec still
                   happens, as the original's, but the chain ran on past it */
                fprintf(stderr, "task %u: an actor's TaskExec to entry %u not passed up its chain\n",
                        (unsigned)slot, (unsigned)exec_pending);
                (void)task_exec_take(t);
            }
            ran = 1;
        }
        if (!ran)
            return;
    }
}

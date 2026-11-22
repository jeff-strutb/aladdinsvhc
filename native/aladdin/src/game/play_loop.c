/*
 * play_loop.c: the game task's play loop: a frame of play, the frame's
 * checks, the wait for the next frame; a demo ends on Start or its time.
 */
#include "game/game.h"
#include "level/level.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define FADE_SLOT 1
#define PLAY_DEMO 2             /* g.play_mode */
#define START_HIGH 0x10         /* Start, in a pad word's high byte */

enum { START_LOAD, START_LOOP };

/*
 * @routine 81:8191 PlayLoopStart
 * WHAT IT DOES: The game task's play loop from its start: the level
 * loaded, the fade-in task started, then the loop.
 * HOW: LevelLoad; BrightnessUpTask in slot 1 (parameter 4); the play
 * starts mark ($03FA) counted up; on into PlayLoop.
 */
TaskRun PlayLoopStart(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    if (*at == START_LOAD) {
        r = LevelLoad(t);
        if (r != TASK_GOES_ON)
            return r;
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
        g.play_starts++;
        *at = START_LOOP;
    }
    return PlayLoop(t);
}

enum { LOOP_STEP, LOOP_PAUSE, LOOP_WAITED };

/*
 * @routine 81:82CF PlayLoop
 * WHAT IT DOES: The play loop: each turn a frame of play and the frame's
 * checks, then the wait for the next frame (and on while the VRAM queue
 * still waits); a demo ends when Start is pressed or its frames run out,
 * and the title comes back.
 * HOW: FrameStep (an actor's TaskExec ends the turn there), RestartPointCheck,
 * LevelExitCheck, HudRefresh, PauseCheck; a frame's wait, again while
 * vram_queue_pending is set. In a demo: Start newly pressed, or the demo's
 * frames counted down to 0, TaskExec TitleAfterFadeOut. (The original
 * waits g.play_wait ($03FB) frames when that byte is nonzero: the chain
 * scene's jump (PlayerSceneCued) sets it to 8, and the copier check would
 * set it to $3F; else one frame.)
 */
TaskRun PlayLoop(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    for (;;) {
        switch (*at) {
        case LOOP_STEP:
            if (FrameStep())
                return task_exec_take(t);
            RestartPointCheck();
            LevelExitCheck();
            HudRefresh();
            *at = LOOP_PAUSE;
            /* fall through */
        case LOOP_PAUSE:
            r = PauseCheck(t);
            if (r != TASK_GOES_ON)
                return r;
            *at = LOOP_WAITED;
            return task_wait(t, g.play_wait ? g.play_wait : 1);
        case LOOP_WAITED:
            if (g.vram_queue_pending)
                return task_wait(t, g.play_wait ? g.play_wait : 1);
            *at = LOOP_STEP;
            if (g.play_mode == PLAY_DEMO
                && (((g.pad1_pressed >> 8) & START_HIGH) || --g.demo_frames_left == 0))
                return task_exec(t, TASK_ENTRY_TITLE_AFTER_FADE_OUT);
            break;
        }
    }
}

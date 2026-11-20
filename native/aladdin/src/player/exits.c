/*
 * exits.c: Aladdin at a stage's end (the record's player/player_exit.c):
 * the exit walk of an escort level, the wait after a level's end scene,
 * and the exit spot; each fades the screen out, waits for the fade's task
 * and starts the running task again at what follows the stage, or at the
 * Genie screen when kind $27's figures were touched.
 *
 * Not converted, dead: 81:CC80 StageNumberText (nothing calls it; the
 * record's proof). Not kept: $147B, which the spot exit counts up and no
 * code reads.
 */
#include "player/player_own.h"
#include "system/task.h"

#define FADE_SLOT 2
#define EXIT_X_MAX 0x2060       /* the escort walk's x pixels go no further */

static int next_frame(Actor *a, uint16_t at)
{
    a->resume = at;
    return 1;
}

/* the screen's fade out: the song's fade and the brightness task */
static void fade_start(void)
{
    SoundQueueF6With1();
    TaskCreate(FADE_SLOT, 0x04, TASK_ENTRY_BRIGHTNESS_DOWN);
}

/* what follows the stage, by the level's byte: 0 the next part; negative
   the story card where `negative_card`; else the level's end */
static int stage_goes_on_exec(bool negative_card)
{
    uint8_t v = stage_goes_on(g.level);

    if (!v) {
        TaskExecRunning(TASK_ENTRY_NEXT_PART);
        return 1;
    }
    TaskExecRunning(negative_card && (v & 0x80) ? TASK_ENTRY_STORY_CARD : TASK_ENTRY_LEVEL_END);
    return 1;
}

/*
 * @routine 81:E730 PlayerEscortExit
 * WHAT IT DOES: He walks out of an escort level's exit (the scroll passed
 * its spot): facing right at a walk's speed, 32 frames, then the screen
 * fades out while he walks on (never past x $2060), and the running task
 * goes on at the level's end or the story card.
 */
static int escort_exit(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    SecondPoolFreeAll();
    g.play_starts++;
    g.stages_ended++;
    if (g.progress_sequence == 0x01)
        ActorSpeedXSetAccel(a, 0x28, 0x57);
    else
        ActorSpeedXSetAccel(a, 0x16, 0x45);
    g.view_edge_rule = 0x04;
    PlayerMoveStart(a, 0x00);
    POSE_FRAMES(a) = 0x20;
    return next_frame(a, P_ESCORT_WALK);
}

static int escort_walk(Actor *a)
{
    ActorAccelerateX(a);
    ActorMoveX(a);
    if (--POSE_FRAMES(a))
        return 1;
    fade_start();
    return next_frame(a, P_ESCORT_FADE);
}

static int escort_fade(Actor *a)
{
    ActorAccelerateX(a);
    ActorMoveX(a);
    if (EXIT_X_MAX < x_pixels(a))
        set_x_pixels(a, EXIT_X_MAX);
    if (task_busy(FADE_SLOT))
        return 1;
    TaskExecRunning(stage_goes_on(g.level) ? TASK_ENTRY_LEVEL_END : TASK_ENTRY_STORY_CARD);
    return 1;
}

/*
 * @routine 81:E7B1 PlayerStageEnd
 * WHAT IT DOES: At a level's end once its end scene is over: the screen
 * fades out, and the running task goes on at the level's end (or the next
 * part, for a level the table marks so).
 */
static int stage_end(Actor *a)
{
    a->facing = 0;
    a->flip = 0;
    SecondPoolFreeAll();
    g.play_starts++;
    g.stages_ended++;
    fade_start();
    return next_frame(a, P_STAGE_END_FADE);
}

/*
 * @routine 81:CBF9 PlayerSpotExit
 * WHAT IT DOES: At a level's exit spot: the stage's end song, he is put on
 * the ground in pose $19 for 128 frames; then the Genie screen (kind $27's
 * figures touched) or the screen fades out (PlayerSpotExitFade).
 */
static int spot_exit(Actor *a)
{
    SecondPoolFreeAll();
    return next_frame(a, P_SPOT_EXIT_SONG);
}

static int spot_exit_song(Actor *a)
{
    SongPlay(0x28);
    g.screen_signal = 0x80;
    g.play_starts++;
    g.stages_ended++;
    g.player_box_wanted = 0;
    snap_to_ground(a);
    PlayerMoveStart(a, 0x19);
    a->resume = P_SPOT_EXIT_POSED;
    return 0x80;
}

/*
 * @routine 81:CC51 PlayerSpotExitFade
 * WHAT IT DOES: The exit spot's fade: the screen fades out, and the
 * running task goes on at the level's end, the story card or the next
 * part.
 */
static int spot_exit_posed(Actor *a)
{
    if (g.figures_touched)
        return PlayerGoTo(a, PLAYER_AT_SCENE_FIGURE);
    TaskCreate(FADE_SLOT, 0x04, TASK_ENTRY_BRIGHTNESS_DOWN);
    return next_frame(a, P_SPOT_EXIT_FADE);
}

/*
 * @routine 81:CC31 PlayerSceneFigure
 * WHAT IT DOES: A level scene's end with kind $27's figures touched: a kind
 * $27 starts (+$07 8, the crosser), 224 frames, and the running task goes
 * on at the Genie screen. (The original stores into the second pool's
 * record without testing that one was free; here none free starts none.)
 */
static int scene_figure(Actor *a)
{
    Actor *f = ActorAllocB();

    if (f) {
        f->state = ACTOR_STATE_NEW;
        f->kind = 0x27;
        f->spawn_byte = 0x08;
    }
    a->resume = P_SCENE_FIGURE_DONE;
    return 0xE0;
}

int player_exit_code(Actor *a)
{
    switch (a->resume) {
    case PLAYER_AT_ESCORT_EXIT:     return escort_exit(a);
    case PLAYER_AT_STAGE_END:       return stage_end(a);
    case PLAYER_AT_SPOT_EXIT:       return spot_exit(a);
    case PLAYER_AT_SCENE_FIGURE:    return scene_figure(a);
    case P_ESCORT_WALK:             return escort_walk(a);
    case P_ESCORT_FADE:             return escort_fade(a);
    case P_STAGE_END_FADE:
        if (task_busy(FADE_SLOT))
            return 1;
        return stage_goes_on_exec(false);
    case P_SPOT_EXIT_SONG:          return spot_exit_song(a);
    case P_SPOT_EXIT_POSED:         return spot_exit_posed(a);
    case P_SPOT_EXIT_FADE:
        if (task_busy(FADE_SLOT))
            return 1;
        return stage_goes_on_exec(true);
    case P_SCENE_FIGURE_DONE:
        TaskExecRunning(TASK_ENTRY_GENIE_SCREEN);
        return 1;
    default:                        return player_unconverted(a, "a resume point of exits.c");
    }
}

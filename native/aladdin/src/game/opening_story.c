/*
 * opening_story.c: the story told before the title screen: six scenes,
 * each a picture with its actors placed in it, held for a set number of
 * frames (Start skips the rest), faded in and out; then the title screen's
 * task. Its task runs in slot 0: the slot's parameter is the scene.
 */
#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

#define FADE_SLOT 1             /* the screen's fades run in slot 1 */
#define STORY_TEXT_SLOT 4       /* the story's text, in slot 4 */
#define WINDOW_SLOT 6           /* the scenes' window effect, in slot 6 */
#define STORY_LEVEL 0x35        /* the level whose set-up the story loads */
#define SCENES 6
#define SCENE_SKIPPED 0x80      /* the scene number once Start skips: past the last */

/*
 * @routine 80:AEA5 ScreenFadeInStart
 * WHAT IT DOES: Starts the screen fading in, over the next frames: the
 * fade-in task (BrightnessUpTask) in slot 1, parameter 4.
 */
void ScreenFadeInStart(void)
{
    TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
}

/*
 * @routine 80:AEAF ScreenFadeOutStart
 * WHAT IT DOES: Starts the screen fading out, over the next frames: the
 * fade-out task (BrightnessDownTask) in slot 1, parameter 4.
 */
void ScreenFadeOutStart(void)
{
    TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
}

/*
 * @routine 80:AEC0 OpeningActorPlace
 * WHAT IT DOES: Places one of the opening story's actors: entry `entry` of
 * its table gives the kind, the byte its code reads at $07, and the place.
 */
void OpeningActorPlace(unsigned entry)
{
    OpeningActor a = opening_actor(entry);

    ActorSpawnAt(a.kind, a.spawn_byte, a.x, a.y);
}

/*
 * @routine 80:ADB6 OpeningScene1Start
 * WHAT IT DOES: Sets up the first scene: its song ($1C) and the story's
 * text task in slot 4 (parameter 0).
 */
static TaskRun OpeningScene1Start(Task *t)
{
    (void)t;
    SongPlay(0x1C);
    TaskCreate(STORY_TEXT_SLOT, 0, TASK_ENTRY_STORY_TEXT);
    return TASK_GOES_ON;
}

/*
 * @routine 80:ADC6 OpeningScene2Start
 * WHAT IT DOES: Sets up the second scene: its song ($1F) and four actors
 * (entries 0, 2, 3 and 1).
 */
static TaskRun OpeningScene2Start(Task *t)
{
    (void)t;
    SongPlay(0x1F);
    OpeningActorPlace(0);
    OpeningActorPlace(2);
    OpeningActorPlace(3);
    OpeningActorPlace(1);
    return TASK_GOES_ON;
}

/* The end the third, fourth and sixth scenes share (80:AE43), in its
   caller's resume point `at` (1 when its own part has been done): the
   window effect's task in slot 6 (parameter $18); a frame (FrameEnd);
   palette $56; palette cycle $1F in record 0; the actor of entry 20. */
enum { SCENE_START, SCENE_END_START, SCENE_END_FRAME };

static TaskRun scene_end(Task *t, uint16_t *at)
{
    TaskRun r;

    if (*at == SCENE_END_START) {
        TaskCreate(WINDOW_SLOT, 0x18, TASK_ENTRY_WINDOW_EFFECT);
        *at = SCENE_END_FRAME;
    }
    r = FrameEnd(t);
    if (r != TASK_GOES_ON)
        return r;
    PaletteLoad(0x56);
    PaletteCycleStart(0, 0x1F);
    OpeningActorPlace(20);
    return task_return(t);
}

/*
 * @routine 80:ADE1 OpeningScene3Start
 * WHAT IT DOES: Sets up the third scene: one actor (entry 7), then the
 * scenes' common end.
 */
static TaskRun OpeningScene3Start(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == SCENE_START) {
        OpeningActorPlace(7);
        *at = SCENE_END_START;
    }
    return scene_end(t, at);
}

/*
 * @routine 80:ADE8 OpeningScene4Start
 * WHAT IT DOES: Sets up the fourth scene: nine actors (entries 10, 11, 13,
 * 12, 4, 6, 8, 21 and 9), then the scenes' common end.
 */
static TaskRun OpeningScene4Start(Task *t)
{
    static const uint8_t entries[9] = { 10, 11, 13, 12, 4, 6, 8, 21, 9 };
    uint16_t *at = task_enter(t);
    unsigned i;

    if (*at == SCENE_START) {
        for (i = 0; i < 9; i++)
            OpeningActorPlace(entries[i]);
        *at = SCENE_END_START;
    }
    return scene_end(t, at);
}

/*
 * @routine 80:AE63 OpeningScene5Start
 * WHAT IT DOES: Sets up the fifth scene: more graphics (set $27, the
 * second table's copy 12, with the NMI off and the screen blanked), an
 * actor (entry 19), the story's text task in slot 4 (parameter 2), and
 * palette cycles $28 and $29 in records 0 and 1.
 */
static TaskRun OpeningScene5Start(Task *t)
{
    (void)t;
    NmiDisable();
    ScreenBlank();
    UnpackSet(0x27);
    VramCopy2(12);
    NmiEnable();
    OpeningActorPlace(19);
    TaskCreate(STORY_TEXT_SLOT, 2, TASK_ENTRY_STORY_TEXT);
    PaletteCycleStart(0, 0x28);
    PaletteCycleStart(1, 0x29);
    return TASK_GOES_ON;
}

/*
 * @routine 80:AE17 OpeningScene6Start
 * WHAT IT DOES: Sets up the sixth scene: seven actors (entries 16, 17, 18,
 * 5, 6, 14 and 15), the story's text task in slot 4 (parameter 5), then
 * the scenes' common end.
 */
static TaskRun OpeningScene6Start(Task *t)
{
    static const uint8_t entries[7] = { 16, 17, 18, 5, 6, 14, 15 };
    uint16_t *at = task_enter(t);
    unsigned i;

    if (*at == SCENE_START) {
        for (i = 0; i < 7; i++)
            OpeningActorPlace(entries[i]);
        TaskCreate(STORY_TEXT_SLOT, 5, TASK_ENTRY_STORY_TEXT);
        *at = SCENE_END_START;
    }
    return scene_end(t, at);
}

/* the six scenes' set-ups (the record's table at $80:ADAA) */
static TaskRun scene_start(Task *t, uint8_t scene)
{
    switch (scene) {
    case 0: return OpeningScene1Start(t);
    case 1: return OpeningScene2Start(t);
    case 2: return OpeningScene3Start(t);
    case 3: return OpeningScene4Start(t);
    case 4: return OpeningScene5Start(t);
    default: return OpeningScene6Start(t);
    }
}

enum { RUN_START, RUN_SET_UP, RUN_FRAME, RUN_FRAME_END };

/*
 * @routine 80:AD78 OpeningSceneRun
 * WHAT IT DOES: Runs one scene of the opening story: sets it up, then lets
 * it play for its frames, or until Start is pressed, which skips the rest
 * of the story (the scene number becomes $80, past the last).
 * HOW: The scene's frames from its table, its set-up, then each frame
 * Start tested, FrameEnd and the count down. The two words after the count
 * (the story's parts' signals) are cleared as it starts.
 */
TaskRun OpeningSceneRun(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    for (;;) {
        switch (*at) {
        case RUN_START:
            t->own.story.part3_done = 0;
            t->own.story.part2_done = 0;
            t->own.story.frames_left = opening_scene_frames(t->param);
            *at = RUN_SET_UP;
            /* fall through */
        case RUN_SET_UP:
            r = scene_start(t, t->param);
            if (r != TASK_GOES_ON)
                return r;
            *at = RUN_FRAME;
            /* fall through */
        case RUN_FRAME:
            if (g.pad_pressed[0] & PAD_START) {
                t->param = SCENE_SKIPPED;
                return task_return(t);
            }
            *at = RUN_FRAME_END;
            /* fall through */
        case RUN_FRAME_END:
            r = FrameEnd(t);
            if (r != TASK_GOES_ON)
                return r;
            if (--t->own.story.frames_left == 0)
                return task_return(t);
            *at = RUN_FRAME;
            break;
        }
    }
}

/* Each scene's screen, from blank: the story's common graphics and
   palette, then the scene's own when it has a graphics set. */
static void scene_screen(uint8_t scene)
{
    OpeningScene s = opening_scene(scene);

    NmiDisable();
    ScreenBlank();
    OamBufferClear();
    ScrollStateClear();
    LevelTablesClear();
    ActorsFreeAll();
    g.line_effects_on = 0;
    vram_fill(0x4000, 0, 32 * 32);      /* 81:95F5 VramFillMap: BG2's map and BG1's emptied */
    vram_fill(0x4800, 0, 32 * 32);
    TextLayerClear();
    UnpackSet(0x0E);
    VramCopy(18);
    PaletteLoad(0x31);
    if (s.gfx_set) {
        UnpackSet(s.gfx_set);
        VramCopy2(0);
        VramCopy2(s.copy);
        PaletteLoad(s.palette);
    }
    NmiEnable();
}

enum { STORY_START, STORY_WAITED, STORY_DARK, STORY_SCENE, STORY_SCENE_RUN, STORY_SCENE_DARK };

/*
 * @routine 80:AC87 OpeningStory
 * WHAT IT DOES: The opening story: after the screen fades out, six scenes
 * one after another (Start skips to the end), then the title screen.
 * HOW: A frame's wait, sound command $F6 with 1, the fade-out and 64
 * frames; the screen set up for the story (level $35's set-up, the camera
 * held, graphics set 1, palette $5A, layers BG1-3 and sprites on the main
 * screen). For each scene (the slot's parameter, 0 up): its screen, the
 * fade-in, OpeningSceneRun, the fade-out and 64 frames. After the sixth
 * (or a skip): the sound command again, and the task starts again at
 * TitleTask.
 */
TaskRun OpeningStory(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    for (;;) {
        switch (*at) {
        case STORY_START:
            *at = STORY_WAITED;
            return task_wait(t, 1);
        case STORY_WAITED:
            SoundQueueF6With1();
            ScreenFadeOutStart();
            *at = STORY_DARK;
            /* fall through */
        case STORY_DARK:
            r = Wait64Frames(t);
            if (r != TASK_GOES_ON)
                return r;
            NmiDisable();
            ScreenBlank();
            ScrollStateClear();
            OamBufferClear();
            DisplayDefaults();
            g.level = STORY_LEVEL;
            LevelRamSetup();
            g.camera_window = -1;
            UnpackSet(1);
            VramCopy(0);
            PaletteLoad(0x5A);
            NmiEnable();
            g.main_layers = 0x17;
            t->param = 0;
            *at = STORY_SCENE;
            /* fall through */
        case STORY_SCENE:
            scene_screen(t->param);
            ScreenFadeInStart();
            *at = STORY_SCENE_RUN;
            /* fall through */
        case STORY_SCENE_RUN:
            r = OpeningSceneRun(t);
            if (r != TASK_GOES_ON)
                return r;
            ScreenFadeOutStart();
            *at = STORY_SCENE_DARK;
            /* fall through */
        case STORY_SCENE_DARK:
            r = Wait64Frames(t);
            if (r != TASK_GOES_ON)
                return r;
            if (++t->param < SCENES) {
                *at = STORY_SCENE;
                break;
            }
            SoundQueueF6With1();
            return task_exec(t, TASK_ENTRY_TITLE);
        }
    }
}

enum { AFTER_START, AFTER_FADING, AFTER_TITLE };

/*
 * @routine 80:9C79 TitleAfterFadeOut
 * WHAT IT DOES: A task's entry: ends the other tasks, fades the screen out
 * and waits for the fade to end, then goes on as the title screen's task.
 */
TaskRun TitleAfterFadeOut(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case AFTER_START:
        TaskSlotsFree1To6();
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
        *at = AFTER_FADING;
        return task_wait(t, 1);
    case AFTER_FADING:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        *at = AFTER_TITLE;
        break;
    }
    return TitleTask(t);
}

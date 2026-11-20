/*
 * story_scenes.c: the ending's story (EndingStory, run from slot 0): six
 * scenes by slot 0's parameter, each set up by its own routine, faded in
 * and out, with 64 frames between scenes. A scene may leave a sound
 * command for its end. Its first four bytes are the figures' (kind $9B's
 * parts) counts, Task.own.ending_story.
 *
 * StoryFrameEnd (80:B170), StoryFadeInStart (80:B17B), StoryFadeOutStart
 * (80:B185) and StoryWait64Frames (80:B18F) are the same code as FrameEnd,
 * ScreenFadeInStart, ScreenFadeOutStart and Wait64Frames (system/,
 * opening_story.c): the calls here are those.
 */
#include "audio/sound.h"
#include "data/game_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define FADE_SLOT 1
#define IRIS_SLOT 3
#define TEXT_SLOT 4
#define WINDOW_SLOT 6
#define STORY_LEVEL 0x39
#define SCENES ENDING_SCENES
#define SHOW_LAYERS 0x17            /* BG1-BG3 and the sprites */
#define END_SCENE_WAIT 0x40
#define FIGURES_FIRST 1             /* scene 5's actors: entries 1-5 */
#define FIGURES_LAST 5
#define MODE7_SCENE_ROW_FLAG 0x20
#define MODE7_FIRST_COLOUR 0x2048
#define IRIS_FADE_SPEED 0x18

/*
 * @routine 80:B196 StoryActorPlace
 * WHAT IT DOES: Places entry `entry` of the story scenes' actors: a new
 * actor of its kind, or, for kind 0, the player.
 * HOW: A kind: spawned at its place with its $07 byte. Kind 0: Aladdin's
 * record is set as the scene needs him: kind byte (+$06) cleared, $07 the
 * byte, the low bytes of x and y its whole pixels' with the fractions
 * cleared, his state to start ($0C).
 */
static void story_actor_place(unsigned entry)
{
    OpeningActor e = ending_actor(entry);
    Actor *p = &g.actors[0];

    if (e.kind) {
        ActorSpawnAt(e.kind, e.spawn_byte, e.x, e.y);
        return;
    }
    p->kind = 0;
    p->spawn_byte = e.spawn_byte;
    p->x = (p->x & 0xFFFF0000u) | (uint32_t)(e.x & 0xFF) << 8;
    p->y = (p->y & 0xFFFF0000u) | (uint32_t)(e.y & 0xFF) << 8;
    p->state = 0x0C;
}

enum { S_START, S_WAITED };

/* the first scene's set-up (80:B09E) */
static TaskRun scene1_start(Task *t)
{
    (void)t;
    SongPlay(0x32);
    TaskCreate(TEXT_SLOT, 0, TASK_ENTRY_STORY_TEXT1);
    return TASK_GOES_ON;
}

/* the second's (80:B0AE): sound command $F6 with 2 as it ends, after 64 frames */
static TaskRun scene2_start(Task *t)
{
    t->own.ending_story.end_sound = 2;
    t->own.ending_story.end_wait = END_SCENE_WAIT;
    TaskCreate(TEXT_SLOT, 3, TASK_ENTRY_STORY_TEXT1);
    return TASK_GOES_ON;
}

/* the third's (80:B0C2): song $34, its actors from the first entry, the text, palette cycle $14 in record 0 */
static TaskRun scene3_start(Task *t)
{
    (void)t;
    SongPlay(0x34);
    story_actor_place(0);
    TaskCreate(TEXT_SLOT, 4, TASK_ENTRY_STORY_TEXT1);
    PaletteCycleStart(0, 0x14);
    return TASK_GOES_ON;
}

/* the fourth's (80:B0DF): song $23, its actor (entry 6), a frame, the Mode 7 screen,
   the spinning zoom task, and the first colour set */
static TaskRun scene4_start(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == S_START) {
        SongPlay(0x23);
        story_actor_place(6);
        *at = S_WAITED;
        return task_wait(t, 1);
    }
    Mode7ScreenSetupKept(MODE7_SCENE_ROW_FLAG);
    TaskCreate(TEXT_SLOT, 0, TASK_ENTRY_SCENE_SPIN_ZOOM);
    g.palette[0] = MODE7_FIRST_COLOUR;
    g.palette_upload++;
    return task_return(t);
}

/* the fifth's (80:B10F): the figures' counts cleared, five actors, the window
   effect task, song $24, a frame, palette $5B, the text, the sound and wait at its end */
static TaskRun scene5_start(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    if (*at == S_START) {
        t->own.ending_story.figures_in_place = 0;
        t->own.ending_story.part3_done = 0;
        t->own.ending_story.parts_go = 0;
        t->own.ending_story.part2_done = 0;
        for (unsigned e = FIGURES_FIRST; e <= FIGURES_LAST; e++)
            story_actor_place(e);
        TaskCreate(WINDOW_SLOT, 0x18, TASK_ENTRY_WINDOW_EFFECT);
        SongPlay(0x24);
        *at = S_WAITED;
    }
    r = FrameEnd(t);
    if (r != TASK_GOES_ON)
        return r;
    PaletteLoad(0x5B);
    TaskCreate(TEXT_SLOT, 6, TASK_ENTRY_STORY_TEXT1);
    t->own.ending_story.end_sound = 2;
    t->own.ending_story.end_wait = END_SCENE_WAIT;
    return task_return(t);
}

/* the sixth's (80:B160): song $13 and the text */
static TaskRun scene6_start(Task *t)
{
    (void)t;
    SongPlay(0x13);
    TaskCreate(TEXT_SLOT, 0x13, TASK_ENTRY_STORY_TEXT1);
    return TASK_GOES_ON;
}

/* the six set-ups (the record's table at 80:B092) */
static TaskRun scene_start(Task *t, uint8_t scene)
{
    switch (scene) {
    case 0: return scene1_start(t);
    case 1: return scene2_start(t);
    case 2: return scene3_start(t);
    case 3: return scene4_start(t);
    case 4: return scene5_start(t);
    default: return scene6_start(t);
    }
}

enum { RUN_START, RUN_SET_UP, RUN_FRAME };

/*
 * @routine 80:B073 StorySceneRun
 * WHAT IT DOES: Runs one scene of the ending's story: its set-up, then a
 * frame at a time for its frames.
 */
static TaskRun StorySceneRun(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    for (;;) {
        switch (*at) {
        case RUN_START:
            t->own.ending_story.frames_left = ending_scene_frames(t->param);
            *at = RUN_SET_UP;
            /* fall through */
        case RUN_SET_UP:
            r = scene_start(t, t->param);
            if (r != TASK_GOES_ON)
                return r;
            *at = RUN_FRAME;
            /* fall through */
        case RUN_FRAME:
            r = FrameEnd(t);
            if (r != TASK_GOES_ON)
                return r;
            if (--t->own.ending_story.frames_left != 0)
                break;
            return task_return(t);
        }
    }
}

/* Each scene's screen, from blank: the story's common graphics and palette,
   then the scene's own when it has a graphics set (which clears the end
   sound and wait). */
static void scene_screen(Task *t, uint8_t scene)
{
    OpeningScene s = ending_scene(scene);

    g.main_layers = SHOW_LAYERS;
    NmiDisable();
    ScreenBlank();
    OamBufferClear();
    ScrollStateClear();
    LevelTablesClear();
    ActorsFreeAll();
    DisplayDefaults();
    g.line_effects_on = 0;
    VramFillMap(VIDEO_MAP_BACK, 0);
    VramFillMap(VIDEO_MAP_FRONT, 0);
    TextLayerClear();
    g.palette[0] = 0;
    UnpackSet(0x0E);
    VramCopyKept(0, 18);
    PaletteLoad(0x31);
    if (s.gfx_set) {
        UnpackSet(s.gfx_set);
        VramCopyKept(1, 0);
        VramCopyKept(1, 0x4D / 7);
        VramCopyKept(1, s.copy);
        PaletteLoad(s.palette);
        t->own.ending_story.end_sound = 0;
        t->own.ending_story.end_wait = 0;
    }
    NmiEnable();
}

enum { E_START, E_WAITED, E_DARK, E_SCENE, E_RUN, E_END_WAIT, E_SCENE_DARK, E_IRIS };

/*
 * @routine 80:AF42 EndingStory
 * WHAT IT DOES: The ending's story: after the screen fades out, six scenes
 * one after another, each set up and run for its frames (a scene may leave
 * a sound command for its end), faded in and out; then the iris task, a
 * sound command, the fade-out, and the ending's stages task.
 * HOW: Sound $F6 with 1, the fade-out and 64 frames; the screen blanked and
 * cleared, level $39 set up, the camera window $FF, graphics set 1, palette
 * $5A. For each scene (the task's parameter, 0 up): its screen, the fade-in,
 * StorySceneRun, then the end sound and its wait when set, the fade-out and
 * 64 frames. After the sixth: IrisCloseTask in slot 3, sound $F6 with 1,
 * the fade-out task (parameter $18) in slot 1, frames until it is done, and
 * the task starts again at EndingStagesTask.
 */
TaskRun EndingStory(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    for (;;) {
        switch (*at) {
        case E_START:
            SoundQueueF6(1);
            ScreenFadeOutStart();
            *at = E_WAITED;
            /* fall through */
        case E_WAITED:
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
            t->param = 0;
            *at = E_SCENE;
            /* fall through */
        case E_SCENE:
            scene_screen(t, t->param);
            ScreenFadeInStart();
            *at = E_RUN;
            /* fall through */
        case E_RUN:
            r = StorySceneRun(t);
            if (r != TASK_GOES_ON)
                return r;
            if (++t->param >= SCENES) {
                TaskCreate(IRIS_SLOT, 0, TASK_ENTRY_IRIS_CLOSE);
                SoundQueueF6With1();
                TaskCreate(FADE_SLOT, IRIS_FADE_SPEED, TASK_ENTRY_BRIGHTNESS_DOWN);
                *at = E_IRIS;
                return task_wait(t, 1);
            }
            if (t->own.ending_story.end_sound) {
                SoundQueueF6(t->own.ending_story.end_sound);
                *at = E_END_WAIT;
                return task_wait(t, t->own.ending_story.end_wait);
            }
            ScreenFadeOutStart();
            *at = E_SCENE_DARK;
            break;
        case E_END_WAIT:
            ScreenFadeOutStart();
            *at = E_SCENE_DARK;
            /* fall through */
        case E_SCENE_DARK:
            r = Wait64Frames(t);
            if (r != TASK_GOES_ON)
                return r;
            *at = E_SCENE;
            break;
        case E_IRIS:
            if (task_busy(FADE_SLOT))
                return task_wait(t, 1);
            return task_exec(t, TASK_ENTRY_ENDING_STAGES);
        }
    }
}

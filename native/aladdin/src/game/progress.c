/*
 * progress.c: the first task, from power-on to the title; the state a new
 * game and a demo start with, and the level the game's progress leads to.
 *
 * Not converted: NextPartFadeTask (81:8169): nothing in the game starts it
 * (no TaskCreate, TaskExec or jump names it; the record's own note).
 */
#include "audio/sound.h"
#include "data/level_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

enum { BOOT_START, BOOT_WAITED, BOOT_TEXT_CARD, BOOT_LOGO, BOOT_TITLE };

/*
 * @routine 81:8000 BootTask
 * WHAT IT DOES: The first task (slot 0, made by PowerOn): the text upload's
 * VRAM address and size set, a frame's wait, then the text card, the
 * sound sets, the logo screen, the NMI on, and on to the title
 * (TitleStartTask, the same task going on).
 * HOW: The region check (84:AFAE RegionCheck) is dead here: it clears $005D
 * and sets it on a PAL console only (the board is NTSC, the branch never
 * runs), and nothing in the game reads $005D; so it is not converted.
 */
TaskRun BootTask(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    switch (*at) {
    case BOOT_START:
        DevStartApply();
        g.text_upload_to = 0x5C00;
        g.text_upload_bytes = 0x0800;
        *at = BOOT_WAITED;
        return task_wait(t, 1);
    case BOOT_WAITED:
        *at = BOOT_TEXT_CARD;
        /* fall through */
    case BOOT_TEXT_CARD:
        r = TextCardScreen(t);
        if (r != TASK_GOES_ON)
            return r;
        SoundSetsLoad();
        if (DevStartEntry() != TASK_ENTRY_NONE)
            return task_exec(t, DevStartEntry());
        *at = BOOT_LOGO;
        /* fall through */
    case BOOT_LOGO:
        r = LogoScreen(t);
        if (r != TASK_GOES_ON)
            return r;
        NmiEnable();
        *at = BOOT_TITLE;
        /* fall through */
    case BOOT_TITLE:
        break;
    }
    return TitleStartTask(t);
}

#define FADE_SLOT 1             /* the brightness tasks run in slot 1 */
#define DEMOS 2
#define DEMO_FRAMES 0x04EC      /* frames a demo plays */
#define PLAY_DEMO 2             /* g.play_mode: the pad read from the demo's script */
#define START_RANDOM 0x01C3     /* $032B-$032C at play's start */
#define START_LIVES 3
#define START_APPLES 0x10       /* two decimal digits */
#define CHEAT_HEALTH_MARKS 10
#define CHEAT_APPLES 0x99

enum { DEMO_FADE, DEMO_FADING, DEMO_PLAY };

/*
 * @routine 81:8031 DemoTask
 * WHAT IT DOES: The demo's task (the title's task goes on as it): the
 * screen darkened, the demo's state set, the next demo's number for next
 * time, the demo's length, then play (DemoPlayStart).
 * HOW: BrightnessDownTask in slot 1 (parameter 4), a frame at a time until
 * the slot is free; DemoStart; the demo number up, back to 0 at 2; the
 * demo's frames $04EC.
 */
TaskRun DemoTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case DEMO_FADE:
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
        *at = DEMO_FADING;
        return task_wait(t, 1);
    case DEMO_FADING:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        DemoStart();
        if (++g.demo_number == DEMOS)
            g.demo_number = 0;
        g.demo_frames_left = DEMO_FRAMES;
        *at = DEMO_PLAY;
        /* fall through */
    case DEMO_PLAY:
        break;
    }
    return DemoPlayStart(t);
}

/*
 * @routine 81:8066 DemoStart
 * @routine 81:8062 DemoStartLong
 * WHAT IT DOES: Sets a demo up to play: demo g.demo_number's level (its
 * progress sequence, stage and restart point), and its pad script from the
 * first entry.
 * HOW: The play mode 2 with 1 frame left on the current entry; the script's
 * start, sequence, stage and restart point from the demos' tables.
 */
void DemoStart(void)
{
    uint8_t demo = g.demo_number;

    g.play_mode = PLAY_DEMO;
    g.demo_input_frames = 1;
    g.demo_input_at = demo_input_start(demo);
    g.progress_sequence = demo_sequence(demo);
    g.progress_stage = demo_stage(demo);
    g.level_restart = demo_restart(demo);
}

enum { GAME_START, GAME_NEW, GAME_GOES_ON };

/*
 * @routine 81:8095 GameStartTask
 * WHAT IT DOES: The game task's start of play from the title: a new game
 * when none has begun, else play goes on with the game's progress.
 * HOW: The player's flags cleared. None begun: NewGameState, the game
 * begun, and on into LevelPlayStartTask. Else: LevelFromProgress, the lives
 * from the level's start count, the player's flags, the second counter,
 * the hover and the level's flags cleared, PlayStartCounts, then the play
 * loop's start.
 */
TaskRun GameStartTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == GAME_START) {
        g.player_flags = 0;
        if (!g.game_begun) {
            NewGameState();
            g.game_begun++;
            *at = GAME_NEW;
        } else {
            LevelFromProgress();
            g.lives = g.lives_start;
            g.player_flags = 0;
            g.hud_count_2 = 0;
            g.hover_on = 0;
            g.level_flags = 0;
            g.opened_flags = 0;
            PlayStartCounts();
            *at = GAME_GOES_ON;
        }
    }
    if (*at == GAME_NEW)
        return LevelPlayStartTask(t);
    return PlayLoopStart(t);
}

/*
 * @routine 81:80A3 LevelPlayStartTask
 * WHAT IT DOES: The game task's start of play at the level the game's
 * progress gives (also after a lost life or a level's end).
 * HOW: LevelFromProgress, PlayStartCounts, the level's flags, the opened
 * flags and the figures touched cleared, then the play loop's start.
 */
TaskRun LevelPlayStartTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        LevelFromProgress();
        PlayStartCounts();
        g.level_flags = 0;
        g.opened_flags = 0;
        g.figures_touched = 0;
        *at = 1;
    }
    return PlayLoopStart(t);
}

/*
 * @routine 81:80D3 DemoPlayStart
 * WHAT IT DOES: The demo's start of play: a new game's state and its
 * counts, then the play loop.
 */
TaskRun DemoPlayStart(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        NewGameState();
        PlayStartCounts();
        *at = 1;
    }
    return PlayLoopStart(t);
}

/*
 * @routine 81:80E0 PlayStartCounts
 * @routine 81:80DC PlayStartCountsLong
 * WHAT IT DOES: Sets what play starts with: the random number's seed and
 * full health; with the cheat flag, 10 health marks, the hover and 99
 * apples.
 * HOW: Random $01C3, the sprite flicker 0; the player's flags, the figures
 * touched and the opened flags cleared.
 */
void PlayStartCounts(void)
{
    g.random = START_RANDOM;
    g.sprite_flicker = 0;
    if (g.cheat_start) {
        g.health_marks = CHEAT_HEALTH_MARKS;
        g.hover_on = 1;
        g.apples = CHEAT_APPLES;
    }
    g.health = g.health_marks;
    g.player_flags = 0;
    g.figures_touched = 0;
    g.opened_flags = 0;
}

/*
 * @routine 81:8118 NewGameState
 * @routine 81:8114 NewGameStateLong
 * WHAT IT DOES: Sets the counts a new game starts with, and the first
 * level.
 * HOW: LevelCountersClear; LevelFromProgress; 3 lives (the start count,
 * shown and left), 3 health marks (shown, and the health), 10 apples
 * (shown too); the second counter, the hover (each shown too), the health
 * marks taken, the figures touched and the opened flags cleared.
 */
void NewGameState(void)
{
    LevelCountersClear();
    LevelFromProgress();
    g.lives_start = g.lives_shown = g.lives = START_LIVES;
    g.health_marks = g.health_shown = g.health = START_LIVES;
    g.apples = g.apples_shown = START_APPLES;
    g.hud_count_2 = g.hud_count_2_shown = 0;
    g.hover_on = g.hover_shown = 0;
    g.level_marks_taken = 0;
    g.figures_touched = 0;
    g.opened_flags = 0;
}

/*
 * @routine 81:8157 LevelFromProgress
 * WHAT IT DOES: Sets the level from the game's progress: the stage of the
 * progress sequence it has reached.
 * HOW: The sequence's first entry plus the stage (8 bits) indexes the
 * progress table again; that byte is the level (the level word's low byte,
 * as the original writes it).
 */
void LevelFromProgress(void)
{
    uint8_t entry = (uint8_t)(progress_first(g.progress_sequence) + g.progress_stage);

    g.level = (uint16_t)((g.level & 0xFF00u) | progress_level(entry));
}

/*
 * @routine 81:8181 NextPartTask
 * WHAT IT DOES: The game task's start of the next part of play (the story
 * card's TaskExec): the level from the game's progress, the level's and
 * the opened flags cleared, then the play loop from its start.
 */
TaskRun NextPartTask(Task *t)
{
    LevelFromProgress();
    g.level_flags = 0;
    g.opened_flags = 0;
    return task_exec(t, TASK_ENTRY_PLAY_LOOP_START);
}

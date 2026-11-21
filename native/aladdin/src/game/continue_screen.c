/*
 * continue_screen.c: the screen between a lost last life and the next try,
 * where the continues left are shown and one can be spent.
 */
#include "audio/sound.h"
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define CONTINUE_LEVEL 0x25         /* the level whose RAM and actors the screen uses */
#define SONG 0x2B
#define FADE_SLOT 1
#define PLAYER_NEW 0x0C             /* a record's state: to start */
#define PLAYER_BYTE_WITH 6          /* his $07 with continues left, and with none */
#define PLAYER_BYTE_WITHOUT 0x0F
#define DIGIT_PLACE 861             /* the continues' digit: a word of the text map (row 26, column 29) */
#define TILE_DIGIT_0 0x30
#define LAYOUT_SCREEN 0x5A
#define LAYOUT_NUMBER 0x3E
#define LAYOUT_CHOICE 0x0A
#define NONE_FIRST_WAIT 0x40        /* frames, then up to $F0 more (Start ends them) when no continues are left */
#define NONE_MORE_WAIT 0xF0
#define BEFORE_FADE_FRAMES 0x28
#define START_HIGH PAD_START

enum {
    K_START, K_NONE_FIRST, K_NONE_LOOP, K_CHOICE, K_BEFORE_FADE, K_FADE
};

/*
 * @routine 80:B441 ContinuesShow
 * WHAT IT DOES: Shows the number of continues left on the continue screen:
 * its digit's tile ($30 plus the count) goes into the text map and the
 * map's upload is asked for.
 */
void ContinuesShow(void)
{
    g.text_map[DIGIT_PLACE] = (uint16_t)((g.text_map[DIGIT_PLACE] & 0xFF00) | (uint8_t)(TILE_DIGIT_0 + g.continues));
    g.text_map_upload++;
}

/* the buffer at $7E:C800 (the tile behaviours) cleared before a level
   starts (80:B44F WramC800Clear) */
static void tile_behaviour_clear(void)
{
    for (unsigned i = 0; i < sizeof g.tile_behaviour; i++)
        g.tile_behaviour[i] = 0;
}

/*
 * @routine 80:B2E3 ContinueScreenTask
 * WHAT IT DOES: The continue screen, after Aladdin's last life is lost: the
 * screen built and faded in with the continues left shown; with none left,
 * a wait (64 frames and up to $F0 more, or until Start); else the choice
 * (Continue or End) by the actor on the screen, a continue spent on
 * Continue. Then the fade-out, and the game goes on (GameStartTask) or the
 * title comes back (TitleStartTask, the progress cleared).
 * HOW: Song $2B; the display shut; level $25's RAM; the apples and the
 * restart point cleared; Aladdin's record new with $07 6 (continues left)
 * or $0F; the continue frames counted up; both maps filled with 0;
 * graphics sets 1, 8 (copy 13, or the second table's copy 17 when none are
 * left), $62, $F5, $0E, $31 and the level's; palettes $60, $4A; BG2's map
 * as BG1's; the tile behaviours cleared; text layouts $5A and $3E around
 * the fade-in task; the digit.
 */
TaskRun ContinueScreenTask(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    switch (*at) {
    case K_START:
        SongPlay(SONG);
        NmiDisable();
        ScreenBlank();
        g.level = CONTINUE_LEVEL;
        LevelRamSetup();
        g.apples = 0;
        g.level_restart = 0;
        g.actors[0].spawn_byte = g.continues ? PLAYER_BYTE_WITH : PLAYER_BYTE_WITHOUT;
        g.actors[0].state = PLAYER_NEW;
        g.continue_frames++;
        VramFillMap(VIDEO_MAP_BACK, 0);
        VramFillMap(VIDEO_MAP_FRONT, 0);
        UnpackSet(1);
        VramCopy(0);
        UnpackSet(8);
        if (g.continues)
            VramCopy(0x5B / 7);
        else
            VramCopy2(0x77 / 7);
        VramCopy(0x62 / 7);
        VramCopy(0xF5 / 7);
        UnpackSet(0x0E);
        VramCopy(0x7E / 7);
        UnpackSet(0x31);
        VramCopy(0x23 / 7);
        PaletteLoad(0x60);
        PaletteLoad(0x4A);
        g.bg_map[1] = g.bg_map[0];
        LevelGraphicsLoadAll();
        NmiEnable();
        tile_behaviour_clear();
        TextLayoutShow(LAYOUT_SCREEN);
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
        TextLayoutShow(LAYOUT_NUMBER);
        ContinuesShow();
        if (!g.continues) {
            g.continue_no = 1;
            g.continue_yes = 0;
            *at = K_NONE_FIRST;
            return FrameEndTimes(t, NONE_FIRST_WAIT);
        }
        g.continue_yes = 0;
        g.continue_no = 0;
        TextLayoutShow(LAYOUT_CHOICE);
        *at = K_CHOICE;
        return FrameEndTimes(t, 1);
    case K_NONE_FIRST:
        r = FrameEndTimes(t, NONE_FIRST_WAIT);
        if (r != TASK_GOES_ON)
            return r;
        t->own.continue_screen.wait_left = NONE_MORE_WAIT;
        *at = K_NONE_LOOP;
        return FrameEndTimes(t, 1);
    case K_NONE_LOOP:
        r = FrameEndTimes(t, 1);
        if (r != TASK_GOES_ON)
            return r;
        if (!(g.pad_pressed[0] & START_HIGH) && --t->own.continue_screen.wait_left != 0)
            return FrameEndTimes(t, 1);
        *at = K_BEFORE_FADE;
        return FrameEndTimes(t, BEFORE_FADE_FRAMES);
    case K_CHOICE:
        r = FrameEndTimes(t, 1);
        if (r != TASK_GOES_ON)
            return r;
        if (!(g.continue_yes | g.continue_no))
            return FrameEndTimes(t, 1);
        if (g.continue_yes) {
            g.continues--;
            ContinuesShow();
        }
        *at = K_BEFORE_FADE;
        return FrameEndTimes(t, BEFORE_FADE_FRAMES);
    case K_BEFORE_FADE:
        r = FrameEndTimes(t, BEFORE_FADE_FRAMES);
        if (r != TASK_GOES_ON)
            return r;
        SoundQueueF6With1();
        TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
        *at = K_FADE;
        return FrameEndTimes(t, 1);
    case K_FADE:
        r = FrameEndTimes(t, 1);
        if (r != TASK_GOES_ON)
            return r;
        if (task_busy(FADE_SLOT))
            return FrameEndTimes(t, 1);
        g.continue_frames = 0;
        if (g.continue_no) {
            g.progress_sequence = 0;
            g.progress_stage = 0;
            g.level = 0;
            g.level_restart = 0;
            g.game_begun = 0;
            return task_exec(t, TASK_ENTRY_TITLE_START);
        }
        g.game_begun = 1;
        g.gem_sets_found = 0;
        g.gem_sets = 0;
        return task_exec(t, TASK_ENTRY_GAME_START);
    }
    return task_exit(t);
}

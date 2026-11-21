/*
 * title_task.c: the title: its start (the opening story first) and its
 * task: the Mode 7 title, the demo or the story when left alone, the menu
 * and the level select.
 */
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

#define FADE_SLOT 1             /* the screen's fades */
#define ZOOM_SLOT 3             /* the Mode 7 zoom */
#define MENU_EFFECT_SLOT 4      /* the menu's window effect */
#define WINDOW_SLOT 6           /* the title's window effect */
#define TITLE_LEVEL 0x40        /* the level whose set-up and map the title shows */
#define TITLE_IDLE_FRAMES 0x0200    /* frames the menu waits before the demo or the story */
#define AFTER_DEMO_IDLE_FRAMES 0x1300
#define ZOOM_ENDED 0x80         /* screen_signal's bit: the zoom task has ended */
#define FULL_SIZE 0x0100        /* Mode 7's a and d at full size (8.8) */

enum { MENU_START, MENU_PASSWORD, MENU_OPTIONS };

/* An actor's place in whole pixels, its fraction kept, as the original's
   16-bit stores of the whole part. */
static void actor_place_whole(Actor *a, uint16_t x, uint16_t y)
{
    a->x = (a->x & 0xFFu) | (uint32_t)x << 8;
    a->y = (a->y & 0xFFu) | (uint32_t)y << 8;
}

/* The play field layer's whole-pixel scroll set to 0, its fraction kept
   (the original's word stores at $1773 and $1777). */
static void scroll_whole_clear(void)
{
    g.layers[0].scroll_x &= 0xFFu;
    g.layers[0].scroll_y &= 0xFFu;
}

/* What every new game starts with, set as the menu comes up. */
static void new_game_counts(void)
{
    g.menu_blink_hidden = 0;
    g.menu_blink_frames = 0;
    g.menu_choice = MENU_START;
    g.menu_choice_was = 0;
    g.menu_cursor_moves = 0;
    g.progress_sequence = 0;
    g.level = 0;
    g.progress_stage = 0;
    if (g.cheat_start)
        LevelNumberText();
    g.health_marks = 3;
    g.lives_start = 3;
    g.apples = 0x10;
    g.figures_touched = 0;
    g.hover_on = 0;
    g.hud_count_2 = 0;
    g.continue_frames = 0;
    g.player_flags = 0;
    /* $0370 is cleared too in the original; nothing reads it */
    g.level_marks_taken = 0;
    g.gem_sets_found = 0;
    g.gem_sets = 0;
    g.gem_sets_total = 0;
    g.level_flags = 0;
    g.opened_flags = 0;
    g.continues = 3;
}

enum {
    TITLE_START,        /* 80:9C91 */
    TITLE_SETUP,        /* 80:9C9D */
    TITLE_MODE7,        /* 80:9D1B */
    TITLE_BRIGHTEN,     /* 80:9D30 */
    TITLE_SHOWN,        /* 80:9D51 */
    TITLE_FADE_IN,      /* 80:9DC5: waits for the fade in */
    TITLE_WHITEN,       /* 80:9DE1: the fixed colour up to white */
    TITLE_DARKEN,       /* 80:9E1D: and down, the picture zooming */
    TITLE_FADE,         /* 80:9E4B: the palette faded in */
    TITLE_IDLE,         /* 80:9E8F: after a demo: a press, or the zoom */
    TITLE_IDLE_STEP,
    TITLE_ZOOM,         /* 80:9EAC */
    TITLE_ZOOM_STEP,
    TITLE_PRESSED_STEP, /* 80:9ED1 */
    TITLE_MENU,         /* 80:9F08 */
    TITLE_MENU_SET_UP,  /* 80:9F0E */
    TITLE_MENU_STEP,    /* 80:9FB9 */
    TITLE_CHOSEN,       /* 80:9FFE */
    TITLE_CHOSEN_FADE,  /* 80:A00A */
    TITLE_LEAVING,      /* 80:A017 */
    TITLE_STORY,        /* 80:AC87: the opening story again */
    TITLE_DEMO          /* 81:8031: the demo */
};

/* 80:9EC8: a press during the zoom: the fade down, waited out with the
   frame's update. */
static TaskRun title_pressed(Task *t, uint16_t *at)
{
    TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
    *at = TITLE_PRESSED_STEP;
    return task_wait(t, 1);
}

/*
 * @routine 80:9C91 TitleTask
 * WHAT IT DOES: The title screen: puts up the Mode 7 title, waits (playing
 * the demo or the story after long idles), and runs the menu: game start,
 * password, options; with the cheat byte set, the level select.
 * HOW: Game RAM cleared; the screen built (level $40's set-up and map, the
 * layers, graphics, palette, Mode7ScreenSetup) and brightened. The first
 * time (no demo just played): kind $82's actor, song $14, the fixed colour
 * whitened and darkened while the picture zooms, the palette faded in,
 * palette cycle $22. After a demo: song $12, and the menu waits for a
 * press or the zoom task's end (a press fades down and up again with the
 * window effect). Then the menu: kind $4A's actor, the window effect in
 * slot 4, the menu's text, a new game's counts; each frame the frame's
 * update, the level select (cheat) or the cursor, the countdown (at 0 the
 * demo, or the story after a demo), a press: sound $4F, the fade down, and
 * the task starts again at its choice.
 */
TaskRun TitleTask(Task *t)
{
    uint16_t *at = task_enter(t);
    Actor *a;

    for (;;) {
        switch (*at) {
        case TITLE_START:
            TaskSlotsFree1To6();
            LevelCountersClear();
            GameRamClear();
            *at = TITLE_SETUP;
            /* fall through */
        case TITLE_SETUP:
            g.play_mode = 0;
            g.level_restart = 0;
            NmiDisable();
            ScreenBlank();
            ScrollStateClear();
            OamBufferClear();
            DisplayDefaults();
            TextLayerClear();
            g.level = TITLE_LEVEL;
            LevelRamSetup();
            g.camera_window = -1;
            scroll_whole_clear();
            LevelMapBuild();
            LayersFill();
            g.main_layers = 0;
            g.sub_layers = 0;
            UnpackSet(1);
            VramCopy(0);
            UnpackSet(4);
            VramCopy(3);
            VramCopy(7);
            UnpackSet(0x2F);
            VramCopy(5);
            PaletteLoad(8);
            NmiEnable();
            *at = TITLE_MODE7;
            return task_wait(t, 1);
        case TITLE_MODE7:
            /* 81:AF3B Mode7ScreenSetup, from the sources: set 4's tiles (unpacked at
               $0800, VramCopy 7 sent them to video address 0) and its map ($0000, sent
               to $4000 by VramCopy 3) */
            video_mode7_screen_setup(0, &g.unpacked[0x0800], &g.unpacked[0x0000]);
            g.blank = 0;
            g.brightness = 0;
            FrameStep();
            *at = TITLE_BRIGHTEN;
            return task_wait(t, 0x20);
        case TITLE_BRIGHTEN:
            TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
            g.bg_map[0] = VIDEO_MAP_TEXT;   /* BG1 shows the text layer's map and tiles */
            g.bg_tiles[0] = VIDEO_TILES_TEXT;
            g.bg_tiles[1] = VIDEO_TILES_TEXT;
            g.main_layers = 0x11;
            g.sub_layers = 0x11;
            *at = TITLE_SHOWN;
            return task_wait(t, 1);
        case TITLE_SHOWN:
            if (g.demo_was_last) {
                g.play_countdown = AFTER_DEMO_IDLE_FRAMES;
                SongPlay(0x12);
                *at = TITLE_IDLE;
                break;
            }
            g.play_countdown = TITLE_IDLE_FRAMES;
            ActorsFreeAll();
            g.main_layers = 0;
            a = ActorCreate(0x82);
            if (a) {
                a->state = 0x0C;
                a->look = 0x0900;
                actor_place_whole(a, 0x80, 0x80);
                g.m7_centre_x = 0x80;
                g.m7_centre_y = 0xB0;
                NmiDisable();
                ScreenBlank();
                PaletteLoad(0x4C);
                UnpackSet(0x22);
                VramCopy(36);
                NmiEnable();
                g.blank = 0;
                SongPlay(0x14);
            }
            *at = TITLE_FADE_IN;
            return task_wait(t, 1);
        case TITLE_FADE_IN:
            if (task_busy(FADE_SLOT))
                return task_wait(t, 1);
            g.blend_use_sub = 0;            /* colour maths: the fixed colour added */
            g.blend_clip = 0;
            g.blend_prevent = 0;
            g.blend_direct_colour = 0;
            g.blend_layers = 0x31;          /* to BG1, the sprites and the backdrop */
            g.blend_half = 0;
            g.blend_subtract = 0;
            g.fixed_red = g.fixed_green = g.fixed_blue = 0;
            *at = TITLE_WHITEN;
            return task_wait(t, 1);
        case TITLE_WHITEN:
            g.fixed_green = g.fixed_blue = ++g.fixed_red;
            if (g.fixed_red < 0x1F)
                return task_wait(t, 1);
            PaletteSave(0x0C, 1);
            t->own.title.fade = 0xE1;
            PaletteFadeStep((int8_t)t->own.title.fade, 1);
            g.main_layers = 0x11;
            g.m7_matrix[0] = 0x00D0;
            g.m7_matrix[3] = 0x00D0;
            *at = TITLE_DARKEN;
            return task_wait(t, 1);
        case TITLE_DARKEN:
            FrameStep();
            g.m7_matrix[0]++;               /* the original's 8-bit steps: $D0 up to $EF, never past $FF */
            g.m7_matrix[3]++;
            g.fixed_green = g.fixed_blue = --g.fixed_red;
            if (g.fixed_red != 0)
                return task_wait(t, 1);
            g.sub_layers = 0x10;
            g.blend_use_sub = 1;            /* now the sub screen (the sprites) added */
            g.blend_layers = 0x21;          /* to BG1 and the backdrop */
            *at = TITLE_FADE;
            return task_wait(t, 1);
        case TITLE_FADE:
            FrameStep();
            PaletteFadeStep((int8_t)t->own.title.fade, 1);
            if (++t->own.title.fade != 0) {
                if (g.m7_matrix[0] & 0xFF) {    /* up to full size, $0100 */
                    g.m7_matrix[0]++;
                    g.m7_matrix[3]++;
                }
                return task_wait(t, 1);
            }
            PaletteCycleStart(0, 0x22);
            *at = TITLE_MENU;
            break;
        case TITLE_IDLE:
            if (PadConfirmPressed())
                return title_pressed(t, at);
            *at = TITLE_IDLE_STEP;
            return task_wait(t, 1);
        case TITLE_IDLE_STEP:
            FrameStep();
            if (!g.screen_signal) {
                *at = TITLE_IDLE;
                break;
            }
            TaskCreate(ZOOM_SLOT, 0, TASK_ENTRY_MODE7_ZOOM_IN);
            *at = TITLE_ZOOM;
            break;
        case TITLE_ZOOM:
            if (PadConfirmPressed())
                return title_pressed(t, at);
            *at = TITLE_ZOOM_STEP;
            return task_wait(t, 1);
        case TITLE_ZOOM_STEP:
            FrameStep();
            if (!(g.screen_signal & ZOOM_ENDED)) {
                *at = TITLE_ZOOM;
                break;
            }
            SongPlay(0x13);
            *at = TITLE_MENU;
            break;
        case TITLE_PRESSED_STEP:
            FrameStep();
            if (task_busy(FADE_SLOT))
                return task_wait(t, 1);
            TaskSlotFree(MENU_EFFECT_SLOT);
            TaskSlotFree(WINDOW_SLOT);
            g.line_effects_on = 0;
            TaskCreate(WINDOW_SLOT, 6, TASK_ENTRY_WINDOW_EFFECT);
            TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
            ActorsFreeAll();
            SongPlay(0x13);
            *at = TITLE_MENU;
            break;
        case TITLE_MENU:
            *at = TITLE_MENU_SET_UP;
            return task_wait(t, 1);
        case TITLE_MENU_SET_UP:
            a = ActorCreate(0x4A);
            if (a) {
                a->state = 0x0C;
                actor_place_whole(a, 0x7C, 0xC8);
            }
            g.m7_matrix[0] = FULL_SIZE;
            g.m7_matrix[3] = FULL_SIZE;
            TaskCreate(MENU_EFFECT_SLOT, 8, TASK_ENTRY_WINDOW_EFFECT);
            MenuMarkMove();
            TextLayoutShow(0xA6);
            TextLayoutShow(0x0E);
            TextLayoutShow(0x10);
            TextLayoutShow(0x12);
            new_game_counts();
            *at = TITLE_MENU_STEP;
            return task_wait(t, 1);
        case TITLE_MENU_STEP:
            FrameStep();
            if (!g.cheat_start || !LevelSelect())
                TitleMenuCursor();
            if (--g.play_countdown == 0) {
                SoundQueueF6(4);
                if (g.demo_was_last) {
                    g.demo_was_last = 0;
                    *at = TITLE_STORY;
                } else {
                    g.demo_was_last = 1;
                    *at = TITLE_DEMO;
                }
                break;
            }
            if (!PadConfirmPressed())
                return task_wait(t, 1);
            *at = TITLE_CHOSEN;
            return task_wait(t, 1);
        case TITLE_CHOSEN:
            SoundQueueAdd(0x4F);
            *at = TITLE_CHOSEN_FADE;
            return task_wait(t, 1);
        case TITLE_CHOSEN_FADE:
            TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
            SoundQueueF6With1();
            *at = TITLE_LEAVING;
            return task_wait(t, 1);
        case TITLE_LEAVING:
            if (task_busy(FADE_SLOT))
                return task_wait(t, 1);
            g.line_effects_on = 0;
            TaskSlotsFree1To6();
            if (g.menu_choice == MENU_OPTIONS)
                return task_exec(t, TASK_ENTRY_OPTIONS);
            if (g.menu_choice == MENU_PASSWORD)
                return task_exec(t, TASK_ENTRY_PASSWORD);
            return task_exec(t, TASK_ENTRY_GAME_START);
        case TITLE_STORY:
            return OpeningStory(t);
        case TITLE_DEMO:
            return DemoTask(t);
        }
    }
}

/*
 * @routine 80:9C64 TitleStartTask
 * WHAT IT DOES: The title's start: frees the other tasks, the demo number
 * and the play mode 0, and runs the opening story, which ends by starting
 * TitleTask in this task. (The original's jump into TitleTask after it,
 * $80:9C76, is never reached: OpeningStory never returns.)
 */
TaskRun TitleStartTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        TaskSlotsFree1To6();
        g.demo_number = 0;
        g.play_mode = 0;
        *at = 1;
    }
    return OpeningStory(t);
}

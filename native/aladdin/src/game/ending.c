/*
 * ending.c: the ending: its zoom scene's task, the stages task that shows
 * the levels played back, and the text pages with their extras.
 */
#include "actor/actor.h"
#include "audio/sound.h"
#include "data/game_tables.h"
#include "game/game.h"
#include "level/level.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/parallax.h"
#include "video/scroll_layer.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define CENTRE_X 0x80
#define CENTRE_Y 0x50
#define SPEED_FIRST 4
#define ZOOM_FIRST 8
#define SPEED_EVERY 16              /* frames between speed-ups, and zoom steps */
#define ZOOM_EVERY 0x18
#define SCALED_EACH 1               /* g.mode7_how: each matrix element scaled first */
#define FADE_SLOT 1
#define TEXT_SLOT 5
#define SINGLE_STAGE 0x0C           /* the stage played alone with fewer than 50 items (the first of many is 4) */
#define FIRST_STAGE 4
#define LAST_STAGE 0x0B             /* scrolled across, not played */
#define ITEMS_FOR_ALL 50
#define SONG 0x26
#define SCROLL_FRAMES 0x29          /* the move to the next stage */
#define STAGE_HOLD_FRAMES 0x3F
#define TEXT_PALETTES_A 0x61
#define TEXT_PALETTES_B 0x5F
#define TEXT_BANDS_CHANNEL 5
#define TIMED_PAGES 0x0E            /* the timed pages' place in the page tables (byte offset) */
#define TIMED_LAST 0x0D             /* the timed pages: 13 */
#define PAGE_FIRST_WAIT 0x0200      /* frames before the first timed page */
#define PAGE_WAIT 0x0294            /* frames a timed page is shown */
#define LONG_WAIT 0xFF
#define EXTRA_TIMED_PAGE 12         /* the pages that show the item count */
#define EXTRA_UNTIMED_PAGE 6
#define ITEMS_ALL 70
#define WIPE_ENTRY_LEFT 8           /* the last wipe's steps */
#define WIPE_ENTRY_RIGHT 12
#define END_SCROLL_X 0x0400
#define END_SCROLL_SPEED 3          /* pixels a frame */
#define WINDOW_RIGHT_MAX 0xFF
#define KIND_END_A 0xA7
#define KIND_END_B 0x95
#define KIND_END_B_BYTE 0x1E

enum { SPIN_START, SPIN_WAITED };

/*
 * @routine 80:B1D8 SceneSpinZoomTask
 * WHAT IT DOES: Spins and zooms a Mode 7 scene: the angle steps on by a
 * speed that grows every 16 frames and the zoom grows every 24, the zoom
 * taken as its negative past $7F.
 * HOW: The centre ($80, $50); Mode 7 scaled (g.mode7_how 1); angle 0 (the
 * task's parameter), speed 4, zoom 8, counts 16 and 16. Each frame: the
 * counts step (speed up every 16, zoom up every 24); the zoom (negated when
 * negative) into the four scales; the matrix words a and b shifted left 4,
 * d = a and c = -b; a frame's wait; Mode7Rotate by the angle, then the
 * angle plus the speed. Never ends.
 */
TaskRun SceneSpinZoomTask(Task *t)
{
    uint16_t *at = task_enter(t);
    uint8_t zoom;

    if (*at == SPIN_START) {
        g.m7_centre_x = CENTRE_X;
        g.m7_centre_y = CENTRE_Y;
        g.mode7_how = SCALED_EACH;
        t->param = 0;
        t->own.spin_zoom.speed = SPEED_FIRST;
        t->own.spin_zoom.zoom = ZOOM_FIRST;
        t->own.spin_zoom.speed_count = SPEED_EVERY;
        t->own.spin_zoom.zoom_count = SPEED_EVERY;
    } else {
        Mode7Rotate(t->param);
        t->param = (uint8_t)(t->param + t->own.spin_zoom.speed);
    }
    if (--t->own.spin_zoom.speed_count == 0) {
        t->own.spin_zoom.speed_count = SPEED_EVERY;
        t->own.spin_zoom.speed++;
    }
    if (--t->own.spin_zoom.zoom_count == 0) {
        t->own.spin_zoom.zoom_count = ZOOM_EVERY;
        t->own.spin_zoom.zoom++;
    }
    zoom = (uint8_t)t->own.spin_zoom.zoom;
    if (zoom & 0x80)
        zoom = (uint8_t)(~zoom + 1);
    for (int i = 0; i < 4; i++)
        g.mode7_scale[i] = zoom;
    g.m7_matrix[0] = (int16_t)((uint16_t)g.m7_matrix[0] << 4);
    g.m7_matrix[3] = g.m7_matrix[0];
    g.m7_matrix[1] = (int16_t)((uint16_t)g.m7_matrix[1] << 4);
    g.m7_matrix[2] = (int16_t)(0 - (uint16_t)g.m7_matrix[1]);
    *at = SPIN_WAITED;
    return task_wait(t, 1);
}

/* ---- the play-back's window ---- */

/*
 * @routine 82:B4AF EndingWipeLeftStep
 * WHAT IT DOES: Closes the ending's window from the left by the step at
 * byte offset x: its left edge less the step's amount, not below 0 pixels,
 * into the window's left edge.
 */
static void wipe_left_step(Task *t, uint8_t x)
{
    uint32_t v = t->own.ending_stages.wipe_left - ending_wipe_step(x);

    if (v & 0x00800000u)
        v &= 0xFF0000FFu;
    t->own.ending_stages.wipe_left = v;
    g.window_edges[0] = (uint8_t)(v >> 8);
}

/*
 * @routine 82:B4D5 EndingWipeRightStep
 * WHAT IT DOES: Closes the ending's window from the right by the step at
 * byte offset x: its right edge less the step's amount, at most 255
 * pixels, into the window's right edge.
 */
static void wipe_right_step(Task *t, uint8_t x)
{
    uint32_t v = t->own.ending_stages.wipe_right - ending_wipe_step(x);

    if (((v >> 8) & 0xFFFF) >= 0x0100)
        v = (v & 0xFF0000FFu) | (uint32_t)WINDOW_RIGHT_MAX << 8;
    t->own.ending_stages.wipe_right = v;
    g.window_edges[1] = (uint8_t)(v >> 8);
}

/*
 * @routine 82:B511 EndingStageFrame
 * @routine 82:B538 EndingStageFrameBands
 * WHAT IT DOES: Draws a frame of a stage played back at the ending: the
 * sprites built (with the backdrop's parallax bands stepped too, for the
 * last stage), the layers scrolled, the actors in view drawn, the backdrop
 * following, and the uploads asked for.
 */
static void stage_frame(bool bands)
{
    g.tile_blocks_count = 0;
    g.frame_ready = 0;
    OamBuildBegin();
    if (bands)
        ParallaxUpdate();
    LayersScroll();
    ActorsInView();
    OamDrawActors();
    BgFollowLayers();
    g.frame_ready++;
    g.tile_blocks_pending++;
    g.vram_queue_pending++;
}

/*
 * @routine 82:B501 EndingFrameEnd
 * WHAT IT DOES: Ends a frame of the ending's stages task: the frame's
 * update, then the wait for the next frame, again while the VRAM queue is
 * still waiting.
 */
static TaskRun frame_end(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        if (FrameStep())
            return task_exec_take(t);
        *at = 1;
        return task_wait(t, 1);
    }
    if (g.vram_queue_pending)
        return task_wait(t, 1);
    return task_return(t);
}

enum { SC_START, SC_BODY, SC_HOLD };

/*
 * @routine 82:B458 EndingStageScroll
 * WHAT IT DOES: The play-back's move to the next stage: for 41 frames the
 * stage is drawn while the view scrolls by its step and the window closes
 * from both sides; then the stage's caption (EndingStageTextTask) is
 * started and the task waits 63 frames as many times as the stage's hold
 * says, and the stage count goes on.
 */
static TaskRun stage_scroll(Task *t)
{
    uint16_t *at = task_enter(t);
    uint8_t x = (uint8_t)(t->own.ending_stages.step << 2);
    int16_t move;
    uint8_t holds;

    switch (*at) {
    case SC_START:
        t->param = SCROLL_FRAMES;
        *at = SC_BODY;
        return task_wait(t, 1);
    case SC_BODY:
        stage_frame(false);
        move = ending_scroll_step(x);
        g.layers[0].scroll_x += (uint32_t)(int32_t)move * 256u;
        g.layers[1].scroll_x += (uint32_t)(int32_t)move * 256u;
        wipe_left_step(t, x);
        wipe_right_step(t, x);
        if (--t->param != 0)
            return task_wait(t, 1);
        TaskCreate(TEXT_SLOT, 0, TASK_ENTRY_ENDING_STAGE_TEXT);
        g.ending_page = t->own.ending_stages.stage;
        t->param = ending_stage_hold(t->own.ending_stages.stage);
        *at = SC_HOLD;
        return task_wait(t, STAGE_HOLD_FRAMES);
    default:
        holds = --t->param;
        if (holds != 0)
            return task_wait(t, STAGE_HOLD_FRAMES);
        t->own.ending_stages.stage++;
        return task_return(t);
    }
}

enum {
    ST_START, ST_STAGE, ST_LOAD, ST_PLAY, ST_AFTER_PLAY, ST_SCROLL, ST_FADE_OUT, ST_BANDS, ST_BANDS_RUN, ST_END_WAIT,
    ST_LAST_WIPE, ST_PLACE, ST_FOREVER
};

/* the window use bytes $03AA, $03AB and $03AC (WOBJSEL) into g.window_use's nibbles: BG1 and BG2, BG3 and BG4, sprites and colour */
static void set_window_use(uint8_t bg12, uint8_t bg34, uint8_t obj_colour)
{
    g.window_use[0] = bg12 & 0x0F;
    g.window_use[1] = bg12 >> 4;
    g.window_use[2] = bg34 & 0x0F;
    g.window_use[3] = bg34 >> 4;
    g.window_use[4] = obj_colour & 0x0F;
    g.window_use[5] = obj_colour >> 4;
}

/* the windows and colour math over a stage of the play-back (not on level $1A: no colour math there) */
static void stage_windows(Task *t)
{
    t->own.ending_stages.wipe_left = 0x38u << 8;
    t->own.ending_stages.wipe_right = 0xC8u << 8;
    g.window_edges[0] = 0x38;
    g.window_edges[1] = 0xC8;
    set_window_use(0x33, 0x0C, 0x23);
    g.window_main = g.main_layers;
    g.window_sub = g.sub_layers;
    if ((uint8_t)g.level == 0x1A)
        return;
    g.blend_subtract = 1;
    g.blend_half = 0;
    g.blend_layers = 0x23;
    g.fixed_red = g.fixed_green = g.fixed_blue = 0x1F;
    g.blend_prevent = 2;
    g.blend_clip = 0;
    g.blend_use_sub = 0;
    g.blend_direct_colour = 0;
}

/*
 * @routine 82:B29C EndingStagesTask
 * WHAT IT DOES: Plays back the levels at the ending: stage $0C alone with
 * fewer than 50 items, else the stages from 4 to $0B, each run as a demo
 * for its countdown with a window over it, the last scrolled across to x
 * $400.
 * HOW: Fewer than 50 items: the single stage $0C; else the stage 4; the
 * song $26. Per stage: the demo's start, the new game state, the play
 * counts, the level loaded; with 50 or more the windows and colour math
 * set; the screen blanked and brightened by the fade-in task; with 50 or
 * more: StageCountdownSet, then a frame of the demo per count, 63 frames,
 * then (not the last stage) the move to the next stage (stage_scroll), its
 * fade-out and the next stage; the last stage scrolls on 3 pixels a frame
 * to x $400, the actors cleared, the window closed by the last two steps,
 * the colour math off, and the two figures that end it placed, then frames
 * for good. Fewer than 50: the caption's task (EndingTextTask) and a frame
 * of the single stage for good.
 */
TaskRun EndingStagesTask(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;
    Actor *a;

    for (;;) {
        switch (*at) {
        case ST_START:
            t->own.ending_stages.single = g.gem_sets_total < ITEMS_FOR_ALL;
            t->own.ending_stages.step = 0;
            g.demo_number = t->own.ending_stages.single ? SINGLE_STAGE : FIRST_STAGE;
            t->own.ending_stages.stage = 0;
            SongPlay(SONG);
            *at = ST_STAGE;
            /* fall through */
        case ST_STAGE:
            DemoStart();
            NewGameState();
            PlayStartCounts();
            *at = ST_LOAD;
            /* fall through */
        case ST_LOAD:
            r = LevelLoad(t);
            if (r != TASK_GOES_ON)
                return r;
            g.continue_frames++;
            if (!t->own.ending_stages.single)
                stage_windows(t);
            g.blank = 1;
            g.brightness = 0;
            g.player_flags = 0xFF;
            TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
            t->own.ending_stages.frames = 0;
            StageCountdownSet();
            if (t->own.ending_stages.single) {
                TaskCreate(TEXT_SLOT, 0, TASK_ENTRY_ENDING_TEXT);
                *at = ST_FOREVER;
                break;
            }
            *at = ST_PLAY;
            /* fall through */
        case ST_PLAY:
            r = frame_end(t);
            if (r != TASK_GOES_ON)
                return r;
            t->own.ending_stages.frames++;
            if (--g.play_countdown != 0)
                break;
            *at = ST_AFTER_PLAY;
            return task_wait(t, STAGE_HOLD_FRAMES);
        case ST_AFTER_PLAY:
            if (g.demo_number == LAST_STAGE) {
                *at = ST_BANDS;
                break;
            }
            *at = ST_SCROLL;
            /* fall through */
        case ST_SCROLL:
            r = stage_scroll(t);
            if (r != TASK_GOES_ON)
                return r;
            t->own.ending_stages.step ^= 1;
            TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
            *at = ST_FADE_OUT;
            return task_wait(t, 1);
        case ST_FADE_OUT:
            if (task_busy(FADE_SLOT))
                return task_wait(t, 1);
            g.demo_number++;
            *at = ST_STAGE;
            break;
        case ST_BANDS:
            *at = ST_BANDS_RUN;
            return task_wait(t, 1);
        case ST_BANDS_RUN:
            g.layers[0].scroll_x += END_SCROLL_SPEED * 256u;
            g.range_x = (uint16_t)(g.range_x + END_SCROLL_SPEED);
            stage_frame(true);
            if (((g.layers[0].scroll_x >> 8) & 0xFFFF) < END_SCROLL_X)
                return task_wait(t, 1);
            ActorRecordsInit();
            OamBufferClear();
            *at = ST_END_WAIT;
            return task_wait(t, 0x40);
        case ST_END_WAIT:
            g.scroll_kind = 0x0C;
            *at = ST_LAST_WIPE;
            return task_wait(t, 1);
        case ST_LAST_WIPE:
            wipe_left_step(t, WIPE_ENTRY_LEFT);
            wipe_right_step(t, WIPE_ENTRY_RIGHT);
            if (g.window_edges[0] != 0 || g.window_edges[1] != WINDOW_RIGHT_MAX)
                return task_wait(t, 1);
            g.blend_prevent = 3;
            g.blend_clip = 0;
            g.blend_use_sub = 0;
            g.blend_direct_colour = 0;
            g.blend_layers = 0;
            g.blend_half = 0;
            g.blend_subtract = 0;
            set_window_use(0, 0, 0x30);
            g.fixed_red = g.fixed_green = g.fixed_blue = 0;
            *at = ST_PLACE;
            return task_wait(t, 0x20);
        case ST_PLACE:
            a = ActorCreate(KIND_END_A);
            if (a)
                a->state = ACTOR_STATE_NEW;
            a = ActorCreate(KIND_END_B);
            if (a) {
                a->state = ACTOR_STATE_NEW;
                a->spawn_byte = KIND_END_B_BYTE;
            }
            *at = ST_FOREVER;
            /* fall through */
        case ST_FOREVER:
            r = frame_end(t);
            if (r != TASK_GOES_ON)
                return r;
            break;
        }
    }
}

/* ---- the text pages ---- */

/*
 * @routine 82:B1CE TextPageExtra
 * WHAT IT DOES: After a page of the ending's text, adds the item count to
 * the two pages that show it: page 12 of the timed text, with layouts $6C
 * and $6E, and page 6 of the untimed captions, with layouts $76 and $78 and
 * then $7C when the count is 70, else $7A.
 */
static void text_page_extra(const Task *t)
{
    if (t->own.ending_text.timed) {
        if (g.ending_page != EXTRA_TIMED_PAGE)
            return;
        TextNumberTwoDigits(g.gem_sets_total, 0x0522);
        TextLayoutShow(0x6C);
        TextLayoutShow(0x6E);
        return;
    }
    if (g.ending_page != EXTRA_UNTIMED_PAGE)
        return;
    TextNumberTwoDigits(g.gem_sets_total, 0x0530);
    TextLayoutShow(0x76);
    TextLayoutShow(0x78);
    TextLayoutShow(g.gem_sets_total == ITEMS_ALL ? 0x7C : 0x7A);
}

enum {
    TX_START, TX_SETUP, TX_PAGE_TOP, TX_COUNT, TX_CONCEAL, TX_DRAW, TX_REVEAL, TX_FOREVER, TX_LONG, TX_LAST_CONCEAL
};

/*
 * @routine 82:B0A9 EndingStageTextTask
 * @routine 82:B0AD EndingTextTask
 * WHAT IT DOES: The ending's text. EndingTextTask: 13 pages, each drawn,
 * its bands revealed, shown for 660 frames and concealed; after the last,
 * long waits and the text masked for good. EndingStageTextTask: the
 * ending play-back's caption for the stage just reached, its page of text
 * drawn, revealed and left up; the task then waits a frame at a time for
 * good, until the next stage's caption takes its slot.
 * HOW: The timed one starts at page 0 with its first wait; both: palettes
 * $61 and $5F, HDMA channel 5 on the bands' window bytes (TextBandsCommit),
 * window 2 open, the kept options' third setting cleared and sealed. Per
 * page: the layer cleared, the bands masked, the page and its places from
 * the tables in bank $A5 (the page ($EE) twice, plus 14 for the timed),
 * TextPageDraw, TextPageExtra, a frame, TextBandsReveal; the timed: the
 * count of 660 frames, TextBandsConceal and the next page; after page 13
 * four waits of 255 frames, TextBandsConceal, then a frame's wait forever.
 */
static TaskRun text_pages(Task *t, uint16_t *at)
{
    TaskRun r;
    unsigned x;

    for (;;) {
        switch (*at) {
        case TX_SETUP:
            PaletteLoad(TEXT_PALETTES_A);
            PaletteLoad(TEXT_PALETTES_B);
            t->own.ending_text.flip = 0;
            g.line_channel[TEXT_BANDS_CHANNEL].target = LINE_WINDOW_USE_BG34;
            g.line_channel[TEXT_BANDS_CHANNEL].direct = 0;
            for (x = 0; x < 8; x++)
                g.text_bands[x] = 0;
            TextBandsCommit(t);
            g.line_effects_on |= 1u << TEXT_BANDS_CHANNEL;
            g.window_edges[2] = 0;
            g.window_edges[3] = 0xFF;
            g.kept_options[2] = 0;
            KeptOptionsSeal();
            *at = t->own.ending_text.timed ? TX_PAGE_TOP : TX_DRAW;
            break;
        case TX_PAGE_TOP:
            if (--t->own.ending_text.timer != 0) {
                *at = TX_COUNT;
                return task_wait(t, 1);
            }
            *at = TX_CONCEAL;
            /* fall through */
        case TX_CONCEAL:
            r = TextBandsConceal(t);
            if (r != TASK_GOES_ON)
                return r;
            *at = TX_DRAW;
            break;
        case TX_COUNT:
            *at = TX_PAGE_TOP;
            break;
        case TX_DRAW: {
            DataRef places;

            TextLayerClear();
            for (x = 0; x < 8; x++) {
                g.text_bands[x] = 8;
                g.text_band_values[x] = 8;
            }
            TextBandsCommit(t);
            x = (unsigned)(g.ending_page * 2u + t->own.ending_text.timed);
            places = ending_text_places((uint8_t)x);
            TextPageDraw(ending_text_page((uint8_t)x), &places);
            text_page_extra(t);
            *at = TX_REVEAL;
            return task_wait(t, 1);
        }
        case TX_REVEAL:
            r = TextBandsReveal(t);
            if (r != TASK_GOES_ON)
                return r;
            if (!t->own.ending_text.timed) {
                *at = TX_FOREVER;
                break;
            }
            t->own.ending_text.timer = PAGE_WAIT;
            if (++g.ending_page != TIMED_LAST) {
                *at = TX_COUNT;
                return task_wait(t, 1);
            }
            t->own.ending_text.waits = 4;
            *at = TX_LONG;
            /* fall through */
        case TX_LONG:
            if (t->own.ending_text.waits != 0) {
                t->own.ending_text.waits--;
                return task_wait(t, LONG_WAIT);
            }
            *at = TX_LAST_CONCEAL;
            /* fall through */
        case TX_LAST_CONCEAL:
            r = TextBandsConceal(t);
            if (r != TASK_GOES_ON)
                return r;
            *at = TX_FOREVER;
            /* fall through */
        default:                        /* TX_FOREVER */
            return task_wait(t, 1);
        }
    }
}

TaskRun EndingTextTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == TX_START) {
        t->own.ending_text.timed = TIMED_PAGES;
        g.ending_page = 0;
        t->own.ending_text.timer = PAGE_FIRST_WAIT;
        *at = TX_SETUP;
    }
    return text_pages(t, at);
}

TaskRun EndingStageTextTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == TX_START) {
        t->own.ending_text.timed = 0;
        *at = TX_SETUP;
    }
    return text_pages(t, at);
}

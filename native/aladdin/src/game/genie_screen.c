/*
 * genie_screen.c: the Genie screen (level $2F), the bonus after a stage
 * where a figure of kind $27 was touched.
 *
 * The record's routines, as the code below has them: GenieFrame (80:AB90) is
 * FrameEndTimes(t, 1) (a wait, then the frame's update); GenieHealthFill
 * (80:AAED), GenieGemsAdd (80:AB15), GenieScreenEnd (80:AB35),
 * GeniePrizeSignal80 (80:AB9B) and GeniePrizeSignal2 (80:ABA5) are the
 * states of GenieScreen from PRIZE_SIGNAL on, kept one after another as the
 * waits they hold; GenieSlotStep, GenieSlotEnd, GenieZoomStep,
 * GenieCountdownStep, GenieZoomReset and GeniePointerCheck are functions.
 */
#include "audio/sound.h"
#include "data/game_tables.h"
#include "game/game.h"
#include "map/map.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define GENIE_LEVEL 0x2F
#define FADE_SLOT 1
#define PRESS_BUTTONS (PAD_A | PAD_B | PAD_X | PAD_Y | PAD_START)
#define SONG_LAMP 0x29
#define SOUND_ZOOM_END 0x3A
#define SOUND_SLOT 0x3B
#define SOUND_PRIZE_END 0xF0
#define SPEED_TOP 0x088A            /* the wheel's top speed */
#define SPEED_UP 0x30
#define SPEED_SLOW_BELOW 0x0130     /* below this the speed falls a step every fourth frame */
#define SPEED_NEAR 0x0110
#define SPEED_NEAR_STEP 0x2E
#define SPEED_AT_SLOT 0x0080        /* in a slot's reach: from here slowed by 6, from $70 stopped on its slot */
#define SPEED_AT_SLOT_LOW 0x0070
#define SPEED_AT_SLOT_STEP 6
#define PRIZES 7
#define LAMP_POINTS 4
#define SIGNAL_LIVES 2
#define SIGNAL_CONTINUE 0x80
#define SIGNAL_FRAMES 0xEA
#define COUNT_MASK ((1u << 24) - 1)  /* the count and the lamp's speed are 24 bits */
#define LAMP_COUNT_START 0x0180     /* the count's whole part when the lamp starts to zoom */
#define LIVES_TOP 0x64
#define LIVES_EVERY 0x10
#define HEALTH_EVERY 10
#define HEALTH_AFTER 0x40
#define HEALTH_TOP 0x0B             /* the most health boxes plus one */
#define CONTINUE_WAIT 0x30
#define END_PAUSE 0x58
#define END_SONG_WAIT 0xA0
#define STOP_WAIT 0x20

enum {                              /* the prizes, by the wheel's slot */
    PRIZE_LAMP, PRIZE_CONTINUE_1, PRIZE_ONE, PRIZE_HEALTH, PRIZE_CONTINUE_2, PRIZE_TWO, PRIZE_CONTINUE_3
};

enum {
    G_START, G_MODE7, G_LAMP_FIRST, G_LAMP_COUNT, G_PHASE_START, G_PHASE_A, G_PHASE_B, G_PHASE_C_INIT, G_PHASE_C,
    G_WHEEL_SET, G_WHEEL_GO, G_WAIT_BUTTON, G_SPIN, G_SLOW_BEGIN, G_SLOW_FRAME, G_STOP_FRAME, G_STOP_PAUSE,
    G_PRIZE_SIGNAL_ON, G_PRIZE_SIGNAL_OFF, G_PRIZE_GIVE, G_LIVES, G_HEALTH, G_HEALTH_AFTER, G_CONTINUE_WAIT,
    G_END_PAUSE, G_END_BLINK, G_END_SOUND, G_END_SOUND_WAIT, G_END_SONG_WAIT, G_END_FADE
};

/*
 * @routine 80:AC51 GenieZoomReset
 * WHAT IT DOES: The lamp's mode-7 scale back to 1 and its offsets 0.
 */
static void zoom_reset(void)
{
    g.m7_matrix[0] = 0x0100;
    g.m7_matrix[3] = 0x0100;
    g.m7_matrix[1] = 0;
    g.m7_matrix[2] = 0;
}

/* The lamp's count ($1A1B-$1A1D, 24 bits: layer_offset_y its upper 16) */
static uint32_t count_get(const Task *t)
{
    return (uint32_t)g.layer_offset_y << 8 | t->own.genie.count_fraction;
}

static void count_set(Task *t, uint32_t v)
{
    t->own.genie.count_fraction = (uint8_t)v;
    g.layer_offset_y = (uint16_t)(v >> 8);
}

static bool count_negative(void)
{
    return (g.layer_offset_y & 0x8000) != 0;
}

/*
 * @routine 80:AC1A GenieCountdownStep
 * WHAT IT DOES: The 24-bit count at $1A1B less the speed ($A5-$A7), and the
 * speed up by $A8.
 */
static void countdown_step(Task *t)
{
    uint32_t speed = (uint32_t)t->own.genie.a5 | (uint32_t)t->own.genie.a6 << 8 | (uint32_t)t->own.genie.a7 << 16;

    count_set(t, (count_get(t) - speed) & COUNT_MASK);
    speed = (speed + t->own.genie.a8) & COUNT_MASK;
    t->own.genie.a5 = (uint8_t)speed;
    t->own.genie.a6 = (uint8_t)(speed >> 8);
    t->own.genie.a7 = (uint8_t)(speed >> 16);
}

/*
 * @routine 80:ABF9 GenieZoomStep
 * WHAT IT DOES: A step of the lamp's zoom: the mode-7 scale words move by
 * the two amounts, and the zoom ($AA) by the step; returns that result's
 * sign (true when negative).
 */
static bool zoom_step(Task *t, uint16_t down, uint16_t up, uint8_t step)
{
    g.m7_matrix[3] = (int16_t)((uint16_t)g.m7_matrix[3] + down);
    g.m7_matrix[0] = (int16_t)((uint16_t)g.m7_matrix[0] - up);
    t->own.genie.zoom = (uint8_t)(t->own.genie.zoom - step);
    return (t->own.genie.zoom & 0x80) != 0;
}

/*
 * @routine 80:ABBE GenieSlotStep
 * WHAT IT DOES: Tells whether the wheel's angle ($A8) has passed the next
 * slot's bound: true, and the slot count ($A9) up (wrapping at 7).
 */
static bool slot_step(Task *t)
{
    unsigned x = t->own.genie.slot;
    bool negative = (t->own.genie.a8 & 0x80) != 0;

    if (x == 7) {
        t->own.genie.slot = 0;
        x = 0;
    }
    if (x == 0 && negative)
        return false;
    if (t->own.genie.a8 >= genie_slot_start(x)) {
        t->own.genie.slot++;
        return true;
    }
    return false;
}

/*
 * @routine 80:ABE4 GenieSlotEnd
 * WHAT IT DOES: Tells whether the wheel's angle ($A8) is at or past the end
 * of the slot the count ($A9) names.
 */
static bool slot_end(const Task *t)
{
    unsigned x = t->own.genie.slot;

    if (x == 1 && !(t->own.genie.a8 & 0x80))
        return true;
    return t->own.genie.a8 >= genie_slot_end(x);
}

/*
 * @routine 80:AC65 GeniePointerCheck
 * WHAT IT DOES: Tells the pointer whether the wheel's angle ($A8) is inside
 * one of the seven slots' windows: the pick counts up when it is; else, if
 * it was set, it becomes $FF.
 */
static void pointer_check(const Task *t)
{
    for (int x = PRIZES - 1; x >= 0; x--) {
        if (t->own.genie.a8 >= genie_slot_start((unsigned)x) && t->own.genie.a8 < genie_pointer_end((unsigned)x)) {
            g.genie_pick++;
            return;
        }
    }
    if (g.genie_pick)
        g.genie_pick = 0xFF;
}

/* a pad button that ends a wait */
static bool pressed(void)
{
    return (g.pad_pressed[0] & PRESS_BUTTONS) != 0;
}

static uint16_t speed16(const Task *t)
{
    return (uint16_t)(t->own.genie.a5 | t->own.genie.a6 << 8);
}

static void speed16_set(Task *t, uint16_t v)
{
    t->own.genie.a5 = (uint8_t)v;
    t->own.genie.a6 = (uint8_t)(v >> 8);
}

static uint16_t angle16(const Task *t)
{
    return (uint16_t)(t->own.genie.a7 | t->own.genie.a8 << 8);
}

static void angle16_set(Task *t, uint16_t v)
{
    t->own.genie.a7 = (uint8_t)v;
    t->own.genie.a8 = (uint8_t)(v >> 8);
}

/* the wheel turns on by its speed, and is drawn at the new angle */
static void wheel_turn(Task *t)
{
    angle16_set(t, (uint16_t)(angle16(t) + speed16(t)));
    Mode7Rotate(t->own.genie.a8);
}

/* the lamp phase's tables into the speed and its step */
static void phase_speed(Task *t)
{
    unsigned p = t->own.genie.slot;

    t->own.genie.a5 = genie_lamp(p, GENIE_LAMP_SPEED_0);
    t->own.genie.a6 = genie_lamp(p, GENIE_LAMP_SPEED_1);
    t->own.genie.a7 = genie_lamp(p, GENIE_LAMP_SPEED_2);
    t->own.genie.a8 = genie_lamp(p, GENIE_LAMP_ACCEL);
}

static uint8_t matrix_d_high(void)
{
    return (uint8_t)((uint16_t)g.m7_matrix[3] >> 8);
}

/* the prize the wheel stopped at, from its angle */
static unsigned prize_of(const Task *t)
{
    unsigned y = 0;

    for (;;) {
        if (t->own.genie.a8 < genie_prize_bound(y))
            return y;
        if (++y == PRIZES)
            return 0;
    }
}

/*
 * @routine 80:A63B GenieScreen
 * WHAT IT DOES: The Genie screen (level $2F), the bonus after a stage where
 * a figure of kind $27 was touched: the screen is built, the Genie's lamp
 * zooms in, the wheel spins until a button stops it, slows and stops on a
 * slot, and the slot's prize is given (health, a continue, or lives); then
 * the next part of the game goes on.
 * HOW: Every other task ended; the screen off and cleared; level $2F's RAM,
 * map and layers; graphics sets $17 and $2E; palettes $5C and $39; Mode 7
 * from what was copied, the circle wipe's table cleared round it, the fade
 * in and song $29. The lamp: its scale and offsets reset, a count run down
 * by a speed that grows (GenieCountdownStep), the scale zoomed in three
 * phases by tables; then the window tasks started and the wheel. The wheel
 * turns up to its top speed while a button is waited for (the first press
 * starts it; a press at the top speed starts it slowing); it slows a step
 * every fourth frame, by more as a slot comes round, and stops on a slot;
 * then 32 frames; the prize by the angle's bounds, its figure's signal
 * (genie_screen_step 2 or $80, then 234 frames), and the prize given; the
 * end: a pause, the cue, the song, the fade-out, and the task starts again
 * at NextPartTask.
 */
TaskRun GenieScreen(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    for (;;) {
        switch (*at) {
        case G_START:
            TaskSlotsFree1To6();
            g.demo_number = 0;
            g.play_mode = 0;
            g.screen_cue = 0;
            g.genie_screen_step = 0;
            g.genie_pose = 0;
            g.genie_pick = 0;
            NmiDisable();
            ScreenBlank();
            ScrollStateClear();
            OamBufferClear();
            DisplayDefaults();
            TextLayerClear();
            g.level = GENIE_LEVEL;
            LevelRamSetup();
            g.layers[0].scroll_x &= 0xFF;
            g.layers[0].scroll_y &= 0xFF;
            g.genie_screen_y = 0xFFB0;
            LevelMapBuild();
            LayersFill();
            UnpackSet(0x17);
            VramCopyKept(0, 0x3F / 7);
            VramCopyKept(0, 0x46 / 7);
            VramCopyKept(0, 0x4D / 7);
            VramCopyKept(0, 0x54 / 7);
            UnpackSet(0x2E);
            VramCopyKept(0, 0x23 / 7);
            PaletteLoad(0x5C);
            PaletteLoad(0x39);
            NmiEnable();
            g.main_layers = 0;
            g.sub_layers = 0;
            g.bg_tiles[0] = VIDEO_TILES_HIGH;
            g.bg_tiles[1] = VIDEO_TILES_LOW;
            *at = G_MODE7;
            return task_wait(t, 1);
        case G_MODE7:
            Mode7ScreenSetupKept(0x10);
            CircleWipeStep(0x50, 0x80, 0x80);
            HdmaTableFill(0x0100, 0xFF00, 0x28);
            TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_UP);
            g.blank = 0;
            g.brightness = 0;
            g.m7_centre_x = 0x80;
            g.m7_centre_y = 0xD0;
            SongPlay(SONG_LAMP);
            g.main_layers = 1;
            g.sub_layers = 1;
            *at = G_LAMP_FIRST;
            /* fall through */
        case G_LAMP_FIRST:
            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            zoom_reset();
            count_set(t, (uint32_t)LAMP_COUNT_START << 8);
            t->own.genie.a5 = 0;
            t->own.genie.a6 = 1;
            t->own.genie.a7 = 0;
            t->own.genie.a8 = 0x40;
            t->own.genie.slot = 0;
            *at = G_LAMP_COUNT;
            /* fall through */
        case G_LAMP_COUNT:
            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            countdown_step(t);
            if (!count_negative()) {
                *at = G_LAMP_COUNT;
                break;
            }
            count_set(t, 0);
            SoundQueueAdd(SOUND_ZOOM_END);
            *at = G_PHASE_START;
            /* fall through */
        case G_PHASE_START:
            t->own.genie.zoom = genie_lamp(t->own.genie.slot, GENIE_LAMP_START);
            t->own.genie.reset_wait = genie_lamp(t->own.genie.slot, GENIE_LAMP_WAIT);
            *at = G_PHASE_A;
            /* fall through */
        case G_PHASE_A: {
            uint8_t zoom, quarter;
            bool negative;

            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            if (t->own.genie.reset_wait) {
                if (t->own.genie.reset_wait == 1)
                    zoom_reset();
                t->own.genie.reset_wait--;
            }
            zoom = t->own.genie.zoom;
            quarter = (uint8_t)(zoom >> 2);
            if (matrix_d_high() == 0) {
                uint8_t keep = zoom;

                zoom = quarter;
                quarter = keep;
            }
            negative = zoom_step(t, zoom, quarter, genie_lamp(t->own.genie.slot, GENIE_LAMP_ZOOM_STEP));
            if (!negative) {
                *at = G_PHASE_A;
                break;
            }
            t->own.genie.zoom = 0;
            phase_speed(t);
            *at = G_PHASE_B;
            break;
        }
        case G_PHASE_B: {
            uint8_t zoom, shifted;
            uint16_t add, take;

            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            zoom = t->own.genie.zoom;
            shifted = (uint8_t)(zoom << 2);
            add = zoom;
            take = shifted;
            if (matrix_d_high() == 0) {
                add = shifted;
                take = zoom;
                countdown_step(t);
                if (count_negative()) {
                    SoundQueueAdd(SOUND_ZOOM_END);
                    count_set(t, 0);
                    t->own.genie.slot++;
                    *at = t->own.genie.slot == 2 ? G_PHASE_C_INIT : G_PHASE_START;
                    break;
                }
            }
            if (t->own.genie.zoom >= genie_lamp(t->own.genie.slot, GENIE_LAMP_LIMIT)) {
                *at = G_PHASE_B;
                break;
            }
            g.m7_matrix[0] = (int16_t)((uint16_t)g.m7_matrix[0] + add);
            g.m7_matrix[3] = (int16_t)((uint16_t)g.m7_matrix[3] - take);
            t->own.genie.zoom = (uint8_t)(t->own.genie.zoom + genie_lamp(t->own.genie.slot, GENIE_LAMP_RISE));
            *at = G_PHASE_B;
            break;
        }
        case G_PHASE_C_INIT:
            phase_speed(t);
            *at = G_PHASE_C;
            /* fall through */
        case G_PHASE_C: {
            uint8_t low;

            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            low = (uint8_t)g.m7_matrix[0];
            if (low) {
                low--;
                g.m7_matrix[0] = (int16_t)(((uint16_t)g.m7_matrix[0] & 0xFF00) | low);
            }
            low = (uint8_t)g.m7_matrix[3];
            if (low) {
                low = (uint8_t)(low + 2);
                if (low & 0x80) {
                    g.m7_matrix[3] = (int16_t)(((uint16_t)g.m7_matrix[3] & 0xFF00) | low);
                } else {
                    g.m7_matrix[3] = (int16_t)(((uint16_t)g.m7_matrix[3] & 0xFF00) + 0x0100);
                }
            }
            countdown_step(t);
            if (!count_negative()) {
                *at = G_PHASE_C;
                break;
            }
            SoundQueueAdd(SOUND_ZOOM_END);
            count_set(t, 0);
            t->own.genie.slot++;
            *at = t->own.genie.slot == LAMP_POINTS ? G_WHEEL_SET : G_PHASE_C_INIT;
            break;
        }
        case G_WHEEL_SET:
            g.main_layers = 0x11;
            g.sub_layers = 0x11;
            g.m7_centre_y = (uint16_t)((g.m7_centre_y & 0xFF00) | 0x80);
            TaskCreate(4, 0x0E, TASK_ENTRY_WINDOW_EFFECT);
            TaskCreate(3, 4, TASK_ENTRY_WINDOW_EFFECT);
            g.window_main = 1;
            g.window_sub = 1;
            g.window_edges[0] = 0;
            g.window_edges[1] = 0xFF;
            g.window_use[0] = 3;        /* $03AA = $33: BG1 and BG2 by window 1; $03AB and $03AC clear */
            g.window_use[1] = 3;
            g.window_use[2] = 0;
            g.window_use[3] = 0;
            g.window_use[4] = 0;
            g.window_use[5] = 0;
            *at = G_WHEEL_GO;
            return task_wait(t, 1);
        case G_WHEEL_GO:
            t->param = 0;
            t->own.genie.a5 = 0;
            t->own.genie.a6 = 0;
            t->own.genie.a7 = 0;
            t->own.genie.a8 = 0;
            t->own.genie.slot = 0;
            t->own.genie.zoom = 0;
            g.screen_cue++;
            *at = G_WAIT_BUTTON;
            /* fall through */
        case G_WAIT_BUTTON:
            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            if (!pressed()) {
                *at = G_WAIT_BUTTON;
                break;
            }
            g.screen_cue = 0;
            *at = G_SPIN;
            return task_wait(t, 1);
        case G_SPIN: {
            uint16_t s = speed16(t);

            if (s >= SPEED_TOP) {
                g.screen_cue = 1;
                if (pressed()) {
                    g.genie_pick = 0xFF;
                    *at = G_SLOW_BEGIN;
                    break;
                }
            }
            if (FrameStep())
                return task_exec_take(t);
            if (s >= SPEED_TOP) {
                /* the carry the compare left is added into the angle */
                speed16_set(t, SPEED_TOP);
                angle16_set(t, (uint16_t)(angle16(t) + SPEED_TOP + 1));
            } else {
                speed16_set(t, (uint16_t)(s + SPEED_UP));
                angle16_set(t, (uint16_t)(angle16(t) + speed16(t)));
            }
            Mode7Rotate(t->own.genie.a8);
            slot_step(t);
            *at = G_SPIN;
            return task_wait(t, 1);
        }
        case G_SLOW_BEGIN:
            g.screen_cue = 0;
            *at = G_SLOW_FRAME;
            /* fall through */
        case G_SLOW_FRAME: {
            uint16_t s;
            bool stop = false;
            bool sound = false;
            bool near = false;

            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            if (!(g.sprite_flicker & 3) && speed16(t) < SPEED_SLOW_BELOW) {
                speed16_set(t, (uint16_t)(speed16(t) - 1));
                if (speed16(t) & 0x8000) {
                    *at = G_STOP_PAUSE;
                    break;
                }
            }
            g.genie_pick_one = 0;
            pointer_check(t);
            if (slot_step(t)) {
                s = speed16(t);
                if (s >= SPEED_NEAR) {
                    s = (uint16_t)(s - SPEED_NEAR_STEP);
                    speed16_set(t, s);
                    stop = (s & 0x8000) != 0;
                    sound = !stop;
                } else if (s < SPEED_AT_SLOT_LOW) {
                    near = true;
                } else if (s < SPEED_AT_SLOT) {
                    if (t->own.genie.zoom != 0 || (RandomNext() & 3) == 0)
                        near = true;
                    else
                        sound = true;
                } else {
                    s = (uint16_t)(s - SPEED_AT_SLOT_STEP);
                    speed16_set(t, s);
                    stop = (s & 0x8000) != 0;
                    sound = !stop;
                }
            }
            if (stop) {
                *at = G_STOP_PAUSE;
                break;
            }
            if (near) {
                g.genie_pick_one = 1;
                g.genie_pose = 1;
                *at = G_STOP_FRAME;
                break;
            }
            if (sound)
                SoundQueueAdd(SOUND_SLOT);
            wheel_turn(t);
            *at = G_SLOW_BEGIN;
            break;
        }
        case G_STOP_FRAME:
            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            speed16_set(t, (uint16_t)(speed16(t) - 1));
            if (speed16(t) & 0x8000) {
                *at = G_STOP_PAUSE;
                break;
            }
            angle16_set(t, (uint16_t)(angle16(t) - speed16(t)));
            pointer_check(t);
            if (slot_end(t)) {
                Mode7Rotate(t->own.genie.a8);
                *at = G_STOP_FRAME;
                break;
            }
            t->own.genie.zoom = 1;
            t->own.genie.slot--;
            if (t->own.genie.slot & 0x80)
                t->own.genie.slot = 0;
            wheel_turn(t);
            *at = G_SLOW_BEGIN;
            break;
        case G_STOP_PAUSE:
            r = FrameEndTimes(t, STOP_WAIT);
            if (r != TASK_GOES_ON)
                return r;
            g.genie_pick = 0xFE;
            t->own.genie.prize = (uint8_t)prize_of(t);
            switch (t->own.genie.prize) {
            case PRIZE_LAMP:
                t->own.genie.song = 0x2C;
                t->own.genie.signal = SIGNAL_LIVES;
                break;
            case PRIZE_ONE:
            case PRIZE_TWO:
                t->own.genie.song = 0x33;
                t->own.genie.signal = SIGNAL_LIVES;
                break;
            case PRIZE_HEALTH:
                t->own.genie.song = 0x20;
                t->own.genie.signal = SIGNAL_LIVES;
                break;
            default:
                t->own.genie.song = 0x2C;
                t->own.genie.signal = SIGNAL_CONTINUE;
                break;
            }
            *at = G_PRIZE_SIGNAL_ON;
            /* fall through */
        case G_PRIZE_SIGNAL_ON:
            g.genie_screen_step = t->own.genie.signal;
            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            g.genie_screen_step = 0;
            *at = G_PRIZE_SIGNAL_OFF;
            /* fall through */
        case G_PRIZE_SIGNAL_OFF:
            r = FrameEndTimes(t, SIGNAL_FRAMES);
            if (r != TASK_GOES_ON)
                return r;
            *at = G_PRIZE_GIVE;
            /* fall through */
        case G_PRIZE_GIVE:
            switch (t->own.genie.prize) {
            case PRIZE_LAMP:
                if (g.health_marks + 1 >= HEALTH_TOP) {
                    t->own.genie.gems = 3;
                    *at = G_LIVES;
                } else {
                    g.health_marks++;
                    *at = G_HEALTH;
                }
                break;
            case PRIZE_ONE:
                t->own.genie.gems = 1;
                *at = G_LIVES;
                break;
            case PRIZE_TWO:
                t->own.genie.gems = 2;
                *at = G_LIVES;
                break;
            case PRIZE_HEALTH:
                if (g.health_marks == g.health) {
                    t->own.genie.gems = 1;
                    *at = G_LIVES;
                } else {
                    *at = G_HEALTH;
                }
                break;
            default:
                g.continues++;
                *at = G_CONTINUE_WAIT;
                break;
            }
            break;
        case G_LIVES:
            r = FrameEndTimes(t, LIVES_EVERY);
            if (r != TASK_GOES_ON)
                return r;
            if (g.lives != LIVES_TOP) {
                g.lives++;
                if (--t->own.genie.gems != 0) {
                    *at = G_LIVES;
                    break;
                }
            }
            *at = G_END_PAUSE;
            break;
        case G_HEALTH:
            r = FrameEndTimes(t, HEALTH_EVERY);
            if (r != TASK_GOES_ON)
                return r;
            if (g.health < g.health_marks) {
                g.health++;
                *at = G_HEALTH;
                break;
            }
            *at = G_HEALTH_AFTER;
            /* fall through */
        case G_HEALTH_AFTER:
            r = FrameEndTimes(t, HEALTH_AFTER);
            if (r != TASK_GOES_ON)
                return r;
            *at = t->own.genie.prize == PRIZE_HEALTH ? G_END_SOUND : G_END_PAUSE;
            break;
        case G_CONTINUE_WAIT:
            r = FrameEndTimes(t, CONTINUE_WAIT);
            if (r != TASK_GOES_ON)
                return r;
            *at = G_END_PAUSE;
            /* fall through */
        case G_END_PAUSE:
            r = FrameEndTimes(t, END_PAUSE);
            if (r != TASK_GOES_ON)
                return r;
            g.genie_screen_step = 1;
            *at = G_END_BLINK;
            /* fall through */
        case G_END_BLINK:
            r = FrameEndTimes(t, 1);
            if (r != TASK_GOES_ON)
                return r;
            g.genie_screen_step = 0;
            *at = G_END_SOUND;
            /* fall through */
        case G_END_SOUND:
            SoundQueueAdd(SOUND_PRIZE_END);
            *at = G_END_SOUND_WAIT;
            /* fall through */
        case G_END_SOUND_WAIT:
            r = FrameEndTimes(t, 2);
            if (r != TASK_GOES_ON)
                return r;
            SongPlay(t->own.genie.song);
            *at = G_END_SONG_WAIT;
            /* fall through */
        case G_END_SONG_WAIT:
            r = FrameEndTimes(t, END_SONG_WAIT);
            if (r != TASK_GOES_ON)
                return r;
            TaskCreate(FADE_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
            SoundQueueF6With1();
            *at = G_END_FADE;
            return task_wait(t, 1);
        case G_END_FADE:
            if (task_busy(FADE_SLOT))
                return task_wait(t, 1);
            TaskSlotsFree1To6();
            return task_exec(t, TASK_ENTRY_NEXT_PART);
        }
    }
}

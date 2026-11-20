/*
 * level_events.c: levels' event tasks that wait for the view or the
 * player to get somewhere, then change the backdrop, the colours or the
 * map.
 *
 * SoundWhileSlot1Task, RiserCoursesTask and SoundLaterTask are made by the
 * chain pieces' fight-won scene (kind $73's code, slots 2, 3, 4).
 */
#include "data/level_tables.h"
#include "audio/sound.h"
#include "level/level.h"
#include "system/state.h"
#include "system/task.h"

#define BAND_SPEEDS 0x500       /* $7E:F900 in g.line_data: the bands' speeds, 4 bytes apart (the low word) */
#define BANDS_FAST_FROM 0x0680  /* the view's x from which the bands speed up */
#define GREY_FROM 0x0E00        /* the player's x from which the screen greys */
#define RED_OUT_FROM 0x1A00     /* the view's x from which level 7's red goes */
#define RED_OUT_STEPS 7
#define RED_OUT_FRAMES 0x0F
#define RED_PALETTE 0x20        /* palette 2's first colour */
#define SPRITES_IN_FRONT 0x10   /* Actor.flags bits 4-5: the sprite priority */
#define CLIMB_LEVEL_TWIN 0x19   /* level $0A's twin, whose colours stay */

static uint16_t view_x(void) { return (uint16_t)(g.layers[0].scroll_x >> 8); }

static void band_speed_set(unsigned band, uint16_t v)
{
    g.line_data[BAND_SPEEDS + 4 * band] = (uint8_t)v;
    g.line_data[BAND_SPEEDS + 4 * band + 1] = (uint8_t)(v >> 8);
}

static uint16_t band_speed(unsigned band)
{
    return (uint16_t)(g.line_data[BAND_SPEEDS + 4 * band] | g.line_data[BAND_SPEEDS + 4 * band + 1] << 8);
}

/* the sprites' window use nibble 0 and the colour's 8 (the original's
   window-for-sprites shadow $80): colour maths everywhere */
static void colour_window_everywhere(void)
{
    g.window_use[4] = 0x0;
    g.window_use[5] = 0x8;
}

enum { BANDS_START, BANDS_WAIT_VIEW, BANDS_FAST };

/*
 * @routine 81:A0C3 BandsSpeedUpTask
 * WHAT IT DOES: Level 6's task that speeds the backdrop bands up once the
 * view gets far into the level, and keeps them at it.
 * HOW: The 11 bands' speeds from their starting table; each frame until
 * the view's x reaches $0680, nothing; then every frame each speed grows
 * by its step up to its most.
 */
TaskRun BandsSpeedUpTask(Task *t)
{
    uint16_t *at = task_enter(t);
    unsigned band;

    switch (*at) {
    case BANDS_START:
        for (band = 0; band < BANDS; band++)
            band_speed_set(band, band_start_speed(band));
        *at = BANDS_WAIT_VIEW;
        return task_wait(t, 1);
    case BANDS_WAIT_VIEW:
        if (view_x() < BANDS_FAST_FROM)
            return task_wait(t, 1);
        *at = BANDS_FAST;
        /* fall through */
    case BANDS_FAST:
        for (band = 0; band < BANDS; band++) {
            uint16_t v = (uint16_t)(band_speed(band) + band_speed_step(band)), most = band_most_speed(band);

            band_speed_set(band, v >= most ? most : v);
        }
        return task_wait(t, 1);
    }
    return task_exit(t);
}

/*
 * @routine 81:A128 GreyPastTask
 * WHAT IT DOES: Level $0E's task that greys the screen down a little once
 * the player gets past x $0E00, then ends.
 * HOW: Each frame until then nothing; then colour maths subtract a grey of
 * 4 from layer 2 everywhere (the fixed colour 4, 4, 4), the palette sent.
 */
TaskRun GreyPastTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        *at = 1;
        return task_wait(t, 1);
    }
    if ((uint16_t)(g.actors[0].x >> 8) < GREY_FROM)
        return task_wait(t, 1);
    g.blend_layers = 0x02;
    g.blend_half = 0;
    g.blend_subtract = 1;
    g.blend_direct_colour = 0;
    g.blend_use_sub = 0;
    g.blend_prevent = 0;
    g.blend_clip = 0;
    colour_window_everywhere();
    g.fixed_red = 4;
    g.fixed_green = 4;
    g.fixed_blue = 4;
    g.palette_upload++;
    return task_exit(t);
}

/*
 * @routine 81:A197 PaletteRedDown
 * WHAT IT DOES: Takes one step of red from each colour of palette 2 (not
 * below 0) and asks for the upload.
 */
static void PaletteRedDown(void)
{
    for (int i = 15; i >= 0; i--) {
        uint16_t c = g.palette[RED_PALETTE + i];

        if (c & 0x1F)
            c--;
        g.palette[RED_PALETTE + i] = c;
    }
    g.palette_upload++;
}

enum { RED_START, RED_WAIT_VIEW };      /* then RED_WAIT_VIEW + n: n steps done */

/*
 * @routine 81:A15B RedOutTask
 * WHAT IT DOES: Level 7's task that, once the view passes x $1A00, takes
 * the red out of palette 2 and adds blue over 8 steps a quarter second
 * apart, then ends.
 * HOW: Colour maths add the fixed colour to layer 1 everywhere; 7 times:
 * 15 frames, PaletteRedDown and the fixed colour's blue up by one; then
 * PaletteRedDown again. The resume point counts the steps. (The original
 * also clears $0080, a byte of slot 0's own data the play loop's task
 * does not use.)
 */
TaskRun RedOutTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == RED_START) {
        *at = RED_WAIT_VIEW;
        return task_wait(t, 1);
    }
    if (*at == RED_WAIT_VIEW) {
        if (view_x() < RED_OUT_FROM)
            return task_wait(t, 1);
        g.blend_layers = 0x01;
        g.blend_half = 0;
        g.blend_subtract = 0;
        g.blend_direct_colour = 0;
        g.blend_use_sub = 0;
        g.blend_prevent = 0;
        g.blend_clip = 0;
        colour_window_everywhere();
        *at = RED_WAIT_VIEW + 1;
        return task_wait(t, RED_OUT_FRAMES);
    }
    PaletteRedDown();
    g.fixed_blue++;
    if (*at < RED_WAIT_VIEW + RED_OUT_STEPS) {
        ++*at;
        return task_wait(t, RED_OUT_FRAMES);
    }
    PaletteRedDown();
    return task_exit(t);
}

enum { CLIMB_START, CLIMB_WATCH, CLIMB_OPEN };

/*
 * @routine 81:A30D ClimbTopTask
 * WHAT IT DOES: Level $0A's (and $19's) task for the climb: puts the
 * player's sprites in front while he is in the map's top 256 pixels, and
 * once he stands at the top (or the restart point is set) lets the view go
 * no lower, changes the colour maths (not on level $19) and, once the view
 * is past x $100, opens the way right ($FE into screen map place $21's
 * high byte).
 * HOW: Each frame: the restart point set ends the watch; his sprite
 * priority $10 with his y under 256, else 0; at the top: the view's y 0,
 * his x's low byte $80 on and his y's low byte under $78.
 */
TaskRun ClimbTopTask(Task *t)
{
    uint16_t *at = task_enter(t);
    Actor *player = &g.actors[0];
    uint16_t px, py;

    switch (*at) {
    case CLIMB_START:
        *at = CLIMB_WATCH;
        return task_wait(t, 1);
    case CLIMB_WATCH:
        if (!g.level_restart) {
            px = (uint16_t)(player->x >> 8);
            py = (uint16_t)(player->y >> 8);
            player->flags = (uint8_t)((player->flags & 0xCF) | ((py >> 8) ? 0 : SPRITES_IN_FRONT));
            if ((uint16_t)(g.layers[0].scroll_y >> 8) != 0 || (px & 0xFF) < 0x80 || (py & 0xFF) >= 0x78)
                return task_wait(t, 1);
        }
        g.scroll_limit_y &= 0x00FF;
        if (g.level != CLIMB_LEVEL_TWIN) {
            g.main_layers = 0x17;
            g.sub_layers = 0x10;
            g.blend_direct_colour = 1;
            g.blend_use_sub = 1;
            g.blend_prevent = 0;
            g.blend_clip = 0;
            g.blend_layers = 0x13;
            g.blend_half = 1;
            g.blend_subtract = 0;
        }
        *at = CLIMB_OPEN;
        return task_wait(t, 1);
    case CLIMB_OPEN:
        if ((view_x() >> 8) != 0x01)
            return task_wait(t, 1);
        g.screen_map.places[0x21] = (uint16_t)((g.screen_map.places[0x21] & 0x00FF) | 0xFE00);
        break;
    }
    return task_exit(t);
}

#define FADE_SLOT 1                 /* the fades run here */
#define SOUND_TIMER 0x4A
#define SOUND_EVERY 0x58
#define RISER_FIRST_WAIT 0x7F
#define RISER_SECOND_WAIT 0x7E
#define LATER_WAIT 0xC0

/*
 * @routine 81:A1C0 SoundWhileSlot1Task
 * WHAT IT DOES: Plays sound $4A every 88 frames for as long as the task in
 * slot 1 (a fade's) runs, then ends.
 */
TaskRun SoundWhileSlot1Task(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at != 0) {
        if (!task_busy(FADE_SLOT))
            return task_exit(t);
    }
    *at = 1;
    SoundQueueAdd(SOUND_TIMER);
    return task_wait(t, SOUND_EVERY);
}

enum { RISER_START, RISER_SECOND, RISER_THIRD, RISER_WATCH };

/*
 * @routine 81:A2D1 RiserCoursesTask
 * WHAT IT DOES: Sends kind $88's riser on course 2, after 127 frames on
 * course 1, after 126 more on course 0, and stops sending it once the task
 * in slot 1 (a fade's) has ended.
 */
TaskRun RiserCoursesTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case RISER_START:
        g.riser_send = 1;
        g.riser_course = 2;
        *at = RISER_SECOND;
        return task_wait(t, RISER_FIRST_WAIT);
    case RISER_SECOND:
        g.riser_course--;
        *at = RISER_THIRD;
        return task_wait(t, RISER_SECOND_WAIT);
    case RISER_THIRD:
        g.riser_course--;
        *at = RISER_WATCH;
        return task_wait(t, 1);
    case RISER_WATCH:
        if (task_busy(FADE_SLOT))
            return task_wait(t, 1);
        g.riser_send = 0;
        break;
    }
    return task_exit(t);
}

/*
 * @routine 81:A2FF SoundLaterTask
 * WHAT IT DOES: Waits 192 frames, queues sound command $F6 with speed 1 and
 * ends.
 */
TaskRun SoundLaterTask(Task *t)
{
    uint16_t *at = task_enter(t);

    if (*at == 0) {
        *at = 1;
        return task_wait(t, LATER_WAIT);
    }
    SoundQueueF6With1();
    return task_exit(t);
}

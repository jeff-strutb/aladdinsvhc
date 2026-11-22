/*
 * scroll_update.c: the play field's scroll each frame, the way the level
 * asks (g.scroll_kind): the camera that follows the player inside a window,
 * scrolls that run on their own, or none; then kept inside the level, empty
 * blocks stopping it (camera.c), and the backdrop's scroll (parallax.c).
 */
#include <stdio.h>

#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

enum { WINDOW_X_BIAS = 0, WINDOW_X_WIDTH = 2, WINDOW_Y_BIAS = 4, WINDOW_Y_HEIGHT = 6, WINDOW_POINT_Y = 8 };

/* the play field's scroll in whole pixels, and set keeping its fraction */
static uint16_t pixels(uint32_t position) { return (uint16_t)(position >> 8); }
static uint32_t with_pixels(uint32_t position, uint16_t v) { return (position & 0xFFu) | (uint32_t)v << 8; }

#define POSITION_BITS 24

/* a 24-bit add from the fraction byte, as the original's two words */
static uint32_t add24(uint32_t position, uint32_t by)
{
    return (position + by) & ((1u << POSITION_BITS) - 1);
}

/* How far the play field's scroll may go right and down. g.scroll_limit_x/y
   are the original's: the left and top edge of the last 256 x 224 view
   (its width less 256, its rows less one times 256). The view's edges are
   w and h from them, so the limit is those less 2 dx and 2 dy (the view
   wider than the level has none: 0). At 256 x 224 they are g.'s. */
static uint16_t limit_across(void)
{
    return g.scroll_limit_x > 2 * view.dx ? (uint16_t)(g.scroll_limit_x - 2 * view.dx) : 0;
}

static uint16_t limit_down(void)
{
    return g.scroll_limit_y > 2 * view.dy ? (uint16_t)(g.scroll_limit_y - 2 * view.dy) : 0;
}

/* the camera's point: x and y (the original's scratch) */
static uint16_t target_x, target_y;

/* From 81:C6E9: up and down, the player's y against the window: past it
   the scroll y moves by how far, at most 6 down and 6 up. */
static void follow_down(uint8_t w)
{
    ScrollLayer *l = &g.layers[0];
    uint16_t d, height = camera_window_word(w, WINDOW_Y_HEIGHT), a;

    if (g.scroll_y_held)
        return;
    d = (uint16_t)(pixels(g.actors[0].y) - target_y + camera_window_word(w, WINDOW_Y_BIAS));
    if (d < height)
        return;
    if (!(d & 0x8000)) {
        a = (uint16_t)(d - height);
        if (a >= 7)
            a = 6;
    } else {
        a = d < 0xFFFA ? 0xFFFA : d;
    }
    l->scroll_y = with_pixels(l->scroll_y, (uint16_t)(pixels(l->scroll_y) + a));
}

/* From 81:C6AE: across, the same with the player's x: at most 3 a frame;
   g.camera_push 1 for a move right, 2 for one left. */
static void follow_across(uint8_t w)
{
    ScrollLayer *l = &g.layers[0];
    uint16_t d, width = camera_window_word(w, WINDOW_X_WIDTH), a;

    if (g.scroll_x_held)
        return;
    d = (uint16_t)(pixels(g.actors[0].x) - target_x + camera_window_word(w, WINDOW_X_BIAS));
    if (d < width)
        return;
    g.camera_push++;
    if (!(d & 0x8000)) {
        a = (uint16_t)(d - width);
        if (a >= 4)
            a = 3;
    } else {
        g.camera_push++;
        a = d < 0xFFFD ? 0xFFFD : d;
    }
    l->scroll_x = with_pixels(l->scroll_x, (uint16_t)(pixels(l->scroll_x) + a));
}

/* The common end, from 81:C71E: the scroll kept to 0 and its limits, the
   stops at empty blocks unless held, then the backdrop. */
static void tail(void)
{
    ScrollLayer *l = &g.layers[0];
    uint16_t x = pixels(l->scroll_x), y = pixels(l->scroll_y);

    if (x & 0x8000)
        x = 0;
    else if (x >= limit_across())
        x = limit_across();
    else if (x < g.scroll_x_min)
        x = g.scroll_x_min;
    else if (x >= g.scroll_x_max)
        x = g.scroll_x_max;
    l->scroll_x = with_pixels(l->scroll_x, x);
    if (y & 0x8000)
        y = 0;
    else if (y >= limit_down())
        y = limit_down();
    l->scroll_y = with_pixels(l->scroll_y, y);
    if (!g.scroll_x_held)
        CameraStopAcross();
    if (!g.scroll_y_held)
        CameraStopUpDown();
    ParallaxUpdate();
}

/* Handler 0, from 81:C67C: the camera that follows the player; a negative
   g.camera_window keeps it where it is (and skips the end). */
static void follow(void)
{
    ScrollLayer *l = &g.layers[0];
    uint8_t w;

    g.scroll_x_before = pixels(l->scroll_x);
    g.scroll_y_before = pixels(l->scroll_y);
    if (g.camera_window < 0)
        return;
    w = (uint8_t)(g.camera_window + g.camera_window_step);
    g.camera_push = 0;
    target_x = (uint16_t)(pixels(l->scroll_x) + g.camera_aim_x + view.dx);
    target_y = (uint16_t)(pixels(l->scroll_y) + camera_window_word(w, WINDOW_POINT_Y) + view.dy);
    follow_across(w);
    follow_down(w);
    tail();
}

/* Handlers 10 and 14, from 81:C60F: the scroll x runs on at set speed `s`
   (0 or 4); up and down the camera follows as handler 0's with window 0. */
static void run_at(uint8_t s)
{
    ScrollLayer *l = &g.layers[0];

    l->scroll_x = add24(l->scroll_x, scroll_speed(s));
    g.camera_push = 0;
    target_y = (uint16_t)(pixels(l->scroll_y) + camera_window_word(0, WINDOW_POINT_Y) + view.dy);
    follow_down(0);
    tail();
}

/*
 * @routine 81:C5A4 ScrollUpdate
 * WHAT IT DOES: Moves the play field's scroll for this frame the way the
 * level asks (g.scroll_kind): the camera that follows the player (0); a
 * scroll running on right by $A0 a frame (in 256ths) keeping the player no
 * further right than $F0 (+ 2 dx: 16 from the view's right edge) on the screen when g.player_no_actor_landing's low
 * 4 bits are 0 (2); by 3 pixels a frame (4); by 3 pixels (6, $300 in
 * 256ths); 8 left and 8 down (8); at a set speed with the camera following
 * up and down (10, 14); or none but the backdrop (12). Then the scroll kept
 * in the level and the backdrop set (not for 12, nor for 0 with the camera
 * held). Kind 16 is a bare return. The view (view.h): the camera's point
 * x is $80 + dx from the scroll, its y point and deadzones lie dy further
 * down, and the limits right and down are 2 dx and 2 dy less.
 * HOW: Kind 4 adds 0 to the fraction first, which changes nothing.
 */
void ScrollUpdate(void)
{
    ScrollLayer *l = &g.layers[0];
    Actor *p = &g.actors[0];

    switch (g.scroll_kind) {
    case 0:
        follow();
        return;
    case 2:
        l->scroll_x = add24(l->scroll_x, 0x00A0);
        if ((g.player_no_actor_landing & 0x000F) == 0) {
            uint16_t most = (uint16_t)(pixels(l->scroll_x) + 0x00F0 + 2 * view.dx);

            if (most < pixels(p->x))
                p->x = with_pixels(p->x, most);
        }
        tail();
        return;
    case 4:
        g.scroll_x_before = pixels(l->scroll_x);
        l->scroll_x = with_pixels(l->scroll_x, (uint16_t)(pixels(l->scroll_x) + 3));
        tail();
        return;
    case 6:
        l->scroll_x = add24(l->scroll_x, 0x0300);
        tail();
        return;
    case 8:
        g.scroll_x_before = pixels(l->scroll_x);
        g.scroll_y_before = pixels(l->scroll_y);
        l->scroll_x = with_pixels(l->scroll_x, (uint16_t)(pixels(l->scroll_x) - 8));
        l->scroll_y = with_pixels(l->scroll_y, (uint16_t)(pixels(l->scroll_y) + 8));
        tail();
        return;
    case 10:
        run_at(0);
        return;
    case 12:
        ParallaxUpdate();
        return;
    case 14:
        run_at(4);
        return;
    case 16:
        return;
    default: {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: scroll kind %u is past the handlers' table\n", g.scroll_kind);
        }
        return;
    }
    }
}

/*
 * guard_states.c: actor kind $08's wait routine: each frame its place kept,
 * its animation and picture, then the step of the state it is in: calm or
 * chasing, facing the view, idle, holding back while the player is to one
 * side, or running off to the right.
 */
#include <stdio.h>
#include "video/view.h"

#include "actor/guard.h"
#include "actor/kinds_d.h"
#include "video/video.h"

/* $1471, $1473: its x and y pixels, kept each frame (nothing else in the
   record reads them) */
static uint16_t kept_x, kept_y;

/* GuardWait's state 1: held while a move is under way; else to its
   chase's pose (g.screen_signal bit 7), to its walk once Aladdin has no
   health, or a chase step. */
static void calm(Actor *a)
{
    if (a->own.bytes[G_MOVING]) {
        g.screen_signal &= 0x80;
        return;
    }
    if (g.screen_signal & 0x80)
        a->resume = G_CHASE_POSE;
    else if (!g.health)
        a->resume = G_WALK;
    else
        guard_chase(a);
}

/*
 * @routine A5:F07A GuardHoldLeft
 * WHAT IT DOES: The guard's state 4: while the player is to its left, it
 * stops when it faces left and, unless a move is under way, keeps pace
 * with him (the chase's close step); once he is level with it or to its
 * right, it goes calm (state 1) at once.
 * Also A5:F09F GuardHoldRight, state 5, the sides swapped: it stops when
 * it faces right.
 */
static void hold(Actor *a, bool right)
{
    uint16_t x = pixels(a->x), px = pixels(g.actors[0].x);

    if (right ? x >= px : px >= x) {
        a->own.bytes[G_STATE] = 0x01;
        calm(a);
        return;
    }
    if (right ? !(a->facing | a->flip) : (a->facing | a->flip)) {
        a->speed_x = 0;                 /* the original's three words of zeros from +$19: its speed across and accel_x */
        a->accel_x = 0;
    }
    if (!(a->own.bytes[G_MOVING] | a->own.bytes[G_LANDING]))
        guard_chase_close(a);
}

/*
 * @routine A5:F0C4 GuardRunOff
 * WHAT IT DOES: The guard's state 6: unless a move is under way, it runs
 * off to the right at speed (5 pixels a frame), and once it is more than
 * 192 pixels right of the player it is gone: counted, state 0, and moved
 * far off ($3F of its x and y's high bytes).
 */
static void run_off(Actor *a)
{
    if (a->own.bytes[G_MOVING]) {
        g.screen_signal &= 0x80;
        return;
    }
    if ((uint16_t)(pixels(g.actors[0].x) + 0xC0) < pixels(a->x)) {
        a->own.bytes[G_GONE]++;
        a->own.bytes[G_STATE] = 0;
        a->x = with_pixels(a->x, (uint16_t)((pixels(a->x) & 0x00FF) | 0x3F00));
        a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0x00FF) | 0x3F00));
        return;
    }
    guard_chase_speed_up(a);
    a->speed_x = (int32_t)((a->speed_x & ~0xFF00) | 0x0500);
    a->facing = 0;
    a->flip = 0;
}

/*
 * @routine A5:F2E5 GuardFaceView
 * WHAT IT DOES: The guard's state 2: it faces toward the point actors are
 * kept in range of: left when it is more than 96 * w / 256 pixels right of it,
 * right when it is more than that left of it (96 at 256 across).
 */
static void face_view(Actor *a)
{
    /* its dead zone is round the player (the camera keeps him at the point): 96 either side, grown with the view (decision 2) */
    uint16_t half = (uint16_t)((0x60 * view.w + VIEW_ORIGINAL_W / 2) / VIEW_ORIGINAL_W);
    uint16_t ahead = (uint16_t)(g.range_x + half);

    if (ahead < pixels(a->x)) {
        a->facing = a->flip = 1;
        return;
    }
    if ((uint16_t)(ahead - pixels(a->x)) >= (uint16_t)(2 * half))
        a->facing = a->flip = 0;
}

/*
 * @routine 83:9209 ActorKind08Wait
 * WHAT IT DOES: Kind $08's wait routine, through A5:F020 GuardWait: its
 * place kept, its animation stepped and its picture sent; then by its
 * state: 0 nothing; 1 calm or chasing; 2 facing the view (GuardFaceView);
 * 3 nothing (A5:F308 GuardIdle); 4 and 5 holding back while the player is
 * to its left or right; 6 running off to the right.
 */
int ActorKind08Wait(Actor *a)
{
    kept_x = pixels(a->x);
    kept_y = pixels(a->y);
    AnimStep(a);
    ActorPictureSend(a);
    switch (a->own.bytes[G_STATE]) {
    case 0:
    case 3:
        break;
    case 1:
        calm(a);
        break;
    case 2:
        face_view(a);
        break;
    case 4:
        hold(a, false);
        break;
    case 5:
        hold(a, true);
        break;
    case 6:
        run_off(a);
        break;
    default: {
        static int told;

        /* past the table's 7 words: no code sets such a state */
        if (!told++)
            fprintf(stderr, "ActorKind08Wait: state %u (past its table) not converted\n", a->own.bytes[G_STATE]);
        break;
    }
    }
    return ACTOR_GOES_ON;
}

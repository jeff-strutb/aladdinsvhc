/*
 * kind5c.c: actor kind $5C of level 8, the Genie's head that leads
 * Aladdin across: once he is right of x $08A0 it flies in to x $0900 and
 * then along a path (one by his lives) of points, bobbing about its line;
 * at each point it makes the next of its children (clouds, kinds $5D, $8B,
 * $8C) and, after the first, waits until he has landed on the last one;
 * at the path's end it flies off and ends. Out of view, it ends; with
 * Aladdin out of health it only bobs. Its wait routine is Kind5CWait.
 */
#include "actor/actor_own.h"
#include "video/view.h"
#include "actor/kinds_d.h"
#include "data/kinds_d_tables.h"
#include "video/video.h"

/* its own bytes */
#define BOB OWN_13              /* ActorBobAboutLine's state */
#define PATH OWN_16             /* its path's place, k5c_path's word (+$16-$17) */
#define FIRST_DONE OWN_18       /* nonzero once its first child is made */
#define LINE OWN_40             /* the y it bobs about (word) */
#define TARGET OWN_42           /* the point it flies to, x pixels (word) */
#define TOP_SPEED OWN_44        /* its speed's limit by distance (word, 8.8) */
#define SPEED OWN_46            /* its speed across (word, 8.8) */
#define STEP OWN_48             /* its speed's change a frame */
#define HEADING OWN_49          /* the way it moves: 0 right, 1 left */
#define AT OWN_4A               /* its point's offset in the path */
#define GONE OWN_4E             /* counted up as it ends */
#define CHILD_LANDED OWN_40     /* a child's: nonzero once Aladdin has landed on it */

static uint16_t own16(const Actor *a, unsigned at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

static void own16_set(Actor *a, unsigned at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

enum {
    WAIT_PLAYER = ACTOR_RESUME_OWN, /* until Aladdin is right of x $08A0 */
    FLY_IN,                     /* each frame of its flight in */
    FIRST_POINT,                /* after 63 frames at x $0900 */
    TO_POINT,                   /* each frame to its point */
    WAIT_LANDED,                /* until he has landed on its last child */
    POSE,                       /* after 15 frames of pose 2 */
    MAKE_CHILD,                 /* each frame until the point's child is made */
    NEXT_POINT,                 /* after 15 frames */
    OFF_WAIT,                   /* the path's end: after 176 frames */
    FLY_OFF,                    /* each frame off, while it can be hit */
    END_WAIT,                   /* after 255 frames */
    GONE_AT,                    /* 83:E128 Kind5CGone: out of view (its wait sends it here) */
    GONE_END,                   /* after 31 frames */
    DONE_AT,                    /* 83:E282 Kind5CDone: Aladdin out of health (its wait sends it here) */
    DONE_BOB                    /* each frame: only its bob */
};

/*
 * @routine 83:E134 Kind5CUnhittable
 * WHAT IT DOES: It can no longer be hit and is not tested: flags bit 7 and
 * +$08 bit 3 cleared.
 */
static void unhittable(Actor *a)
{
    a->flags &= 0x7F;
    a->anim_flags &= 0xF7;
}

/*
 * @routine 83:E13F Kind5CSpeedPick
 * WHAT IT DOES: Its speed limit and step by its distance to its point (the
 * nearer, the slower): the distance, $80 or more taken as $70, / 4, picks
 * the entry.
 */
static void speed_pick(Actor *a)
{
    uint16_t d = (uint16_t)(pixels(a->x) - own16(a, TARGET));
    uint8_t at;

    if (d & 0x8000)
        d = (uint16_t)-d;
    if (d >= 0x80)
        d = 0x70;
    at = (uint8_t)((d >> 2) & 0x1C);
    own16_set(a, TOP_SPEED, k5c_speed_limit(at));
    a->own.bytes[STEP] = k5c_speed_step(at);
}

/* Its speed less `taken`; below 0: 0, and it heads the way it faces. */
static void slow(Actor *a, uint8_t taken)
{
    uint16_t speed = own16(a, SPEED);

    if (speed < taken) {
        own16_set(a, SPEED, 0);
        a->own.bytes[HEADING] = a->facing;
    } else {
        own16_set(a, SPEED, (uint16_t)(speed - taken));
    }
}

/*
 * @routine 83:E167 Kind5CMove
 * WHAT IT DOES: Moves it across by its speed: heading the way it faces,
 * its speed grows by its step up to its limit (or drops by the step when
 * above it); facing the other way, it drops by 8 a frame until it turns
 * about. Then its x on or back by the speed.
 */
static void move(Actor *a)
{
    uint16_t limit = own16(a, TOP_SPEED);

    if (a->facing != a->own.bytes[HEADING]) {
        slow(a, 0x08);
    } else if (limit < own16(a, SPEED)) {
        slow(a, a->own.bytes[STEP]);
    } else {
        uint16_t speed = (uint16_t)(own16(a, SPEED) + a->own.bytes[STEP]);

        own16_set(a, SPEED, limit < speed ? limit : speed);
    }
    if (!a->own.bytes[HEADING])
        a->x = wrap24(a->x + own16(a, SPEED));
    else
        a->x = wrap24(a->x - own16(a, SPEED));
}

/*
 * @routine 83:E1DE Kind5CFace
 * WHAT IT DOES: Faces it toward its point: 0 when the point is at or right
 * of its x, 1 when left.
 */
static void face(Actor *a)
{
    a->facing = own16(a, TARGET) < pixels(a->x) ? 1 : 0;
}

/*
 * @routine 83:E22A Kind5CPathByLives
 * WHAT IT DOES: Its path by Aladdin's lives: 0 with one, 1 with two or
 * three, 2 with four or more (or none).
 */
uint8_t Kind5CPathByLives(void)
{
    uint8_t left = (uint8_t)(g.lives - 1);

    return !left ? 0 : left < 3 ? 1 : 2;
}

/* On to the point at AT: its x the target. */
static int to_point(Actor *a)
{
    own16_set(a, TARGET, k5c_path_x(own16(a, PATH), a->own.bytes[AT]));
    a->resume = TO_POINT;
    return 1;
}

/* At a point: pose 2 for 15 frames. */
static int pose(Actor *a)
{
    AnimStart(a, 2);
    a->resume = POSE;
    return 0x0F;
}

/*
 * @routine 83:E239 ActorBobAboutLine
 * WHAT IT DOES: Moves it by its BOB state: 0 still; 1 motion preset $47,
 * then (2) flown against its motion until its y is above LINE; 3 preset
 * $48, then (4) flown until its y is back at or below LINE, and again.
 * Only kind $5C calls it.
 */
static void bob_about_line(Actor *a)
{
    switch (a->own.bytes[BOB]) {
    case 1:
        ActorMotionPreset(a, 0x47);
        a->own.bytes[BOB]++;
        /* fall through */
    case 2:
        ActorFlyAgainst(a);
        if (pixels(a->y) < own16(a, LINE))
            a->own.bytes[BOB]++;
        return;
    case 3:
        ActorMotionPreset(a, 0x48);
        a->own.bytes[BOB]++;
        /* fall through */
    case 4:
        ActorFly(a);
        if (pixels(a->y) >= own16(a, LINE))
            a->own.bytes[BOB] = 1;
        return;
    default:                            /* 0: still (past 4 the original jumps where the table's next words point; never set) */
        return;
    }
}

/*
 * @routine 83:E000 Kind5CStart
 * WHAT IT DOES: Actor kind $5C: waits until Aladdin is right of x $08A0,
 * then comes in at that x above the view (y $FFFF) and flies in (preset
 * $45) to x $0900; there it bobs about its line and follows its path (by
 * his lives): to each point (Kind5CFace, Kind5CSpeedPick, Kind5CMove),
 * then (from the second point on, once he has landed on its last child)
 * its poses 2 and 3, the point's child made ($07 the point's number,
 * ActorCreateChild; the two name each other at +$37), the next point; at
 * the path's end it waits 176 frames, flies off (preset $46) until it can
 * no longer be hit, waits 255 frames and ends.
 * Also 83:E128 Kind5CGone (out of view: it can no longer be hit, waits 31
 * frames and ends) and 83:E282 Kind5CDone (Aladdin out of health: pose 4,
 * and only its bob from then on), where its wait routine sends it.
 */
int Kind5CStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->resume = WAIT_PLAYER;
        return 1;
    case WAIT_PLAYER:
        if (pixels(g.actors[0].x) <= 0x08A0)
            return 1;
        a->x = with_pixels(a->x, 0x08A0);
        a->own_pictures = k5c_pictures();
        a->y = with_pixels(a->y, 0xFFFF);
        a->view_box = 0x18;
        a->anim_flags = 0x02;
        own16_set(a, SPEED, 0);
        a->own.bytes[STEP] = 0;
        a->own.bytes[HEADING] = 0;
        a->own.bytes[GONE] = 0;
        a->own.bytes[FIRST_DONE] = 0;
        own16_set(a, PATH, k5c_path(Kind5CPathByLives()));
        a->own.bytes[AT] = 0;
        a->spawn_at = 0x50;
        a->flags = 0x90;
        a->redraw = 0xFF;
        ActorMotionPreset(a, 0x45);
        AnimStart(a, 0);
        a->resume = FLY_IN;
        return 1;
    case FLY_IN:
        ActorFlyAgainst(a);
        if (pixels(a->x) < 0x0900)
            return 1;
        AnimStart(a, 1);
        a->flip = 1;
        own16_set(a, LINE, pixels(a->y));
        a->own.bytes[BOB]++;
        a->resume = FIRST_POINT;
        return 0x3F;
    case FIRST_POINT:
        return to_point(a);
    case TO_POINT:
        face(a);
        speed_pick(a);
        move(a);
        if (pixels(a->x) < k5c_path_x(own16(a, PATH), a->own.bytes[AT]))
            return 1;
        if (!a->own.bytes[FIRST_DONE]) {
            a->own.bytes[FIRST_DONE]++;
            return pose(a);
        }
        a->resume = WAIT_LANDED;
        return 1;
    case WAIT_LANDED:
        if (!a->next->own.bytes[CHILD_LANDED])
            return 1;
        return pose(a);
    case POSE:
        AnimStart(a, 3);
        a->resume = MAKE_CHILD;
        return 1;
    case MAKE_CHILD: {
        Actor *child;

        a->spawn_byte = (uint8_t)(a->own.bytes[AT] >> 2);
        child = ActorCreateChild(a, k5c_path_kind(own16(a, PATH), a->own.bytes[AT]));
        if (!child)
            return 1;
        a->next = child;
        child->next = a;
        child->own.bytes[CHILD_LANDED] = 0;
        a->resume = NEXT_POINT;
        return 0x0F;
    }
    case NEXT_POINT:
        a->own.bytes[AT] = (uint8_t)(a->own.bytes[AT] + 4);
        if (!k5c_path_end(own16(a, PATH), a->own.bytes[AT]))
            return to_point(a);
        a->resume = OFF_WAIT;
        return 0xB0;
    case OFF_WAIT:
        a->own.bytes[BOB] = 0;
        AnimStart(a, 0);
        ActorMotionPreset(a, 0x46);
        a->resume = FLY_OFF;
        return 1;
    case FLY_OFF:
        ActorFlyAgainst(a);
        if (a->flags & 0x40)
            return 1;
        unhittable(a);
        a->resume = END_WAIT;
        return 0xFF;
    case END_WAIT:
        a->own.bytes[GONE]++;
        return ActorEndPlaced(a);
    case GONE_AT:
        unhittable(a);
        a->own.bytes[GONE]++;
        a->resume = GONE_END;
        return 0x1F;
    case GONE_END:
        return ActorEndPlaced(a);
    case DONE_AT:
        AnimStart(a, 4);
        ActorPictureSend(a);
        a->resume = DONE_BOB;
        return 1;
    case DONE_BOB:
        bob_about_line(a);
        AnimStep(a);
        ActorPictureSend(a);
        return 1;
    case ACTOR_START_SECOND:            /* 83:96CA ActorEndPlaced */
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:E1EE Kind5CWait
 * WHAT IT DOES: Kind $5C's wait routine: it bobs about its line, animated
 * and drawn; out of the view (more than $140 pixels to either side of
 * g.range_x) its code goes on at Kind5CGone when its wait runs out; in
 * view with Aladdin out of health, at Kind5CDone after one frame, its wait
 * routine off.
 */
int Kind5CWait(Actor *a)
{
    bob_about_line(a);
    AnimStep(a);
    ActorPictureSend(a);
    if ((uint16_t)(pixels(a->x) - g.range_x + 0x140 + view.dx) >= (uint16_t)(0x280 + 2 * view.dx)) {
        a->resume = GONE_AT;
        return ACTOR_GOES_ON;
    }
    if (g.health)
        return ACTOR_GOES_ON;
    a->resume = DONE_AT;
    a->wait = 1;
    a->flags &= 0x7F;
    return ACTOR_GOES_ON;
}

/*
 * kind58_start.c: actor kind $58, the platforms of stage a3 s0 that move
 * to and fro along a direction, which Aladdin rides or hangs from: its
 * start, the pose it shows each frame (Kind58Pose, from kind58.c) and its
 * wait (Kind58Wait, from kind_waits_rest.c).
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

/* its own bytes: +$40-$45 */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of this leg or rest */
    uint8_t leg;                /* +$41: frames of its first leg (a half leg); the others are twice */
    uint8_t copier;             /* +$42: frames to the copier test (counted by nothing that reads it here) */
    uint8_t carries;            /* +$43: nonzero: Aladdin rides it this frame (ActorTestCarries) */
    int8_t lift;                /* +$44: ActorLiftPlayerStep's step: 0 none, positive hung from, negative let go */
    uint8_t pose;               /* +$45: its animation: 0, 1 ridden, 3 the variant, 4/5 hung from, $FF Aladdin dead */
} Kind58Own;

_Static_assert(sizeof(Kind58Own) <= OWN_BYTES, "a view fits the own bytes");

static inline Kind58Own *own(Actor *a) { return (Kind58Own *)a->own.bytes; }

#define VIEW_X_AT_END 0x07D0    /* in level 8, the view's left edge from which it ends for good */

enum {
    FORWARD = ACTOR_RESUME_OWN, /* each frame of a leg along its direction */
    REST_1,                     /* each frame of its $40 at the far end */
    BACK,                       /* each frame of a leg back */
    REST_2,                     /* each frame of its $40 at home */
};

/*
 * @routine 83:DC4E Kind58Pose
 * WHAT IT DOES: Kind $58's pose for the frame: ridden, hung from, or
 * neither, and the dead pose once Aladdin has no health; starts the
 * animation when the pose changes. By its pose: negative, nothing. 0 or 3
 * (the variant): health 0, the dead pose $FF (animation 2); else pose 1
 * when it carries him, else (+$44 positive) pose 4 plus his flip; else
 * nothing. 1: pose 0 unless it carries him; when it does, +$42 = $20. 2:
 * the dead pose. 4 and up: pose 0 once +$44 is 0.
 */
static void pose(Actor *a)
{
    Kind58Own *o = own(a);
    uint8_t to;

    if (o->pose & 0x80)
        return;
    switch (o->pose) {
    case 0:
    case 3:
        if (!g.health) {
            o->pose = 0xFF;
            AnimStart(a, 0x02);
            return;
        }
        if (o->carries)
            to = 0x01;
        else if (o->lift > 0)
            to = (uint8_t)(g.actors[0].flip + 4);
        else
            return;
        break;
    case 1:
        if (o->carries) {
            o->copier = 0x20;
            return;
        }
        to = 0x00;
        break;
    case 2:
        o->pose = 0xFF;
        AnimStart(a, 0x02);
        return;
    default:
        if (o->lift)
            return;
        to = 0x00;
        break;
    }
    o->pose = to;
    AnimStart(a, to);
}

/* each frame of a leg: its pose, moved by its direction, carrying him */
static void leg_step(Actor *a, int back)
{
    pose(a);
    if (back)
        ActorMoveBackByVector(a, 0x14);
    else
        ActorMoveByVector(a, 0x14);
    ActorCarryPlayer(a);
}

/*
 * @routine 83:DBBB Kind58Start
 * WHAT IT DOES: Actor kind $58 started: a platform that moves along its
 * direction and back, resting $40 frames at each end, showing by its
 * animation whether Aladdin rides it or hangs from it. +$08 bit 1 and its
 * wait routine on, +$28 $FF, flip 0; +$07 bit 4 clear: pose 0; set: +$07
 * &= $0F, pose 3. Its first leg lasts the frames its +$07 gives, its
 * direction too; then for good: that leg, $40 frames of rest, twice as
 * many frames back, $40 of rest, and again with the longer leg.
 */
int Kind58Start(Actor *a)
{
    Kind58Own *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->redraw = 0xFF;
        a->flip = 0;
        o->carries = 0;
        o->lift = 0;
        if (a->spawn_byte & 0x10) {
            a->spawn_byte &= 0x0F;
            o->pose = 3;
            AnimStart(a, 0x03);
        } else {
            o->pose = 0;
            AnimStart(a, 0x00);
        }
        o->leg = k58_leg(a->spawn_byte);
        o->frames = o->leg;
        a->facing = k58_direction(a->spawn_byte);
        a->resume = FORWARD;
        return 1;
    case FORWARD:
        leg_step(a, 0);
        if (--o->frames)
            return 1;
        o->frames = 0x40;
        a->resume = REST_1;
        return 1;
    case REST_1:
        pose(a);
        if (--o->frames)
            return 1;
        o->frames = (uint8_t)(o->leg << 1);
        a->resume = BACK;
        return 1;
    case BACK:
        leg_step(a, 1);
        if (--o->frames)
            return 1;
        o->frames = 0x40;
        a->resume = REST_2;
        return 1;
    case REST_2:
        pose(a);
        if (--o->frames)
            return 1;
        o->frames = (uint8_t)(o->leg << 1);
        a->resume = FORWARD;
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:DC9B Kind58Wait
 * WHAT IT DOES: Kind $58's wait, an actor that carries and lifts Aladdin:
 * its animation; (on a copier, whose cartridge RAM reads back, Aladdin
 * would be put on it when +$42 counts out: the retail board reads open
 * bus there, plan section 1, item 8: not converted); the carry test with
 * its box ($2C) raised 8 and the lift step; in level 8 with the view's
 * left edge past $07D0 it ends for good unless it can be hit (flags bit
 * 6); else it ends once out of range by box $18.
 */
int Kind58Wait(Actor *a)
{
    uint16_t y = pixels(a->y);

    AnimStep(a);
    a->y = with_pixels(a->y, (uint16_t)(y - 8));
    a->own.bytes[OWN_4C] = 0x2C;
    ActorTestCarries(a, 0x00);
    a->y = with_pixels(a->y, y);
    a->own.bytes[OWN_4A] = (uint8_t)y;
    a->own.bytes[OWN_4B] = (uint8_t)(y >> 8);
    ActorLiftPlayerStep(a, 0x01);
    if (g.level == 8 && pixels(g.layers[0].scroll_x) >= VIEW_X_AT_END) {
        if (a->flags & 0x40)
            return ACTOR_GOES_ON;
        return ActorEndPlaced(a);
    }
    return ActorEndOutOfRange(a, 0x18);
}

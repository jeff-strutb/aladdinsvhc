/*
 * kind2a.c: actor kind $2A: its start, which paces (or, no script, leaps
 * at Aladdin), its second state, downed: knocked away, a short flight up
 * and a fall; and its wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t leap;               /* +$40: the leaper: frames left of its leap */
    uint8_t below[2];           /* +$41: the leaper: how far below it he must be (word) */
    uint8_t frames;             /* +$43: frames left of a pace or its flight */
    uint8_t leg;                /* +$44: frames of each pace after the first */
    uint8_t speed[2];           /* +$45: its pacing speed (word) */
    uint8_t downs;              /* +$47: counted each time it is downed; nonzero, its wait no longer animates or tests */
    uint8_t hits;               /* +$48: counted when downed by a shot (its wait) */
    uint8_t bob;                /* +$49: nonzero: it bobs with its frames (its wait) */
    uint8_t bob_frame;          /* +$4A: the frame it last bobbed on */
    uint8_t up;                 /* +$4B: its wait's bob direction */
    uint8_t right;              /* +$4C: counted when downed facing right (its wait) */
} Kind2AOwn;

_Static_assert(sizeof(Kind2AOwn) <= OWN_BYTES, "a view fits the own bytes");

static inline Kind2AOwn *own(Actor *a) { return (Kind2AOwn *)a->own.bytes; }
static uint16_t word(const uint8_t *w) { return (uint16_t)(w[0] | w[1] << 8); }

enum {
    LOOK_FOR = ACTOR_RESUME_OWN,    /* the leaper: each frame until he is below and near */
    SPOTTED,                        /* after its 5 frames in pose 1 */
    RISE,                           /* each frame of its rise to him */
    DROP,                           /* each frame of its drop on him */
    DROP_STOP,                      /* each frame until the drop stops */
    DROP_WAIT,                      /* after its $20 frames */
    LEAP,                           /* each frame of its leap */
    LEAP_ON,                        /* each frame after it, for good */
    PACE,                           /* each frame of a pace at its speed */
    SLOW,                           /* each frame of slowing to a stop */
    TURN,                           /* after its 8 frames stopped */
    TURNED,                         /* after its 8 frames turned */
    SPEED_UP,                       /* each frame of speeding up */
    DOWNED_WAIT,                    /* the second state: after its 9 frames */
    FLY,                            /* each of its 16 frames up */
    FALL,                           /* each frame of its fall for good */
};

/* The carry the original's last 24-bit add of its y and speed down left
   (ActorStepY's, ActorFall's), which its next compare adds in. */
static unsigned y_carry(const Actor *a)
{
    return wrap24(a->y) < wrap24((uint32_t)a->speed_y);
}

static void face_player(Actor *a)
{
    a->facing = ActorSideOfPlayer(a);
    a->flip = (uint8_t)(a->facing ^ 0x01);
}

/* the leap at him: accel x $20, $38 frames, facing him */
static int leap(Actor *a)
{
    a->accel_x = 0x20;
    own(a)->leap = 0x38;
    face_player(a);
    a->resume = LEAP;
    return 1;
}

/* the leaper's test (83:A21F): below and near ($0C), pose 1 for 5 frames */
static int look_for(Actor *a)
{
    if (!ActorPlayerBelow(a) || !ActorNearPlayer(a, 0x0C)) {
        a->resume = LOOK_FOR;
        return 1;
    }
    AnimStart(a, 0x01);
    a->resume = SPOTTED;
    return 5;
}

/* the leaper's rise (83:A244): each frame a step up, accelerating below
   speed $400, until its y plus +$41 reaches his */
static int rise(Actor *a)
{
    ActorStepY(a);
    if ((uint16_t)(pixels(a->y) + word(own(a)->below) + y_carry(a)) >= pixels(g.actors[0].y)) {
        AnimStart(a, 0x02);
        a->accel_y = 0x18;
        return leap(a);
    }
    if ((uint16_t)a->speed_y < 0x0400)
        ActorAccelerateY(a);
    a->resume = RISE;
    return 1;
}

/* the pace: `frames` frames at its speed from the next frame */
static int pace(Actor *a, uint8_t frames)
{
    own(a)->frames = frames;
    a->resume = PACE;
    return 1;
}

/*
 * @routine 83:A1E5 Kind2AStart
 * WHAT IT DOES: Actor kind $2A: with bit 4 of +$07 it paces across at the
 * speed of its variant (its low digit), slowing to a stop, turning and
 * speeding up again, bobbing with its frames (its wait). Without it (no
 * script has that) it waits until Aladdin is below and near, then by +$07
 * (halved) rises toward him or drops on him, and leaps across at him and
 * on for good. Its start: +$4C 0, its wait routine on, pose 0, motion
 * preset 6, +$47-$49 and +$4B 0. Its second entry is Kind2ASecond.
 */
int Kind2AStart(Actor *a)
{
    Kind2AOwn *o = own(a);

    switch (a->resume) {
    case ACTOR_START: {
        uint8_t variant;
        uint16_t speed;

        o->right = 0;
        a->flags |= 0x90;
        AnimStart(a, 0x00);
        ActorMotionPreset(a, 0x06);
        o->downs = 0;
        o->hits = 0;
        o->bob = 0;
        o->up = 0;
        if (!(a->spawn_byte & 0x10)) {
            o->below[0] = k2a_reach(a->spawn_byte);
            o->below[1] = 0;
            a->spawn_byte >>= 1;
            face_player(a);
            return look_for(a);
        }
        variant = (uint8_t)(a->spawn_byte & 0x0F);
        speed = k2a_speed((uint8_t)(variant << 1));
        a->speed_x = speed;
        o->speed[0] = (uint8_t)speed;
        o->speed[1] = (uint8_t)(speed >> 8);
        o->leg = (uint8_t)(k2a_leg(variant) << 1);
        AnimStart(a, 0x02);
        a->facing = 0;
        a->flip = 1;
        o->bob++;
        o->bob_frame = 0xFF;
        a->accel_x = 0x14;
        return pace(a, k2a_leg(variant));
    }
    case ACTOR_START_SECOND:
        return Kind2ASecond(a);
    case LOOK_FOR:
        return look_for(a);
    case SPOTTED:
        if (!a->spawn_byte)
            return rise(a);
        a->accel_y = 0x18;
        a->resume = DROP;
        return Kind2AStart(a);
    case RISE:
        return rise(a);
    case DROP:
        /* each frame it falls until its y plus $18 reaches his */
        ActorFall(a);
        if ((uint16_t)(pixels(a->y) + 0x18 + y_carry(a)) < pixels(g.actors[0].y))
            return 1;
        a->accel_y = 0x80;
        a->resume = DROP_STOP;
        return Kind2AStart(a);
    case DROP_STOP:
        ActorStepY(a);
        ActorDecelerateY(a);
        if ((uint16_t)a->speed_y)
            return 1;
        AnimStart(a, 0x02);
        a->resume = DROP_WAIT;
        return 0x20;
    case DROP_WAIT:
        return leap(a);
    case LEAP:
        if ((uint16_t)a->speed_x < 0x0240)
            ActorAccelerateX(a);
        ActorDecelerateY(a);
        ActorMove(a);
        if (--o->leap)
            return 1;
        a->accel_y = 0x06;
        a->resume = LEAP_ON;
        return Kind2AStart(a);
    case LEAP_ON:
        ActorAccelerateY(a);
        ActorMoveX(a);
        ActorStepYBack(a);
        return 1;
    case PACE:
        ActorMoveX(a);
        if (--o->frames)
            return 1;
        a->resume = SLOW;
        return 1;
    case SLOW:
        ActorMoveX(a);
        ActorDecelerateX(a);
        if ((uint16_t)a->speed_x)
            return 1;
        a->resume = TURN;
        return 8;
    case TURN:
        a->flip = a->facing;
        a->facing ^= 0x01;
        a->resume = TURNED;
        return 8;
    case TURNED:
        a->resume = SPEED_UP;
        return 1;
    case SPEED_UP:
        ActorMoveX(a);
        ActorAccelerateX(a);
        if ((uint16_t)a->speed_x < word(o->speed))
            return 1;
        a->speed_x = signed24((wrap24((uint32_t)a->speed_x) & ~0xFFFFu) | word(o->speed));
        return pace(a, o->leg);
    case DOWNED_WAIT:
        ActorMotionPreset(a, 0x28);
        o->frames = 0x10;
        a->resume = FLY;
        return 1;
    case FLY:
        ActorFly(a);
        if (--o->frames)
            return 1;
        AnimStart(a, 0x03);
        ActorMotionReset(a, 0x10);
        a->resume = FALL;
        return 1;
    case FALL:
        /* the original also bumps a byte of cartridge RAM each frame and,
           on a board where it reads back (a copier), pushes Aladdin 16
           pixels right; the retail board has none (plan section 1, item
           8): not converted */
        AnimStep(a);
        if ((uint8_t)((uint32_t)a->speed_y >> 8) != 0x03)
            ActorAccelerateY(a);
        ActorStepY(a);
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:A37B Kind2ASecond
 * WHAT IT DOES: Actor kind $2A in its second state (downed): pose 4,
 * motion preset $28, its wait routine on, +$47 up (its wait stops). A
 * facing with bit 7 (no script has it): facing and flip its bit 0, its
 * speed across one pixel more, straight to the flight. Else, not downed
 * facing right: sound $0F and 9 pixels down; not downed by a shot: facing
 * away from Aladdin's flip, 9 frames first; motion preset $28 again. The
 * flight: 16 frames up (ActorFly), pose 3, gravity $10, then it falls for
 * good (its speed held at 3), its wait ending it out of range.
 */
int Kind2ASecond(Actor *a)
{
    Kind2AOwn *o = own(a);

    AnimStart(a, 0x04);
    ActorMotionPreset(a, 0x28);
    a->flags |= 0x80;
    o->downs++;
    if (a->facing & 0x80) {
        uint32_t v = wrap24((uint32_t)a->speed_x);

        a->facing &= 0x01;
        a->flip = a->facing;
        a->speed_x = signed24((v & ~0xFF00u) | (((v >> 8) + 1) & 0xFF) << 8);
        o->frames = 0x10;
        a->resume = FLY;
        return 1;
    }
    if (!o->right) {
        SoundQueueAdd(0x0F);
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + 9));
    }
    if (!o->hits) {
        a->flip = g.actors[0].flip;
        a->facing = (uint8_t)(a->flip ^ 0x01);
        a->resume = DOWNED_WAIT;
        return 9;
    }
    a->resume = DOWNED_WAIT;
    return Kind2AStart(a);
}

/*
 * @routine 83:A413 Kind2AWait
 * WHAT IT DOES: Kind $2A's wait: while +$49 is set it bobs with its
 * animation at each new frame (3 down on frame 4, 2 up or down on frame 5,
 * 3 up on frame 6); until downed (+$47) it animates, is stomped or hurts,
 * and when downed by a shot (no script has it) counts +$48, and +$4C when
 * its facing has no bit 7; ended when out of range (box 8).
 */
int Kind2AWait(Actor *a)
{
    Kind2AOwn *o = own(a);

    if (o->bob && a->picture != o->bob_frame) {
        uint16_t y = pixels(a->y);
        int moved = 1;

        o->bob_frame = a->picture;
        switch (a->picture) {
        case 4:
            y = (uint16_t)(y + 3);
            o->up = 0;
            break;
        case 5:
            y = (uint16_t)(o->up ? y + 2 : y - 2);
            break;
        case 6:
            y = (uint16_t)(y - 3);
            o->up++;
            break;
        default:
            moved = 0;
            break;
        }
        if (moved)
            a->y = with_pixels(a->y, y);
    }
    if (!o->downs) {
        AnimStep(a);
        (void)ActorStompOrHurt(a);
        if (ActorDownedByShot(a)) {
            if (!(a->facing & 0x80))
                o->right++;
            o->hits++;
        }
    }
    return ActorEndOutOfRange(a, 0x08);
}

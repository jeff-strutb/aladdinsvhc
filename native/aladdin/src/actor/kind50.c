/*
 * kind50.c: actor kind $50 of level 0, which rises out of the ground in
 * steps, waits facing Aladdin, sinks back and waits again, for good; and
 * its downed state. Its wait routine is Kind50Wait (kind_waits_rest.c).
 */
#include "actor/kinds_c.h"
#include "system/system.h"

#define K50_STOMP_SOUND 0x0F    /* the sound when it is downed by a stomp, not a shot */
#define K50_FALL 0x2F           /* ActorMotionPreset's preset for its fall away */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of its wait below or on top */
    uint8_t top;                /* +$41: frames it waits each time ($20 or $40) */
    uint8_t step;               /* +$42: frames left of a step */
    uint8_t pose;               /* +$43: its animation: 6 down, 0 up */
    uint8_t low;                /* +$44: its second state: 7 once its pose came to the lowest of its fall */
    uint8_t downed;             /* +$45: counted up by Kind50Wait when a shot downs it */
} Kind50Own;

_Static_assert(sizeof(Kind50Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind50Own *own(Actor *a) { return (Kind50Own *)a->own.bytes; }

enum {
    BELOW = ACTOR_RESUME_OWN,   /* each frame hidden below the ground */
    RISING,                     /* each frame of the 7 before a step up */
    TOP,                        /* each frame on top */
    SINKING,                    /* each frame of the 7 before a step down */
    HIT_SHOWN,                  /* its second state: after its first pose's 3 frames */
    HIT_RISES,                  /* each frame of its hit poses */
    FALLING,                    /* each frame of its fall away */
};

/*
 * @routine 83:D1A7 Kind50Face
 * WHAT IT DOES: Turns the actor to face Aladdin.
 */
static void face(Actor *a)
{
    a->facing = a->flip = ActorSideOfPlayer(a);
}

/*
 * @routine 83:D18F Kind50Down8
 * WHAT IT DOES: Moves the actor 8 pixels down.
 */
static void down8(Actor *a)
{
    a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + 8));
}

/*
 * @routine 83:D19B Kind50Up8
 * WHAT IT DOES: Moves the actor 8 pixels up (its SBC takes the borrow of a
 * clear carry: 7 and 1).
 */
static void up8(Actor *a)
{
    a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 8));
}

/*
 * @routine 83:D18E Kind50Still
 * WHAT IT DOES: Nothing: kind $50 calls it where it turns between rising
 * and sinking. (Not converted: an RTS.)
 */

/*
 * @routine 83:D109 Kind50Start
 * WHAT IT DOES: Actor kind $50 started: hidden in the ground, it waits a
 * random time, rises in 6 steps (7 frames each), waits on top, sinks in 6
 * steps, waits, and again, always facing Aladdin.
 */
static int start(Actor *a)
{
    Kind50Own *o = own(a);

    o->downed = 0;
    a->anim_flags = 0x03;
    a->flags |= 0x90;
    face(a);
    o->pose = 6;
    AnimStart(a, 6);
    o->frames = (uint8_t)RandomFromList(4);
    o->top = (int8_t)RandomNext() < 0 ? 0x40 : 0x20;
    a->resume = BELOW;
    return 1;
}

/* Hidden: facing him for frames + 1 frames, then the rise's first step. */
static int below(Actor *a)
{
    Kind50Own *o = own(a);

    face(a);
    if (!((uint8_t)--o->frames & 0x80))
        return 1;
    o->frames = o->top;
    o->step = 7;
    a->resume = RISING;
    return 1;
}

/* Rising: every 7 frames the pose one less and 8 pixels up (7 and the
   borrow), down to pose 0, which is held 7 frames, then on top. */
static int rising(Actor *a)
{
    Kind50Own *o = own(a);

    if (--o->step)
        return 1;
    if ((int8_t)--o->pose < 0) {
        o->pose++;
        a->resume = TOP;
        return 1;
    }
    AnimStart(a, o->pose);
    up8(a);
    o->step = 7;
    return 1;
}

/* On top: facing him for its frames, then the sink's first step. */
static int top(Actor *a)
{
    Kind50Own *o = own(a);

    face(a);
    if (--o->frames)
        return 1;
    o->step = 7;
    a->resume = SINKING;
    return 1;
}

/* Sinking: every 7 frames the pose one more and 8 pixels down, to pose 6;
   then below again. */
static int sinking(Actor *a)
{
    Kind50Own *o = own(a);

    if (--o->step)
        return 1;
    if (++o->pose == 7) {
        o->pose--;
        o->frames = o->top;
        a->resume = BELOW;
        return 1;
    }
    AnimStart(a, o->pose);
    down8(a);
    o->step = 7;
    return 1;
}

/*
 * @routine 83:D1C7 Kind50Second
 * WHAT IT DOES: Kind $50 downed (its second state, state $8C): by a stomp
 * it plays sound $0F; it shows its hit poses (7 up to 13) as it sinks a
 * step and rises again, then falls away out of view and ends there.
 */
static int downed(Actor *a)
{
    Kind50Own *o = own(a);

    if (a->facing & 0x80)
        a->facing = a->flip = a->facing & 0x01;
    if (!o->downed)
        SoundQueueAdd(K50_STOMP_SOUND);
    o->pose = (uint8_t)(13 - o->pose);
    AnimStart(a, o->pose);
    a->resume = HIT_SHOWN;
    return 3;
}

static int hit_shown(Actor *a)
{
    Kind50Own *o = own(a);
    uint8_t pose = (uint8_t)(o->pose - 1);

    o->low = 0;
    if (pose == 6) {
        o->low = 7;
        pose = 7;
    }
    o->pose = pose;
    AnimStart(a, pose);
    if (!o->low)
        down8(a);
    a->resume = HIT_RISES;
    return 8;
}

static int hit_rises(Actor *a)
{
    Kind50Own *o = own(a);

    if (++o->pose == 0x0E) {
        AnimStart(a, 0x0E);
        ActorMotionPreset(a, K50_FALL);
        a->resume = FALLING;
        return 1;
    }
    AnimStart(a, o->pose);
    up8(a);
    return 1;
}

int Kind50Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return downed(a);
    case BELOW:
        return below(a);
    case RISING:
        return rising(a);
    case TOP:
        return top(a);
    case SINKING:
        return sinking(a);
    case HIT_SHOWN:
        return hit_shown(a);
    case HIT_RISES:
        return hit_rises(a);
    case FALLING:
        AnimStep(a);
        ActorFly(a);
        if (ActorEndOutOfRange(a, 0x00) == ACTOR_ENDED)
            return ACTOR_ENDED;
        return 1;
    default:
        return kind_lost(a, "Kind50Code");
    }
}

/*
 * kind_a1_a6.c: actor kind $A1 (it bounces about at random; hit, it flies
 * off) with its wait, and kind $A6 (it stays until far above or below the
 * point actors are kept in range of).
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"

#define BOUNCE_LONG 0x6A        /* the long bounce, taken once; after it $68 */
#define BOUNCE_AFTER 0x68
#define BOUNCE_LIST 0x40        /* RandomFromList's list of its bounces */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t long_taken;         /* +$40: set once it has taken the long bounce */
    uint8_t hits;               /* +$41: counted up each time Aladdin comes down on it */
} KindA1Own;

_Static_assert(sizeof(KindA1Own) <= OWN_BYTES, "a view fits the own bytes");

static inline KindA1Own *own(Actor *a) { return (KindA1Own *)a->own.bytes; }

enum {
    BOUNCE = ACTOR_RESUME_OWN,  /* $A1: each frame of its bouncing */
    HIT_WAIT,                   /* $A1 hit: after its 8 frames */
    FLY_OFF,                    /* each frame of its flight off */
    STAY,                       /* $A6: each frame */
};

/* hit: motion $1F, then each frame it flies while it can be hit */
static int fly_off(Actor *a)
{
    ActorMotionPreset(a, 0x1F);
    a->resume = FLY_OFF;
    return 1;
}

/*
 * @routine 84:A97F KindA1Start
 * WHAT IT DOES: Actor kind $A1: it bounces about: each time it lands (kept
 * 12 above the floor line), sound $4D and a bounce (motion preset) picked
 * at random from list $40; the long bounce ($6A) only once, then $68 in
 * its place. +$08 3, flags $D0 (its wait routine on, it can be hit),
 * facing left, pose 0, motion $6B to start. Its second entry is KindA1Hit.
 */
int KindA1Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0x03;
        a->flags = 0xD0;
        a->facing = 0x01;
        own(a)->long_taken = 0;
        own(a)->hits = 0;
        AnimStart(a, 0x00);
        ActorMotionPreset(a, 0x6B);
        a->resume = BOUNCE;
        return 1;
    case BOUNCE: {
        uint8_t pick;

        ActorFly(a);
        if (!ActorClampAboveFloor12(a))
            return 1;
        SoundQueueAdd(0x4D);
        pick = (uint8_t)RandomFromList(BOUNCE_LIST);
        if (pick == BOUNCE_LONG) {
            if (own(a)->long_taken)
                pick = BOUNCE_AFTER;
            else
                own(a)->long_taken++;
        }
        ActorMotionPreset(a, pick);
        return 1;
    }
    case ACTOR_START_SECOND:
        return KindA1Hit(a);
    case HIT_WAIT:
        return fly_off(a);
    case FLY_OFF:
        ActorFly(a);
        if (a->flags & 0x40)
            return 1;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 84:A9FC KindA1Hit
 * WHAT IT DOES: Actor kind $A1's second entry (hit): pose 0, +$08 bit 4
 * set; after 8 frames when +$41 is set, it flies off (motion $1F) each
 * frame while it can still be hit, then ends.
 */
int KindA1Hit(Actor *a)
{
    AnimStart(a, 0x00);
    a->anim_flags |= 0x10;
    if (own(a)->hits) {
        a->resume = HIT_WAIT;
        return 8;
    }
    return fly_off(a);
}

/*
 * @routine 84:A9C7 KindA1Touch
 * WHAT IT DOES: Kind $A1's wait (through its stub 83:940D): its animation
 * on; once it can no longer be hit (flags bit 6) it ends; else, unless
 * g.chain_head_struck is set, Aladdin touching it is hurt or, coming down
 * on it, stands on it, which counts a hit and plays sound $0F and sends it
 * to state $8C; otherwise a shot can down it. With g.chain_head_struck set
 * it goes to state $8C at once. State $8C also turns its wait routine off.
 */
int KindA1Touch(Actor *a)
{
    AnimStep(a);
    if (!(a->flags & 0x40))
        return ActorEndPlaced(a);
    if (!g.chain_head_struck) {
        if (ActorBouncesOrHurtsPlayer(a) != 1) {
            (void)ActorDownedByShot(a);
            return ACTOR_GOES_ON;
        }
        own(a)->hits++;
        SoundQueueAdd(0x0F);
    }
    a->flags &= (uint8_t)~0x80;
    a->state = 0x8C;
    return ACTOR_GOES_ON;
}

/*
 * @routine 84:AC3F KindA6Stay
 * WHAT IT DOES: Actor kind $A6: shown (pose 0, its flip its +$07, +$08 3)
 * where it was placed until it is 256 or more pixels above or below the
 * point actors are kept in range of, then ends; it tests from the next
 * frame.
 */
int KindA6Stay(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0x03;
        AnimStart(a, 0x00);
        a->flip = a->spawn_byte;
        a->resume = STAY;
        return 1;
    default:
        if ((uint16_t)(g.range_y - pixels(a->y) + 0x0100) < 0x0200)
            return 1;
        return ActorEndPlaced(a);
    }
}

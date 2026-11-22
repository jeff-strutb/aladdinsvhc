/*
 * kind70_start.c: actor kind $70, a scripted actor of a scene: with +$07
 * zero it rises, flies off and carries the escort once it meets it; else
 * it waits to be hit and then cues the scene on.
 *
 * The same code is kind $6E's second entry (83:F296 in the table at
 * $83:8DFD): kind $6E's code is its start's owner's.
 */
#include "actor/actor_own.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t carrying;           /* +$40: frames of the first flight; then nonzero while it carries the escort */
} K70Own;

static inline K70Own *k70(Actor *a) { return (K70Own *)a->own.bytes; }

enum {
    HIT_WAIT = ACTOR_RESUME_OWN,    /* the hit one: until it can be hit */
    HIT_CUE,                    /* after $80 frames */
    HIT_POSE,                   /* after $20 frames of animation 4 */
    STAY,
    POSE1,                      /* the flyer: after 32 frames of animation 0 */
    HOP,                        /* after 23 frames of animation 1 */
    FLIGHT,                     /* 40 frames of flight */
    UP                          /* up against the pull until out of range, carrying the escort once in reach */
};

/* The escort within reach of its hold point: 24 left of the flyer, 16
   below: within 8 across (one way) and 16 down. */
static bool escort_in_reach(const Actor *a, const Actor *escort)
{
    uint16_t dx = (uint16_t)(pixels(a->x) - 0x18 - pixels(escort->x));
    uint16_t dy = (uint16_t)(pixels(a->y) + 0x10 - pixels(escort->y));

    return dx < 0x08 && dy < 0x10;
}

/*
 * @routine 83:F296 Kind70Start
 * WHAT IT DOES: Actor kind $70, a scripted actor (flags $90). +$07
 * nonzero: it faces left with animation 3 and waits until it can be hit
 * (flags bit 6), counts the scene's cue (g.scripted_cue) up, after $80 frames
 * counts the second cue (g.scene_cue_waited) up, shows animation 4 for $20
 * frames and animation 5, and stays. +$07 zero: animation 0 for $20
 * frames, animation 1 for $17, then 16 right and 13 up with animation 2,
 * motion preset $4F and $28 frames of flight; then gravity $18 and each
 * frame up against the pull until out of range (box 0). While it carries
 * the escort (actors[1]) the escort is 30 to its left and level with it;
 * until then, when the escort is not rising and in reach of its hold
 * point, it counts the scene's cue up and takes hold.
 */
int Kind70Start(Actor *a)
{
    K70Own *k = k70(a);
    Actor *escort = &g.actors[1];

    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x90;
        if (a->spawn_byte) {
            a->facing = 1;
            a->flip = 1;
            AnimStart(a, 3);
            a->resume = HIT_WAIT;
            return 1;
        }
        k->carrying = 0;
        AnimStart(a, 0);
        a->resume = POSE1;
        return 0x20;
    case HIT_WAIT:
        if (!ACTOR_ON_SCREEN(a))
            return 1;
        g.scripted_cue++;
        a->resume = HIT_CUE;
        return 0x80;
    case HIT_CUE:
        g.scene_cue_waited++;
        AnimStart(a, 4);
        a->resume = HIT_POSE;
        return 0x20;
    case HIT_POSE:
        AnimStart(a, 5);
        a->resume = STAY;
        return 1;

    case POSE1:
        AnimStart(a, 1);
        a->resume = HOP;
        return 0x17;
    case HOP:
        a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) + 0x10));
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 0x0D));
        AnimStart(a, 2);
        ActorMotionPreset(a, 0x4F);
        k->carrying = 0x28;
        a->resume = FLIGHT;
        return 1;
    case FLIGHT:
        ActorFly(a);
        if (--k->carrying)
            return 1;
        a->accel_y = 0x18;
        a->resume = UP;
        return 1;
    case UP:
        ActorFlyAgainst(a);
        if (ActorEndOutOfRange(a, 0) == ACTOR_ENDED)
            return ACTOR_ENDED;
        if (k->carrying) {
            escort->x = with_pixels(escort->x, (uint16_t)(pixels(a->x) - 0x1E));
            escort->y = with_pixels(escort->y, pixels(a->y));
            return 1;
        }
        if (escort->speed_y < 0 || !escort_in_reach(a, escort))
            return 1;
        g.scripted_cue++;
        k->carrying++;
        return 1;
    default:                            /* STAY */
        return 1;
    }
}

/*
 * @routine 83:F36B ActorKind70Wait
 * WHAT IT DOES: Kind $70's wait routine: its animation plays on (AnimStep).
 */
int Kind70Wait(Actor *a)
{
    AnimStep(a);
    return ACTOR_GOES_ON;
}

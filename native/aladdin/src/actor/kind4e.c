/*
 * kind4e.c: actor kind $4E of stage a5 s0, a thing Aladdin bounces off when
 * he comes down on it: it then plays a squash and a spring back. Its wait
 * routine is Kind4EWait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"

#define POSE OWN_41             /* the animation of the squash or spring step */
#define STILL OWN_42            /* nonzero: Aladdin does not bounce on it (while it springs) */

enum {
    REST = ACTOR_RESUME_OWN,    /* for good */
    BOUNCED,                    /* 83:CFC1 Kind4EBounced: where its wait routine sends it */
    SQUASHED,                   /* after 16 frames of animation 7 */
    SQUASH_STEP,                /* after 8 frames of each of animations 5 down to 0 */
    SQUASH_HOLD,                /* after 48 frames */
    SPRING_STEP                 /* after 8 frames of each of animations 0 up to 5 */
};

/* The rest, for good: not still, animation 6. */
static int rest(Actor *a)
{
    a->own.bytes[STILL] = 0;
    AnimStart(a, 6);
    a->resume = REST;
    return 1;
}

/*
 * @routine 83:CFA9 Kind4EStart
 * WHAT IT DOES: Actor kind $4E started: it rests in animation 6; Aladdin
 * may bounce on it (Kind4EWait).
 * Also 83:CFC1 Kind4EBounced (where Kind4EWait sends it once he has
 * bounced): animation 7 for 16 frames, still (he cannot bounce again),
 * animations 5 down to 0 for 8 frames each, 48 frames, animations 0 up to
 * 5 for 8 frames each, then the rest again.
 */
int Kind4EStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x03;
        a->flags |= 0x80;
        return rest(a);
    case BOUNCED:
        AnimStart(a, 7);
        a->resume = SQUASHED;
        return 0x10;
    case SQUASHED:
        a->own.bytes[STILL]++;
        a->own.bytes[POSE] = 0x05;
        AnimStart(a, 5);
        a->resume = SQUASH_STEP;
        return 8;
    case SQUASH_STEP:
        if ((int8_t)--a->own.bytes[POSE] >= 0) {
            AnimStart(a, a->own.bytes[POSE]);
            return 8;
        }
        a->resume = SQUASH_HOLD;
        return 0x30;
    case SQUASH_HOLD:
        a->own.bytes[POSE] = 0;
        AnimStart(a, 0);
        a->resume = SPRING_STEP;
        return 8;
    case SPRING_STEP:
        if (a->own.bytes[POSE] < 5) {   /* it stays 5 at the rest, as the original's */
            AnimStart(a, ++a->own.bytes[POSE]);
            return 8;
        }
        return rest(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:CFF8 Kind4EWait
 * WHAT IT DOES: Kind $4E's wait routine: unless it is still, Aladdin coming
 * down on it bounces off it (ActorBouncesPlayer), its code then going on
 * at Kind4EBounced, with sound $12; its animation steps; it ends once out
 * of range (box 0).
 */
int Kind4EWait(Actor *a)
{
    if (!a->own.bytes[STILL] && ActorBouncesPlayer(a)) {
        a->resume = BOUNCED;
        SoundQueueAdd(0x12);
    }
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x00);
}

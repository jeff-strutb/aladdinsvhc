/*
 * kind_5e.c: actor kind $5E: a brief show of its animation, then its end.
 */
#include "actor/actor_own.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left */
} K5EOwn;

static inline K5EOwn *k5e(Actor *a) { return (K5EOwn *)a->own.bytes; }

enum { SHOW = ACTOR_RESUME_OWN };

/*
 * @routine 83:E42A Kind5EBrief
 * WHAT IT DOES: Actor kind $5E: look $0600, anim_flags bits 0-1, its wait
 * routine on (flags $90), animation $0C for $23 frames, then it ends. (The
 * original tests its +$07 bit 2 and picks the same animation and frames
 * either way.) Its second entry (state $80 up), 83:E461, ends it
 * (ActorEndPlaced).
 */
int Kind5EBrief(Actor *a)
{
    if (a->resume == ACTOR_START_SECOND)    /* 83:E461: a JML to ActorEndPlaced */
        return ActorEndPlaced(a);
    if (a->resume == SHOW) {
        if (--k5e(a)->frames)
            return 1;
        return ActorEndPlaced(a);
    }
    a->look = 0x0600;
    a->anim_flags |= 0x03;
    a->flags |= 0x90;
    k5e(a)->frames = 0x23;
    AnimStart(a, 0x0C);
    a->resume = SHOW;
    return 1;
}

/*
 * @routine 83:E458 ActorWaitAnimRange10
 * WHAT IT DOES: Kind $5E's wait routine: its animation on, and it ends
 * once out of range (box $10).
 */
int ActorWaitAnimRange10(Actor *a)
{
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x10);
}

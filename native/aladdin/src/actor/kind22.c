/*
 * kind22.c: actor kind $22, a thing that stays where it was placed
 * and hurts Aladdin when he touches it. (kind20_dazed.c's other
 * routine, 83:9C49 Kind20Dazed, is kind $20's third entry, in kind20.c.)
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

enum { STAY = ACTOR_RESUME_OWN };

/*
 * @routine 83:9CD2 Kind22Start
 * WHAT IT DOES: Actor kind $22: anim_flags 0, its wait routine on (flags
 * bit 7), and it stays where it was placed for good.
 * Also 83:9CDE Kind22Second, its second entry (state $80-$BF): it ends
 * (ActorEndPlaced).
 */
int Kind22Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0;
        a->flags |= 0x80;
        a->resume = STAY;
        return 1;
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    default:                            /* STAY */
        return 1;
    }
}

/*
 * @routine 83:9CE2 Kind22Wait
 * WHAT IT DOES: Kind $22's wait routine (each frame it waits): Aladdin
 * touching it is hurt; it ends when out of range by box $18.
 */
int Kind22Wait(Actor *a)
{
    (void)ActorHurtsPlayerOnTouch(a);
    return ActorEndOutOfRange(a, 0x18);
}

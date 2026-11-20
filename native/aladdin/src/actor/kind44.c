/*
 * kind44.c: actor kind $44, which shows its animation 4 and ends. Its wait
 * routine is Kind44Wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

#define FRAMES OWN_40           /* frames left */

/*
 * @routine 83:B45B Kind44Start
 * WHAT IT DOES: Actor kind $44: shown (flags $90, +$08 bit 1), its look
 * $0600, facing right; animation 4 for $20 frames, then it ends.
 */
int Kind44Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->look = 0x0600;
        a->facing = a->flip = 0;
        a->own.bytes[FRAMES] = 0x20;
        AnimStart(a, 4);
        a->resume = ACTOR_RESUME_OWN;
        return 1;
    default:
        return --a->own.bytes[FRAMES] ? 1 : ActorEndPlaced(a);
    }
}

/*
 * @routine 83:B488 Kind44Wait
 * WHAT IT DOES: Kind $44's wait routine: its animation steps, and it ends
 * once out of range (box 0).
 */
int Kind44Wait(Actor *a)
{
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x00);
}

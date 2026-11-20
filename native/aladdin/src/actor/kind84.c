/*
 * kind84.c: actor kind $84, spawned by the grabber: it shows its first
 * animation for 16 frames and ends. Its second entry in the record's table
 * is its wait work, as code, which its flags never reach. Its wait routine
 * is ActorKind84Wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

/*
 * @routine 84:95E3 Kind84Start
 * WHAT IT DOES: Kind $84's start: shown (+$08 bit 1, flags $90), animation
 * 0 for 16 frames, then it ends.
 */
int Kind84Start(Actor *a)
{
    if (a->resume == ACTOR_START) {
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        AnimStart(a, 0);
        a->resume = ACTOR_RESUME_OWN;
        return 0x10;
    }
    return ActorEndPlaced(a);
}

/*
 * @routine 83:92E9 ActorKind84Wait
 * WHAT IT DOES: Kind $84's wait routine, through 84:95FD Kind84WaitWork:
 * its animation steps.
 */
int ActorKind84Wait(Actor *a)
{
    AnimStep(a);
    return ACTOR_GOES_ON;
}

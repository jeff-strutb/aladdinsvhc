/*
 * kind5f.c: actor kind $5F, a brief picture: shown by its $07's animation
 * for 31 frames, then gone.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

#define LEFT OWN_40             /* frames left of its showing */

/*
 * @routine 83:E465 Kind5FStart
 * WHAT IT DOES: Actor kind $5F: shows animation $07 for 31 frames, then
 * ends for good. (Its second entry and wait, 83:E480 GroundPushMark, are
 * the player's code: left aside.)
 */
int Kind5FStart(Actor *a)
{
    if (a->resume == ACTOR_START) {
        a->flags = 0x10;
        AnimStart(a, a->spawn_byte);
        a->own.bytes[LEFT] = 0x1F;
        a->resume = ACTOR_RESUME_OWN;
        return 1;
    }
    AnimStep(a);
    return --a->own.bytes[LEFT] ? 1 : ActorEndPlaced(a);
}

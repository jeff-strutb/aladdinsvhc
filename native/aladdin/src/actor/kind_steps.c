/*
 * kind_steps.c: what kinds that only animate, send their own pictures or
 * go once out of range do each frame they wait; and the steps of an actor
 * along a path.
 *
 * Four of the record's routines here do only AnimStep and are the actor
 * core's AnimateWhileWaiting (kinds.c), which the rows of their kinds name:
 *   @routine 84:9533 KindAnimate9533 (kind $82's wait)
 *   @routine 84:9628 KindAnimate9628 (kind $85's wait and second entry's work)
 *   @routine 84:9EF0 KindAnimate9EF0 (kind $95's wait)
 *   @routine 84:9F4A KindAnimate9F4A (kind $96's wait)
 * and three more the same, for the rows of kinds $24, $A3 and $9C:
 *   @routine 84:8048 Kind24Animate
 *   @routine 84:8399 KindA3Animate
 *   @routine 84:A81B Kind9CAnimate
 * Two more are KindAnimateSend's code exactly, for the rows of kinds $21
 * and $93:
 *   @routine A5:F3E3 Kind21AnimateSend
 *   @routine A5:F434 Kind93AnimateSend
 * Three more are each one kind's wait, so its kind's owner writes them
 * (one kind, one owner): 84:9387 KindAnimateFreeOff (kind $7F, kinds C),
 * 84:A975 KindA0AnimateFreeOff (kind $A0, kinds D), A5:F603 Kind91FreeOff
 * (kinds $91 and $92, kinds D). The path steps (84:8CAF ActorPathStep,
 * 84:8DFB ActorPathNext) come with kind $73, which calls them.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"

/*
 * @routine 84:903B KindAnimateSend
 * WHAT IT DOES: One frame of a kind that animates its own picture: its
 * animation on, then its picture sent (ActorPictureSend). Kind $76's wait, and (as Kind21AnimateSend and
 * Kind93AnimateSend) kinds $21 and $93's.
 */
int KindAnimateSend(Actor *a)
{
    AnimStep(a);
    ActorPictureSend(a);
    return ACTOR_GOES_ON;
}




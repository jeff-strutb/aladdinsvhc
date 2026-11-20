/*
 * player_grab.c: an actor that takes hold of the player.
 */
#include "actor/actor_a.h"
#include "player/player.h"

#define HOLDS spawn_at          /* its +$21: nonzero while it holds him */

/*
 * @routine 83:BAF3 ActorHoldPlayer
 * WHAT IT DOES: A frame of an actor that takes hold of the player: when
 * ActorGrabTest finds him it holds him (its +$21 set) at its offset
 * +$13, his code going on as he hangs (PLAYER_AT_GRABBED_HANG); while it
 * holds him he stays there until g.player_no_actor_landing is cleared, which
 * lets him go with a pause of 8 frames (its +$4F) before it can hold him
 * again.
 */
void ActorHoldPlayer(Actor *a)
{
    if (a->HOLDS) {
        if (!(uint8_t)g.player_no_actor_landing) {
            a->HOLDS = 0;
            a->own.bytes[OWN_4F] = 8;
            return;
        }
        PlayerToActorAbove(a, a->own.bytes[OWN_13]);
        return;
    }
    if (a->own.bytes[OWN_4F]) {
        a->own.bytes[OWN_4F]--;
        return;
    }
    if (!ActorGrabTest(a))
        return;
    g.actors[0].resume = PLAYER_AT_GRABBED_HANG;
    PlayerSpotAboveActor(a, a->own.bytes[OWN_13]);
    a->HOLDS++;
}

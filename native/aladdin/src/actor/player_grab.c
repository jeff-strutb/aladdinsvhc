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

/*
 * @routine 83:888C PlayerFacesActorClose
 * WHAT IT DOES: Tells whether Aladdin is close in front of the actor: his
 * x plus 22 (8 when he faces right) less the actor's below 30, and his y
 * plus 3 less the actor's below 26, unsigned.
 */
static bool PlayerFacesActorClose(const Actor *a)
{
    const Actor *p = &g.actors[0];
    uint16_t reach = p->flip ? 0x0008 : 0x0016;

    return (uint16_t)(reach + pixels(p->x) - pixels(a->x)) < 0x001E &&
           (uint16_t)(pixels(p->y) + 3 - pixels(a->y)) < 0x001A;
}

/*
 * @routine 83:882E ActorGrabbableByPlayer
 * WHAT IT DOES: Tells whether Aladdin can take hold of the actor: he is
 * close in front of it (PlayerFacesActorClose) and either
 * g.player_no_actor_landing is set, or no state $8C is counted, the box his
 * code wants is not 7 to $0F, he is in the air, and he is not rising faster
 * than 2 pixels a frame.
 * Also 83:8861 ActorGrabbableByPlayerLong, the same for other banks.
 */
bool ActorGrabbableByPlayer(const Actor *a)
{
    uint16_t rise;

    if (!PlayerFacesActorClose(a))
        return false;
    if ((uint8_t)g.player_no_actor_landing)
        return true;
    if (g.player_hurt_count)
        return false;
    if (g.player_box_wanted >= 0x07 && g.player_box_wanted < 0x10)
        return false;
    if (!g.player_in_air)
        return false;
    rise = pixels((uint32_t)g.actors[0].speed_y);
    return !(rise & 0x8000) || rise >= 0xFFFE;
}

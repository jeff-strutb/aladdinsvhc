/*
 * actor_84_misc.c: steps of bank $84's actors.
 */
#include "actor/actor_a.h"

/*
 * @routine 84:832B ActorAnimFreeFar
 * WHAT IT DOES: Plays the actor's animation, lets it hurt or be stomped
 * by the player (ActorStompOrHurt), and frees it when it is out of range
 * by box $58; then its parent (the record its own $4C names, as its index
 * in g.actors) counts one fewer in its own $42. ACTOR_ENDED when freed.
 */
int ActorAnimFreeFar(Actor *a)
{
    AnimStep(a);
    (void)ActorStompOrHurt(a);
    if (!ActorFreeOutOfRange(a, 0x58))
        return ACTOR_GOES_ON;
    g.actors[a->own.bytes[OWN_4C]].own.bytes[OWN_42]--;
    return ACTOR_ENDED;
}

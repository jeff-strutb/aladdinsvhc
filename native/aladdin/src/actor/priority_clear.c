/*
 * priority_clear.c: an actor's sprite priority.
 */
#include "actor/actor_a.h"

/*
 * @routine 84:81B2 ActorPriorityClear
 * WHAT IT DOES: Clears the actor's sprite priority bits (its flags' bits
 * 4-5), except in level $0E.
 */
void ActorPriorityClear(Actor *a)
{
    if ((uint8_t)g.level == 0x0E)
        return;
    a->flags &= (uint8_t)~0x30;
}

/*
 * actor_state_8c.c: state $8C (with bit 7 of the byte at $08 set and of
 * the byte at $09 cleared) for the actor's own record.
 */
#include "actor/actor_own.h"

/*
 * @routine 83:8254 ActorStateSet8CSelf
 * WHAT IT DOES: Puts the actor into state $8C, with bit 7 of its byte at
 * $08 set and of its byte at $09 cleared (it can no longer be hit):
 * ActorStateSet8C for the actor's own record.
 * Also 83:8263 ActorStateSet8CLong, ActorStateSet8C for callers in other
 * banks.
 */
void ActorStateSet8CSelf(Actor *a)
{
    ActorStateSet8C(a);
}

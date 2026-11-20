/*
 * rise_fall.c: an actor posed by its rise or fall.
 */
#include "actor/actor_own.h"

/*
 * @routine A5:EA20 ActorAnimRiseFall
 * WHAT IT DOES: Shows an actor rising or falling: animation $0C while it
 * rises (its vertical speed's top byte negative), $0E while it falls.
 */
void ActorAnimRiseFall(Actor *a)
{
    ActorAnimChange(a, ((a->speed_y >> 16) & 0x80) ? 0x0C : 0x0E);
}

/*
 * pool_second.c: the second pool of actors (actors[3..7]).
 */
#include "actor/actor_own.h"

/*
 * @routine 83:95C1 SecondPoolFreeAll
 * WHAT IT DOES: Frees every record of the second pool that is in use (as a
 * level ends): ActorFree and ActorTileSlotRelease for each whose state is
 * nonzero.
 */
void SecondPoolFreeAll(void)
{
    int i;

    for (i = 3; i < 3 + ACTOR_POOL_B_SIZE; i++) {
        Actor *a = &g.actors[i];

        if (a->state) {
            ActorFree(a);
            ActorTileSlotRelease(a);
        }
    }
}

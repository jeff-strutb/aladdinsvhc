/*
 * pool.c: the actor records an actor can be created in. The pool's 28
 * records (actors[8..35]) and the second pool's 5 (actors[3..7]) are each
 * kept on a stack of the free ones.
 */
#include <string.h>

#include "actor/actor_own.h"

#define POOL_FIRST (ACTOR_RECORDS - ACTOR_POOL_SIZE)      /* actors[8] */
#define POOL_B_FIRST 3                                     /* actors[3] */

/*
 * @routine 81:AADD ActorRecordsInit
 * WHAT IT DOES: Empties every actor record and fills the free lists for a
 * new level: the second pool's 5 records with actors[3] on top, the pool's
 * 28 with actors[8] on top; each of the 28 gets its number, 28 for
 * actors[35] down to 1 for actors[8]; the four palette slots have no users.
 */
void ActorRecordsInit(void)
{
    int k;

    memset(g.actors, 0, sizeof g.actors);
    for (k = 0; k < ACTOR_POOL_B_SIZE; k++)
        g.free_pool_b[k] = &g.actors[POOL_B_FIRST + ACTOR_POOL_B_SIZE - 1 - k];
    g.free_pool_b_top = ACTOR_POOL_B_SIZE - 1;
    for (k = 0; k < ACTOR_POOL_SIZE; k++) {
        g.free_pool[k] = &g.actors[ACTOR_RECORDS - 1 - k];
        g.actors[POOL_FIRST + k].pool_number = (uint8_t)(k + 1);
    }
    g.free_pool_top = ACTOR_POOL_SIZE - 1;
    memset(g.palette_slot_users, 0, sizeof g.palette_slot_users);
}

/*
 * @routine 81:AB3F ActorAlloc
 * WHAT IT DOES: Takes a free record of the pool, if there is one.
 */
Actor *ActorAlloc(void)
{
    if (g.free_pool_top < 0)
        return NULL;
    return g.free_pool[g.free_pool_top--];
}

/*
 * @routine 81:EA41 ActorAllocB
 * WHAT IT DOES: Takes a free record from the second pool, if there is one.
 * Also 81:EA3D ActorAllocBLong, the same for callers in other banks.
 */
Actor *ActorAllocB(void)
{
    if (g.free_pool_b_top < 0)
        return NULL;
    return g.free_pool_b[g.free_pool_b_top--];
}

/*
 * @routine 81:9D82 ActorsFreeAll
 * WHAT IT DOES: Puts every record of the pool back on the free list: the
 * list emptied, then each record from actors[35] down to actors[8] freed as
 * an actor that ends (ActorFreePlaced), in use or not, so each counts one
 * off its kind and lets go of its palette slot.
 */
void ActorsFreeAll(void)
{
    int i;

    g.free_pool_top = -1;
    for (i = ACTOR_RECORDS - 1; i >= POOL_FIRST; i--)
        ActorFreePlaced(&g.actors[i]);
    g.free_pool_top = ACTOR_POOL_SIZE - 1;
}

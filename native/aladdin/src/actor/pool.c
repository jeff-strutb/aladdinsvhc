/*
 * pool.c: the actor records an actor can be created in. The pool's 28
 * records (actors[8..35]) and the second pool's 5 (actors[3..7]) are each
 * kept on a stack of the free ones.
 */
#include <stdio.h>
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

static long at(const Actor *a) { return a - g.actors; }

/* How a pass decides to swap a neighbouring pair (the entry below and the
   entry above it, nearer the top). */
typedef enum {
    SMALL_ON_TOP,           /* 81:ADC0: when the one below is the lower record */
    LARGE_ON_TOP_OR_SAME,   /* 81:ADFC: when the one below is the higher, or the same */
    LARGE_ON_TOP            /* 81:AE38: when the one below is the higher */
} SortOrder;

/* `passes` passes of a bubble sort of the free list from entry `from` down:
   each pass compares as many neighbouring pairs as passes are left. */
static void sort_from(int from, uint16_t passes, SortOrder order)
{
    for (; passes; passes--) {
        int k = from, pairs;

        for (pairs = passes; pairs > 0; pairs--, k--) {
            Actor *below = g.free_pool[k - 1], *above = g.free_pool[k];
            int swap = order == SMALL_ON_TOP ? at(below) < at(above)
                     : order == LARGE_ON_TOP_OR_SAME ? at(below) >= at(above) : at(below) > at(above);

            if (swap) {
                g.free_pool[k - 1] = above;
                g.free_pool[k] = below;
            }
        }
    }
}

/*
 * @routine 81:ADC0 ActorFreeListSort
 * WHAT IT DOES: Orders the top of the free list so the lowest records are
 * taken first, when more than `passes` records are free: that many passes
 * of a bubble sort from the top down. -1 when none is free or too few (the
 * original goes on into 81:ADF9 ReturnFFLong, A = $FF), 0 when sorted.
 */
int ActorFreeListSort(uint8_t passes)
{
    if (g.free_pool_top < 0 || g.free_pool_top < passes)
        return -1;
    sort_from(g.free_pool_top, passes, SMALL_ON_TOP);
    return 0;
}

/*
 * @routine 81:ADFC ActorFreeListSortHigh
 * WHAT IT DOES: Orders the top of the free list so the highest records are
 * taken first, when more than `passes` records are free. -1 when none is
 * free or too few, 0 when sorted.
 */
int ActorFreeListSortHigh(uint8_t passes)
{
    if (g.free_pool_top < 0 || g.free_pool_top < passes)
        return -1;
    sort_from(g.free_pool_top, passes, LARGE_ON_TOP_OR_SAME);
    return 0;
}

/*
 * @routine 81:AE38 ActorFreeListTake
 * WHAT IT DOES: Takes `count` records (2 or more) off the top of the free
 * list at once, ordered so the lowest comes first (count - 1 passes from
 * the old top). Returns the first taken's place in g.free_pool, the rest
 * after it; -1 with too few free. (The original's A is 2 * (count - 1).)
 */
int ActorFreeListTake(uint8_t count)
{
    int old_top = g.free_pool_top;

    if (count < 2) {
        static int told;

        /* A = 0: the original's pass counters start at 0 and count down
           through 65,536 passes of 256 pairs, below the list's start */
        if (!told++)
            fprintf(stderr, "ActorFreeListTake: a take of one record not converted\n");
        return -1;
    }
    if (old_top < 0 || old_top < count - 1)
        return -1;
    g.free_pool_top = (int8_t)(old_top - count);
    sort_from(old_top, (uint16_t)(count - 1), LARGE_ON_TOP);
    return g.free_pool_top + 1;
}

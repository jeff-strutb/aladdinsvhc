/*
 * kind92.c: actor kind $92, a marker that puts the level's escort actor
 * (actors[1], a guard: guard.h) at its own place and clears the escort's
 * state, then ends. Its wait routine is Kind91FreeOff (kinds E's: kinds $91
 * and $92 share it).
 */
#include <stdio.h>

#include "actor/guard.h"
#include "actor/kinds_d.h"
#include "actor/kinds_e.h"

#define ESCORT_47 OWN_47        /* the escort's +$47: the marker's $07 (A5:E826's table is read by it) */

/*
 * @routine A5:F5BA Kind92Start
 * WHAT IT DOES: Actor kind $92: flags bit 7 set; the escort's x and y are its
 * own, its resume point the code at A5:E826 (the escort's own scene, not
 * converted: the guard's G_NOT_CONVERTED reports it), its +$47 its $07. By
 * $07: 0, the escort's state and its busy count cleared; 1, 2, 3, the state
 * cleared. Then it ends.
 */
int Kind92Start(Actor *a)
{
    Actor *e = &g.actors[1];

    a->flags |= 0x80;
    e->x = with_pixels(e->x, pixels(a->x));
    e->y = with_pixels(e->y, pixels(a->y));
    e->resume = G_NOT_CONVERTED;
    e->own.bytes[ESCORT_47] = a->spawn_byte;
    switch (a->spawn_byte) {
    case 0:
        e->own.bytes[G_STATE] = 0;
        e->own.bytes[G_GONE] = 0;
        return ActorEndPlaced(a);
    case 1:
    case 2:
    case 3:
        e->own.bytes[G_STATE] = 0;
        return ActorEndPlaced(a);
    default: {
        static int told;

        /* past its table of four ways: the original jumps where the next words point */
        if (!told++)
            fprintf(stderr, "Kind92Start: $07 %u (past its table) not converted\n", a->spawn_byte);
        a->resume = ACTOR_RESUME_OWN;
        return 1;
    }
    }
}

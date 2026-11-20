/*
 * kind91.c: actor kind $91, a marker that steers the level's escort actor
 * (actors[1], a guard: guard.h): by its $07 it sets the escort's state once
 * one has passed the other, or at once, or sets or clears one of the
 * escort's flags, and ends. Its wait routine is Kind91FreeOff (kinds E's:
 * kinds $91 and $92 share it).
 */
#include <stdio.h>

#include "actor/guard.h"
#include "actor/kinds_d.h"
#include "actor/kinds_e.h"

enum {
    BEFORE = ACTOR_RESUME_OWN,  /* variant 0: each frame, until the escort is at or past it */
    AFTER                       /* variant 1: each frame, until it is at or past the escort */
};

/*
 * @routine A5:F534 Kind91Start
 * WHAT IT DOES: Actor kind $91, a marker for the level's escort actor. By
 * $07: 0, once the escort is idle (its +$45 zero) and this marker is at or
 * past the escort's x, the escort's state 4; 1, once the escort is idle and
 * at or past the marker's x, state 5 (each frame on); 2, state 6 if the
 * escort is idle; 3, if the escort is busy: idle again, state 1 and the
 * escort moved to the marker's x and y; 4, the escort's +$46 set; 5, it
 * cleared. Variants 2 to 5 end at once.
 */
int Kind91Start(Actor *a)
{
    Actor *e = &g.actors[1];

    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x80;
        switch (a->spawn_byte) {
        case 0:
            a->resume = BEFORE;
            return 1;
        case 1:
            a->resume = AFTER;
            return 1;
        case 2:
            if (!e->own.bytes[G_GONE])
                e->own.bytes[G_STATE] = 6;
            return ActorEndPlaced(a);
        case 3:
            if (e->own.bytes[G_GONE]) {
                e->own.bytes[G_GONE] = 0;
                e->own.bytes[G_STATE] = 1;
                e->x = with_pixels(e->x, pixels(a->x));
                e->y = with_pixels(e->y, pixels(a->y));
            }
            return ActorEndPlaced(a);
        case 4:
            e->own.bytes[G_BACK_OFF] = 1;
            return ActorEndPlaced(a);
        case 5:
            e->own.bytes[G_BACK_OFF] = 0;
            return ActorEndPlaced(a);
        default: {
            static int told;

            /* past its table of six variants: the original jumps where the
               next words point */
            if (!told++)
                fprintf(stderr, "Kind91Start: $07 %u (past its table) not converted\n", a->spawn_byte);
            a->resume = ACTOR_RESUME_OWN + 2;
            return 1;
        }
        }
    case BEFORE:
        if (!e->own.bytes[G_GONE] && pixels(a->x) >= pixels(e->x))
            e->own.bytes[G_STATE] = 4;
        return 1;
    case AFTER:
        if (!e->own.bytes[G_GONE] && pixels(e->x) >= pixels(a->x))
            e->own.bytes[G_STATE] = 5;
        return 1;
    default:
        return 1;
    }
}

/*
 * kind34.c: actor kind $34, a short-lived figure that animates in place
 * or flies off, by its $07, then ends. Its wait routine is Kind34Wait
 * (kinds C's kind_waits_rest.c).
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_d.h"

enum {
    IN_PLACE = ACTOR_RESUME_OWN,    /* part 0, each frame */
    FLY_OFF,                        /* part 1, each frame */
    STAY                            /* a part past its table (reported) */
};

/*
 * @routine 83:BB97 Kind34Start
 * WHAT IT DOES: Actor kind $34, a short-lived figure: counts itself in its
 * kind's count of live actors, starts animation 6 in palette 4, and by its
 * $07 plays one of three parts for 21 frames before it ends: part 0
 * animates in place, part 1 flies off upward, part 2 ends at once.
 * Also 83:BBBF Kind34Part0, 83:BBCD Kind34Part1 (its speed across cleared,
 * up by the speed table's entry 9, gravity 2) and 83:BBED Kind34Part2.
 * Its second entry (state $80-$BF) is 83:96CA ActorEndPlaced: it ends.
 */
int Kind34Start(Actor *a)
{
    uint8_t *frames = &a->own.bytes[OWN_40];

    switch (a->resume) {
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case ACTOR_START:
        g.kind_count[0x34]++;
        a->flags = 0x10;
        a->look = (uint16_t)((a->look & 0x00FF) | 0x0400);
        AnimStart(a, 6);
        *frames = 0x15;
        switch (a->spawn_byte) {
        case 0:
            a->resume = IN_PLACE;
            return 1;
        case 1:
            ActorSpeedXClear(a);
            ActorSpeedYSet(a, 0x09);
            a->accel_y = 0x02;
            a->resume = FLY_OFF;
            return 1;
        case 2:
            return ActorEndPlaced(a);
        default: {
            static int told;

            /* past the table of three parts: the original jumps where the
               next words point ($0000 for part 3) */
            if (!told++)
                fprintf(stderr, "Kind34Start: part %u (past its table) not converted\n", a->spawn_byte);
            a->resume = STAY;
            return 1;
        }
        }
    case IN_PLACE:
        AnimStep(a);
        return --*frames ? 1 : ActorEndPlaced(a);
    case FLY_OFF:
        AnimStep(a);
        ActorFly(a);
        return --*frames ? 1 : ActorEndPlaced(a);
    default:                            /* STAY */
        return 1;
    }
}

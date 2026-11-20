/*
 * kind49.c: actor kind $49, which carries the player along as it moves:
 * its start and its wait routine.
 */
#include <stdio.h>

#include "actor/actor_own.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_4C];
    uint8_t carry_box;          /* +$4C: the box ActorTestCarries tests the player with */
} K49Own;

_Static_assert(sizeof(K49Own) <= sizeof(((Actor *)0)->own.bytes), "a view fits the own bytes");

static inline K49Own *k49(Actor *a) { return (K49Own *)a->own.bytes; }

enum { CARRY = ACTOR_RESUME_OWN };

/*
 * @routine 84:804D Kind49Carry
 * WHAT IT DOES: Actor kind $49 (through 83:923C Kind49Enter): its wait
 * routine on (flags $90); then each frame it carries the player along by
 * however far it moved (ActorCarryPlayer); its own moves are its wait
 * routine's.
 */
int Kind49Carry(Actor *a)
{
    if (a->resume == CARRY) {
        ActorCarryPlayer(a);
        return 1;
    }
    a->flags |= 0x90;
    a->resume = CARRY;
    return 1;
}

/*
 * @routine 84:805B Kind49Wait
 * Also 83:9240 ActorKind49Wait, the wait table's stub that calls it.
 * WHAT IT DOES: Kind $49's wait routine: whether it carries the player
 * this frame (ActorTestCarries, test 0, with box $34, $38 or $3C by its
 * +$07 0, 1 or 2, kept at +$4C), then it ends when out of range by box
 * $18 (ActorFreeOutOfRange). The original jumps through a table of three
 * parts by +$07; a +$07 past them jumps where the next code's bytes point,
 * which no placement gives: reported once, and nothing done.
 */
int Kind49Wait(Actor *a)
{
    static const uint8_t box[3] = { 0x34, 0x38, 0x3C };

    if (a->spawn_byte > 2) {
        static int told;

        if (!told++)
            fprintf(stderr, "Kind49Wait: +$07 %u (past its three parts) not converted\n", a->spawn_byte);
        return ACTOR_GOES_ON;
    }
    k49(a)->carry_box = box[a->spawn_byte];
    ActorTestCarries(a, 0);
    return ActorFreeOutOfRange(a, 0x18) ? ACTOR_ENDED : ACTOR_GOES_ON;
}


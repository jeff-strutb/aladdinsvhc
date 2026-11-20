/*
 * actor_steps.c: small steps of an actor's code.
 */
#include "actor/actor_a.h"

/*
 * @routine 83:CF10 ActorFlagOnce
 * WHAT IT DOES: Each of eight such actors (by the low 3 bits of its $07)
 * has a flag in g.level_flags: when its flag is set already, it is set
 * again; else the case by bits 3-4 of $07 runs (flag_drops.c), which
 * drops its children and sets the flag. Its flag's mask (bit n, the table
 * at $80:D316) is kept in its own $41.
 */
void ActorFlagOnce(Actor *a)
{
    uint8_t mask = (uint8_t)(1u << (a->spawn_byte & 0x07));

    a->own.bytes[OWN_41] = mask;
    if (g.level_flags & mask)
        g.level_flags |= mask;
    else
        flag_drop_case(a);
}

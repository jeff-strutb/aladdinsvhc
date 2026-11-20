/*
 * anim_pick.c: an actor's animation picked by what it is doing.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"

/*
 * @routine A5:EA15 ActorAnimChange
 * WHAT IT DOES: Starts animation `anim` unless the actor already plays it
 * (its +$2D, the animation's offset, is twice it). As the original doubles
 * it in a byte, animation `anim` is taken as its low 7 bits.
 */
void ActorAnimChange(Actor *a, uint8_t anim)
{
    uint8_t twice = (uint8_t)(anim << 1);

    if (twice == a->anim)
        return;
    AnimStart(a, (uint8_t)(twice >> 1));
}

/*
 * @routine 84:8A48 ActorAnimBy40
 * WHAT IT DOES: Starts the animation the actor's own +$40 picks from a byte
 * table.
 */
void ActorAnimBy40(Actor *a)
{
    AnimStart(a, anim_by_40(a->own.bytes[OWN_40]));
}

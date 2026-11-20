/*
 * anim_pick.c: an actor's animation picked by what it is doing.
 */
#include "actor/actor_own.h"

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

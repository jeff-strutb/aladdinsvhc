/*
 * actor_fields.c: small settings of an actor's record: its state, and
 * bytes its kind's code reads.
 */
#include "actor/actor_own.h"

/*
 * @routine 83:824B ActorStateSetCC
 * WHAT IT DOES: Puts the actor into state $CC: its kind's third entry at
 * its next turn.
 */
void ActorStateSetCC(Actor *a)
{
    a->state = 0xCC;
}

/*
 * @routine 83:8267 ActorStateSet8C
 * WHAT IT DOES: Puts the actor into state $8C (its kind's second entry at
 * its next turn), with bit 7 of its byte at $08 set and of its flags
 * cleared.
 */
void ActorStateSet8C(Actor *a)
{
    a->anim_flags |= 0x80;
    a->flags &= 0x7F;
    a->state = 0x8C;
}

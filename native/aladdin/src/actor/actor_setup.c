/*
 * actor_setup.c: an actor started from tables by its own index.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"

/*
 * @routine 84:8D8F ActorStartBy40
 * WHAT IT DOES: Starts the actor from its own index +$40: anim_flags 2 and
 * flags $A0, its word at +$42 from a word table less $0100, its byte at
 * +$41 from a byte table, and its bytes +$46-$49, +$4C and +$18 cleared.
 */
void ActorStartBy40(Actor *a)
{
    uint8_t i = a->own.bytes[OWN_40];
    uint16_t word = (uint16_t)(start_by_40_word(i) - 0x0100);

    a->anim_flags = 0x02;
    a->flags = 0xA0;
    a->own.bytes[OWN_42] = (uint8_t)word;
    a->own.bytes[OWN_43] = (uint8_t)(word >> 8);
    a->own.bytes[OWN_41] = start_by_40_byte(i);
    a->own.bytes[OWN_46] = 0;
    a->own.bytes[OWN_47] = 0;
    a->own.bytes[OWN_48] = 0;
    a->own.bytes[OWN_49] = 0;
    a->own.bytes[OWN_4C] = 0;
    a->own.bytes[OWN_18] = 0;
}

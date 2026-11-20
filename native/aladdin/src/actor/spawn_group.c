/*
 * spawn_group.c: an actor spawning several children at once.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"

#define RING_KIND 0x34

/*
 * @routine 83:D0A3 ActorSpawnRing
 * WHAT IT DOES: Spawns up to five children of kind $34 in a ring of
 * points around the actor (each point the last moved by the ring table's
 * entry), each with $07 = 1, at spawn point 0; stops at the first that
 * finds no record free. Its own place and $07-$08 are as before after;
 * its $40 is left the children not spawned (0 when all five were), its
 * $43 the last entry's offset, its spawn point 0.
 */
void ActorSpawnRing(Actor *a)
{
    uint32_t x = a->x, y = a->y;
    uint8_t byte07 = a->spawn_byte, byte08 = a->anim_flags;
    uint8_t at = 0;

    a->own.bytes[OWN_40] = 5;
    for (;;) {
        RingStep r = ring_step(at);

        a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) + r.dx));
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + r.dy));
        a->own.bytes[OWN_43] = at;
        a->spawn_byte = 1;
        a->spawn_at = 0;
        if (!ActorCreateChild8(a, RING_KIND))
            break;
        at = (uint8_t)(a->own.bytes[OWN_43] + 4);
        if (--a->own.bytes[OWN_40] == 0)
            break;
    }
    a->spawn_byte = byte07;
    a->anim_flags = byte08;
    a->x = with_pixels(a->x, pixels(x));
    a->y = with_pixels(a->y, pixels(y));
}

/*
 * @routine 83:D332 ActorSpawnPair
 * WHAT IT DOES: Spawns two children that look like the actor, of its own
 * kind: at spawn point $46, then (if the first found a record) at $48
 * with bit 2 of $07 set; the children take its $07 + 2 as theirs. The
 * actor's $07 keeps only bit 0 after.
 */
void ActorSpawnPair(Actor *a)
{
    a->spawn_byte = (uint8_t)(a->spawn_byte + 2);
    a->spawn_at = 0x46;
    if (ActorSpawnSameLook(a, a->kind)) {
        a->spawn_byte |= 0x04;
        a->spawn_at = 0x48;
        (void)ActorSpawnSameLook(a, a->kind);
    }
    a->spawn_byte &= 0x01;
}

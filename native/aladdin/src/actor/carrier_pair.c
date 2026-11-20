/*
 * carrier_pair.c: an actor that sends in a pair: a carrier (kind $38) and
 * the thing it holds (kind $53), each knowing the other.
 */
#include "actor/actor_own.h"

#define CARRIER_KIND 0x38
#define THING_KIND 0x53
#define PAIR_AT 0x44                    /* the spawn point both are placed at */
#define OWN_SENDER OWN_4C               /* the record of the actor that sent them (index in g.actors) */
#define OWN_OTHER OWN_4E                /* the record of the other of the pair (index in g.actors) */

static uint8_t index_of(const Actor *a) { return (uint8_t)(a - g.actors); }

/* the new record starts at its next turn, facing as the sender, knowing its sender */
static void pair_start(const Actor *sender, Actor *a)
{
    a->state = ACTOR_STATE_NEW;
    a->facing = sender->facing;
    a->flip = sender->facing;
    a->own.bytes[OWN_SENDER] = index_of(sender);
    a->own.bytes[OWN_SENDER + 1] = 0;
}

/*
 * @routine 84:A056 CarrierPairSend
 * WHAT IT DOES: Sends in a carrier (kind $38) 40 pixels above the point
 * actors are kept in range of, and a thing it holds (kind $53) at the
 * carrier's spawn point $44 less 4 across and 12 down, both new and facing
 * as the sender, in the two records its own +$4A names (the places
 * ActorFreeListTake gave in g.free_pool: it counts on 1 as the carrier is
 * made); each the other's record in its +$4E (an index in g.actors) and
 * the sender's record in its +$4C, the carrier's in the sender's +$4E; the
 * carrier's $07 the sender's plus one, its +$45, +$46 cleared; the sender's
 * +$41 and +$4B counted up and +$40 = $3C frames before the next.
 */
void CarrierPairSend(Actor *sender)
{
    uint16_t x = pixels(sender->x), y = pixels(sender->y);
    Actor *carrier, *thing;

    sender->x = with_pixels(sender->x, g.range_x);
    sender->y = with_pixels(sender->y, (uint16_t)(g.range_y - 0x28));
    carrier = ActorCreateIn(g.free_pool[sender->own.bytes[OWN_4A]], CARRIER_KIND);
    sender->own.bytes[OWN_4A]++;
    if (carrier) {
        sender->own.bytes[OWN_4E] = index_of(carrier);
        sender->own.bytes[OWN_4F] = 0;
        carrier->spawn_byte = (uint8_t)(sender->spawn_byte + 1);
        sender->spawn_at = PAIR_AT;
        ActorSpawnPlace(sender, carrier);
        pair_start(sender, carrier);
        carrier->own.bytes[OWN_45] = 0;
        carrier->own.bytes[OWN_46] = 0;
        sender->own.bytes[OWN_41]++;
        sender->own.bytes[OWN_40] = 0x3C;
    }
    thing = ActorCreateIn(g.free_pool[sender->own.bytes[OWN_4A]], THING_KIND);
    if (thing) {
        sender->spawn_at = PAIR_AT;
        ActorSpawnPlace(sender, thing);
        thing->x = with_pixels(thing->x, (uint16_t)(pixels(thing->x) - 4));
        thing->y = with_pixels(thing->y, (uint16_t)(pixels(thing->y) + 12));
        pair_start(sender, thing);
        if (carrier) {
            thing->own.bytes[OWN_OTHER] = index_of(carrier);
            thing->own.bytes[OWN_OTHER + 1] = 0;
            carrier->own.bytes[OWN_OTHER] = index_of(thing);
            carrier->own.bytes[OWN_OTHER + 1] = 0;
        }
    }
    sender->x = with_pixels(sender->x, x);
    sender->y = with_pixels(sender->y, y);
    sender->own.bytes[OWN_4B]++;
}

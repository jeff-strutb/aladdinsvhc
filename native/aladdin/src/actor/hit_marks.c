/*
 * hit_marks.c: the small actors of the second pool that mark a hit (kind
 * $11) or a splash (kind $13) at a record's place.
 */
#include "actor/actor_a.h"

/* a mark of `kind` at `at`'s place, when the second pool has a record */
static void mark_at(const Actor *at, uint8_t kind)
{
    Actor *c = ActorAllocB();

    if (!c)
        return;
    c->kind = kind;
    c->state = ACTOR_STATE_NEW;
    c->spawn_byte = 0;
    c->anim_flags = 0x80;
    c->x = with_pixels(c->x, pixels(at->x));
    c->y = with_pixels(c->y, pixels(at->y));
}

/*
 * @routine 83:827D Spawn11AtY
 * WHAT IT DOES: Starts an actor of kind $11 (a hit's mark) at the
 * record's place, when the second pool has a free record.
 */
void Spawn11AtY(const Actor *at)
{
    mark_at(at, 0x11);
}

/*
 * @routine 83:82B5 Spawn13AtY
 * WHAT IT DOES: Starts an actor of kind $13 (a splash) at the record's
 * place, when the second pool has a free record.
 * Also 83:82B1 Spawn13AtYLong (shot_hit.c), for callers in other banks.
 */
void Spawn13AtY(const Actor *at)
{
    mark_at(at, 0x13);
}

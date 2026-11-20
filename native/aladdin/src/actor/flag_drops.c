/*
 * flag_drops.c: the cases of ActorFlagOnce (actor_steps.c): an actor that
 * drops children (kind $30) once, its flag in g.level_flags marking that
 * it has. Its own $41 holds its flag's mask meanwhile.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "system/system.h"

#define DROP_KIND 0x30

/* ActorFlagOnce's mark (83:CF61): the actor's flag set, it drops no more */
static void flag_mark(const Actor *a)
{
    g.level_flags |= a->own.bytes[OWN_41];
}

/*
 * @routine 83:CF53 FlagDropChild
 * WHAT IT DOES: Drops a child of kind $30 and sort `sort` (its $07) at the
 * actor's spawn point $3C; when there was a record for it, marks the
 * actor's flag so it drops no more.
 */
static void drop_child(Actor *a, uint8_t sort)
{
    a->spawn_byte = sort;
    a->spawn_at = 0x3C;
    if (ActorCreateChild8(a, DROP_KIND))
        flag_mark(a);
}

/*
 * @routine 83:CF31 FlagDropMaybe
 * WHAT IT DOES: ActorFlagOnce's case 0: by chance drops a child of one of
 * the sorts of the first table, and marks the actor's flag when it does,
 * or when the chance gives none.
 */
static void drop_maybe(Actor *a)
{
    uint8_t sort;

    RandomNext();
    sort = drop_sort_maybe((uint8_t)g.random);
    if (sort)
        drop_child(a, sort);
    else
        flag_mark(a);
}

/*
 * @routine 83:CF42 FlagDropAlways
 * WHAT IT DOES: ActorFlagOnce's case 1: drops a child of a sort the second
 * table picks by chance, and marks the actor's flag.
 */
static void drop_always(Actor *a)
{
    RandomNext();
    drop_child(a, drop_sort_always((uint8_t)g.random));
}

/*
 * @routine 83:CF67 FlagDropThree
 * WHAT IT DOES: ActorFlagOnce's case 3: drops three children (kind $30,
 * sort 7) at the actor's spawn point $92, numbered 2, 1 and 0 in their
 * $21, and marks the actor's flag; when the records run out it stops
 * without the mark. Its own $40 counts them.
 */
static void drop_three(Actor *a)
{
    a->own.bytes[OWN_40] = 2;
    a->spawn_byte = 0x07;
    a->spawn_at = 0x92;
    do {
        Actor *c = ActorSpawn(a, DROP_KIND);

        if (!c)
            return;
        c->spawn_at = a->own.bytes[OWN_40];
    } while ((int8_t)--a->own.bytes[OWN_40] >= 0);
    flag_mark(a);
}

/* ActorFlagOnce's case by bits 3-4 of its $07 (the table at $83:CF29). */
void flag_drop_case(Actor *a)
{
    switch ((a->spawn_byte & 0x18) >> 3) {
    case 0:
        drop_maybe(a);
        break;
    case 1:
        drop_always(a);
        break;
    case 2:
        /* 83:CF51 FlagDropFour: a child of sort 4, and the mark */
        drop_child(a, 0x04);
        break;
    default:
        drop_three(a);
        break;
    }
}

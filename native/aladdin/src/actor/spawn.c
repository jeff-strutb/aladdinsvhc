/*
 * spawn.c: new actors started at a place: one a caller names, or a
 * child at its spawner's spawn point.
 */
#include "actor/actor_own.h"
#include "data/actor_tables.h"

/*
 * @routine 80:A4DC ActorSpawnAt
 * WHAT IT DOES: Starts a new actor of `kind` at (x, y) pixels with `byte07`
 * at its +$07, at its next turn; NULL when no record is free or its kind's
 * set-up fails.
 * Also 80:A4D8 ActorSpawnAtLong, the same for callers in other banks.
 */
Actor *ActorSpawnAt(uint8_t kind, uint8_t byte07, uint16_t x, uint16_t y)
{
    Actor *a = ActorCreate(kind);

    if (!a)
        return NULL;
    a->state = ACTOR_STATE_NEW;
    a->spawn_byte = byte07;
    a->x = with_pixels(a->x, x);
    a->y = with_pixels(a->y, y);
    return a;
}

/*
 * @routine 81:AA81 ActorSpawnPlace
 * WHAT IT DOES: Places a child at its spawner's spawn point: the spawner's
 * spawn index picks an offset across and one down; across is added to the
 * spawner's x, or taken away when bit 0 of its flip is set; down is added to
 * its y. The fractions stay the child's.
 */
void ActorSpawnPlace(Actor *spawner, Actor *child)
{
    uint16_t across = spawn_offset_x(spawner->spawn_at);
    uint16_t px = pixels(spawner->x);

    px = (uint16_t)((spawner->flip & 1) ? px - across : px + across);
    child->x = with_pixels(child->x, px);
    child->y = with_pixels(child->y, (uint16_t)(pixels(spawner->y) + spawn_offset_y(spawner->spawn_at)));
}

/*
 * @routine 81:AA11 ActorSpawnInit
 * WHAT IT DOES: Starts a child as a new actor at its spawner's spawn
 * point, facing the spawner's way: its facing and flip both the spawner's
 * flip, no placement, the spawner's +$07.
 */
void ActorSpawnInit(Actor *spawner, Actor *child)
{
    child->state = ACTOR_STATE_NEW;
    ActorSpawnPlace(spawner, child);
    child->facing = spawner->flip;
    child->flip = spawner->flip;
    child->placement = 0xFF;
    child->spawn_byte = spawner->spawn_byte;
}

/*
 * @routine 81:A9B1 ActorCreateChild8
 * WHAT IT DOES: Creates an actor of `kind` as a child of the spawner,
 * placed at its spawn point (ActorCreate, then ActorSpawnInit); NULL when it
 * could not be created.
 */
Actor *ActorCreateChild8(Actor *spawner, uint8_t kind)
{
    Actor *child = ActorCreate(kind);

    if (child)
        ActorSpawnInit(spawner, child);
    return child;
}

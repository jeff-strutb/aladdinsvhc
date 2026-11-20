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

/*
 * @routine 81:A9FA ActorTileSlotShare
 * WHAT IT DOES: Gives a child its spawner's palette slot, if it has one,
 * and counts it among the slot's users.
 */
void ActorTileSlotShare(Actor *spawner, Actor *child)
{
    if (!spawner->tile_slot)
        return;
    child->tile_slot = spawner->tile_slot;
    g.palette_slot_users[spawner->tile_slot - 1]++;
}

/*
 * @routine 81:A9BF ActorSpawn
 * WHAT IT DOES: Spawns a child of `kind` at the spawner's spawn point,
 * sharing its palette slot, if a pool record is free; NULL otherwise. It is
 * not counted among its kind and runs no set-up.
 * Also 81:AA0E ActorSpawnTyped, its part once the record is taken.
 */
Actor *ActorSpawn(Actor *spawner, uint8_t kind)
{
    Actor *child = ActorAlloc();

    if (!child)
        return NULL;
    child->kind = kind;
    ActorSpawnInit(spawner, child);
    ActorTileSlotShare(spawner, child);
    return child;
}

/*
 * @routine 81:A9CE ActorSpawnSameLook
 * WHAT IT DOES: ActorSpawn, with the spawner's look copied to the child
 * before it takes the slot.
 */
Actor *ActorSpawnSameLook(Actor *spawner, uint8_t kind)
{
    Actor *child = ActorAlloc();

    if (!child)
        return NULL;
    child->kind = kind;
    ActorSpawnInit(spawner, child);
    child->look = spawner->look;
    ActorTileSlotShare(spawner, child);
    return child;
}

/*
 * @routine 81:AA2D ActorSpawnFacing
 * WHAT IT DOES: Spawns a child of `kind` at the spawner's spawn point,
 * new, facing `facing` (its flip the same), with `byte07` at its +$07, if a
 * pool record is free; NULL otherwise. Its placement and palette slot are
 * left as the record had them.
 */
Actor *ActorSpawnFacing(Actor *spawner, uint8_t kind, uint8_t facing, uint8_t byte07)
{
    Actor *child = ActorAlloc();

    if (!child)
        return NULL;
    child->kind = kind;
    child->state = ACTOR_STATE_NEW;
    ActorSpawnPlace(spawner, child);
    child->facing = facing;
    child->flip = facing;
    child->spawn_byte = byte07;
    return child;
}

/*
 * @routine 81:A9A6 ActorCreateChild
 * WHAT IT DOES: Creates an actor of `kind` as a child of the spawner,
 * placed at its spawn point (ActorCreate, then ActorSpawnInit); NULL when it
 * could not be created. (81:A9B1 ActorCreateChild8 is the same with 8-bit
 * registers after.)
 */
Actor *ActorCreateChild(Actor *spawner, uint8_t kind)
{
    return ActorCreateChild8(spawner, kind);
}

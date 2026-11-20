/*
 * map_probe.c: the map's block at a point ahead of an actor.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "map/map.h"

/*
 * @routine 83:B5F6 ActorMapAhead5Down12
 * WHAT IT DOES: Gives the kind of the map's tile 5 pixels ahead of the
 * actor (along its facing, the table at $80:D176) and 12 below it
 * (TileAtProbeLong).
 */
uint8_t ActorMapAhead5Down12(Actor *a)
{
    MapPoint p;

    p.x = (uint16_t)(pixels(a->x) + ahead_5(a->facing));
    p.y = (uint16_t)(pixels(a->y) + 12);
    return TileAt(a, &p);
}

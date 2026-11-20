/*
 * wanted_d.c: stand-ins for the routines of other sessions that kinds
 * session D's kinds call, asked for under the wanted lines of their
 * owners' headers, until the owners' code lands. Each is weak, so the
 * owner's definition replaces it at link time; each prints its name once
 * and does nothing harmful. Each goes when its owner lands it.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "map/map.h"

#define TELL_ONCE(name) do { static int told; if (!told++) fprintf(stderr, "%s not converted (actor/wanted_d.c stand-in)\n", name); } while (0)

/* actors A's: its place not kept */
__attribute__((weak)) void ActorKeepSpot(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorKeepSpot (81:9D56)");
}

/* map/'s, for kind $08: as ActorOutOfWall, nothing found */
__attribute__((weak)) int ActorOutOfWallAt(Actor *a, uint8_t probe, MapPoint *p)
{
    TELL_ONCE("ActorOutOfWallAt (81:B60C)");
    p->behaviour = 0;
    return ActorOutOfWall(a, probe);
}

/* kinds B's: never ended */
__attribute__((weak)) int ActorEndIfAhead(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorEndIfAhead (83:9749)");
    return ACTOR_GOES_ON;
}

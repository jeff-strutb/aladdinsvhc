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

/* actors A's, for kind $08: no speed from the tables, no animation change */
__attribute__((weak)) void ActorSpeedXFromA5(Actor *a, uint8_t at)
{
    (void)a;
    (void)at;
    TELL_ONCE("ActorSpeedXFromA5 (A5:EA93)");
}

__attribute__((weak)) void ActorSpeedYFromA5(Actor *a, uint8_t at)
{
    (void)a;
    (void)at;
    TELL_ONCE("ActorSpeedYFromA5 (A5:EAA3)");
}

__attribute__((weak)) void ActorAnimChange(Actor *a, uint8_t anim)
{
    (void)a;
    (void)anim;
    TELL_ONCE("ActorAnimChange (A5:EA15)");
}

__attribute__((weak)) void ActorAnimByTile(Actor *a, uint8_t behaviour)
{
    (void)a;
    (void)behaviour;
    TELL_ONCE("ActorAnimByTile (A5:E9D2)");
}

__attribute__((weak)) void ActorAnimRiseFall(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorAnimRiseFall (A5:EA20)");
}

/* map/'s, for kind $08: as ActorOutOfWall, nothing found */
__attribute__((weak)) int ActorOutOfWallAt(Actor *a, uint8_t probe, MapPoint *p)
{
    TELL_ONCE("ActorOutOfWallAt (81:B60C)");
    p->behaviour = 0;
    return ActorOutOfWall(a, probe);
}

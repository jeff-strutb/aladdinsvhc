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

/* video's: no circle drawn (the wipe stays shut) */
__attribute__((weak)) void CircleWipeStep(int16_t screen_x, int16_t screen_y, uint16_t radius)
{
    (void)screen_x;
    (void)screen_y;
    (void)radius;
    TELL_ONCE("CircleWipeStep (81:...)");
}

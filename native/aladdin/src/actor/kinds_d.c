/*
 * kinds_d.c: the kinds n5-kinds-d converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-kinds-d's alone.
 *
 * Below the table: stand-ins for the routines of other sessions these kinds
 * call (asked for under the wanted lines of actor/actor.h and map/map.h),
 * until their owners' code lands. Each is weak, so the owner's definition
 * replaces it at link time, and prints its name once.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "map/map.h"

const KindCode kinds_d[] = {
    { 0x30, Kind30Start, ActorEndPlaced, NULL, Kind30Wait },
    { 0, NULL, NULL, NULL, NULL }
};

/* ---- stand-ins ---- */

#define TELL_ONCE(name) do { static int told; if (!told++) fprintf(stderr, "%s not converted (actor/kinds_d.c stand-in)\n", name); } while (0)

/* actors A's: an actor that is never hit and never out of range */
__attribute__((weak)) bool ActorHitsTestBox(const Actor *a)
{
    (void)a;
    TELL_ONCE("ActorHitsTestBox (83:8312)");
    return false;
}

__attribute__((weak)) int ActorEndOutOfRange(Actor *a, uint8_t at)
{
    (void)a;
    (void)at;
    TELL_ONCE("ActorEndOutOfRange (83:96BE)");
    return ACTOR_GOES_ON;
}

__attribute__((weak)) Actor *ActorTakesShot(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorTakesShot (83:81E8)");
    return NULL;
}

/* map/'s: no wall, and on the ground at once */
__attribute__((weak)) bool ActorOutOfWallProbe4(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorOutOfWallProbe4 (81:B5AD)");
    return false;
}

__attribute__((weak)) bool ActorOutOfGround(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorOutOfGround (81:B4F3)");
    return true;
}

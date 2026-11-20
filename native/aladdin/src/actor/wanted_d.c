/*
 * wanted_d.c: stand-ins for the routines of other sessions that kinds
 * session D's kinds call, asked for under the wanted lines of their
 * owners' headers, until the owners' code lands. Each is weak, so the
 * owner's definition replaces it at link time; each prints its name once
 * and returns a harmless value. Each goes when its owner lands it.
 */
#include <stdio.h>

#include "actor/actor_own.h"

#define TELL_ONCE(name) do { static int told; if (!told++) fprintf(stderr, "%s not converted (actor/wanted_d.c stand-in)\n", name); } while (0)

/* actors A's: no shot reaches it */
__attribute__((weak)) Actor *ActorTakesShot(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorTakesShot (83:81E8)");
    return NULL;
}

/* actors A's: no children */
__attribute__((weak)) void ActorSpawnPair(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorSpawnPair (83:D332)");
}

/* actors A's: he never lands on it */
__attribute__((weak)) bool PlayerLandsOnActor(Actor *a)
{
    (void)a;
    TELL_ONCE("PlayerLandsOnActor (83:848D)");
    return false;
}

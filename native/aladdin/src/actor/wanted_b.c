/*
 * wanted_b.c: kinds B's weak stand-ins for the routines of other areas its
 * kinds call, until their owners land them (each declared under the
 * owner's wanted lines). Each prints its name once and gives a
 * harmless answer; the owner's definition replaces it at link time, and
 * then the stand-in goes.
 */
#include "actor/actor_own.h"

/* n5-actors-a (actor_steps.c, flag_drops.c) */
__attribute__((weak)) void ActorFlagOnce(Actor *a)
{
    (void)a;
    unconverted("ActorFlagOnce (83:CF10)");
}

/* n5-actors-a (shot_hit.c): no shot reached it */
__attribute__((weak)) Actor *ActorTakesShot(Actor *a)
{
    (void)a;
    unconverted("ActorTakesShot (83:81E8)");
    return NULL;
}

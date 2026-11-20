/*
 * wanted_b.c: kinds B's weak stand-ins for the routines of other areas its
 * kinds call, until their owners land them (each declared under the
 * owner's wanted lines). Each prints its name once and gives a
 * harmless answer; the owner's definition replaces it at link time, and
 * then the stand-in goes.
 */
#include "actor/actor_own.h"

/* n5-actors-a (motion.c) */
__attribute__((weak)) void ActorFallAgainst(Actor *a)
{
    (void)a;
    unconverted("ActorFallAgainst (81:A520)");
}

/* n5-actors-a (player_grab.c) */
__attribute__((weak)) void ActorHoldPlayer(Actor *a)
{
    (void)a;
    unconverted("ActorHoldPlayer (83:BAF3)");
}

/* n5-actors-a (actor_steps.c) */
__attribute__((weak)) void ActorYByFrame(Actor *a)
{
    (void)a;
    unconverted("ActorYByFrame (83:F031)");
}


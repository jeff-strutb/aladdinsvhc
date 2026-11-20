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

/* n5-actors-a (player_carry.c) */
__attribute__((weak)) void ActorTestCarries(Actor *a, uint8_t test)
{
    (void)a;
    (void)test;
    unconverted("ActorTestCarries (83:FB89)");
}

/* n5-actors-a (motion.c) */
__attribute__((weak)) void ActorFallAgainst(Actor *a)
{
    (void)a;
    unconverted("ActorFallAgainst (81:A520)");
}

/* n5-actors-a (motion.c) */
__attribute__((weak)) void ActorMotionPresetY(Actor *a, uint8_t preset)
{
    (void)a;
    (void)preset;
    unconverted("ActorMotionPresetY (81:A3EC)");
}

/* n5-actors-a (player_grab.c) */
__attribute__((weak)) void ActorHoldPlayer(Actor *a)
{
    (void)a;
    unconverted("ActorHoldPlayer (83:BAF3)");
}

/* n5-actors-a (actor_y_steps.c) */
__attribute__((weak)) void ActorYByFrame47(Actor *a)
{
    (void)a;
    unconverted("ActorYByFrame47 (83:BDE9)");
}

/* n5-actors-a (actor_steps.c) */
__attribute__((weak)) void ActorYByFrame(Actor *a)
{
    (void)a;
    unconverted("ActorYByFrame (83:F031)");
}

/* n5-actors-a (actor_84_misc.c): not out of range */
__attribute__((weak)) int ActorAnimFreeFar(Actor *a)
{
    (void)a;
    unconverted("ActorAnimFreeFar (84:832B)");
    return ACTOR_GOES_ON;
}

/* native-video (video.h's ActorPictureSend) */
__attribute__((weak)) void ActorPictureSend(Actor *a)
{
    (void)a;
    unconverted("ActorPictureSend (81:949E)");
}

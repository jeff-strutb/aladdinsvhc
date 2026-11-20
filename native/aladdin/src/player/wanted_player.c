/*
 * wanted_player.c: stand-ins for the routines of other areas the player's
 * code calls, until their owners land them (each asked for under
 * / * wanted * / in its owner's header). Each is weak: the owner's definition
 * replaces it at link. Each prints its name once and does nothing.
 */
#include <stdio.h>

#include "player/player_own.h"
#include "level/level.h"

#define TELL_ONCE(name) do { static int told; if (!told++) fprintf(stderr, "%s not converted (player/wanted_player.c stand-in)\n", name); } while (0)

/* actor/ (n5-actors-a) */
__attribute__((weak)) void ActorMotionPresetY(Actor *a, uint8_t preset)
{
    (void)a;
    (void)preset;
    TELL_ONCE("ActorMotionPresetY (81:A3EC)");
}

__attribute__((weak)) Actor *ActorSpawnShared(Actor *spawner, uint8_t kind)
{
    (void)spawner;
    (void)kind;
    TELL_ONCE("ActorSpawnShared (81:A9E9)");
    return NULL;
}

/* level/ (n5-levels) */
__attribute__((weak)) void Level035CSet(void)
{
    TELL_ONCE("Level035CSet (81:878B)");
}

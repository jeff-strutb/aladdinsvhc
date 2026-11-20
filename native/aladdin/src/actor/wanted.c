/*
 * wanted.c: stand-ins for the routines of other areas the actors call,
 * until their owners' code lands. Each is weak: the owner's definition
 * replaces it at link time. Each prints its name once and does nothing
 * (a slot claim fails, so no kind with graphics is created yet). Both are
 * asked for under video/video.h's wanted lines.
 */
#include <stdio.h>

#include "actor/actor_own.h"

#define TELL_ONCE(name) do { static int told; if (!told++) fprintf(stderr, "%s not converted (actor/wanted.c stand-in)\n", name); } while (0)

__attribute__((weak)) int TileSlotClaim(uint8_t kind, Actor *a)
{
    (void)kind;
    (void)a;
    TELL_ONCE("TileSlotClaim (82:ABCE)");
    return -1;
}

__attribute__((weak)) void TileSlotFill(uint8_t kind)
{
    (void)kind;
    TELL_ONCE("TileSlotFill (82:AB77)");
}

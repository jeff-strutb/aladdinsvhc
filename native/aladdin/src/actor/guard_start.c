/*
 * guard_start.c: the start of actor kind $08 (the guard, and the scripted
 * figures that share its kind): its record set up at the player's place,
 * then the start its $07 names; the guard's own start (a short walk, then
 * stopped). The kind's one function, Kind08Start, takes every resume point
 * on to guard_pieces.c.
 */
#include <stdio.h>

#include "actor/guard.h"
#include "actor/kinds_d.h"
#include "data/kinds_d_tables.h"

/* The guard's own start ($A5:E58B): state 1, a walk of 40 frames at speed
   entry 6, then stopped. */
static int guard(Actor *a)
{
    a->own.bytes[G_STATE] = 0x01;
    a->own.bytes[G_LANDING] = 0x28;
    ActorSpeedXFromA5(a, 0x06);
    AnimStart(a, 1);
    a->resume = G_WALK_IN;
    return 1;
}

/*
 * @routine A5:E554 GuardRestart
 * WHAT IT DOES: GuardStart past its setting up: its fields cleared and on
 * by its $07: 0 and 12 the guard (12: shown at once, it never gives up the
 * chase); 7-10 the escort's spots (guard_escort.c); 11 a guard put at a spot
 * and held (A5:F00E GuardPlacedHold); 1-6 the scripted figures of Aladdin
 * (guard_scene.c), which start over as way 0 at the end of ways 1 and 6.
 */
static int restart(Actor *a)
{
    a->own.bytes[G_CLEARED_40] = 0;
    a->own.bytes[G_STATE] = 0;
    a->own.bytes[G_MOVING] = 0;
    a->own.bytes[G_IN_AIR] = 0;
    a->own.bytes[G_GONE] = 0;
    a->own.bytes[G_BACK_OFF] = 0;
    g.screen_signal = 0;
    g.rider_kind = 0;
    switch (a->spawn_byte & 0x7F) {
    case 0:
        return guard(a);
    case 12:
        a->own.bytes[G_SHOWN] = 0x01;
        return guard(a);
    case 1:
        a->resume = G_SCENE_RUNS;
        return GuardScene(a);
    case 2:
        a->resume = G_SCENE_LEAP;
        return GuardScene(a);
    case 3:
        a->resume = G_SCENE_HOPS;
        return GuardScene(a);
    case 4:
        a->resume = G_SCENE_DROPIN;
        return GuardScene(a);
    case 5:
        a->resume = G_SCENE_WALKOFF;
        return GuardScene(a);
    case 6:
        a->resume = G_SCENE_FALLPOSE;
        return GuardScene(a);
    case 7:
        a->resume = G_ESCORT_STAY18;
        return GuardEscort(a);
    case 8:
        a->resume = G_ESCORT_SPOT0;
        return GuardEscort(a);
    case 9:
        a->resume = G_ESCORT_SPOT0C;
        return GuardEscort(a);
    case 10:
        a->resume = G_ESCORT_SPOT10;
        return GuardEscort(a);
    case 11:
        /* A5:F00E GuardPlacedHold: put at spot $20, in state 4 (holding
           back while the player is to its left), stopped */
        a->own.bytes[G_STATE] = 0x04;
        ActorPlaceAtSpot(a, 0x20);
        AnimStart(a, 0);
        return guard_stop(a);
    default: {
        static uint8_t told[128];

        if (!told[a->spawn_byte & 0x7F]++)
            fprintf(stderr, "Kind08Start: $07 %u's start (A5:E56D's table) not converted\n", a->spawn_byte & 0x7F);
        a->resume = G_NOT_CONVERTED;
        return 1;
    }
    }
}

/*
 * @routine 83:9205 Kind08Enter
 * WHAT IT DOES: Actor kind $08's start entry: on at GuardStart.
 * Also A5:E515 GuardStart: the record shown and hittable (+$08 bit 0, flags
 * $90), its picture redrawn, facing right, its probe and picture lists, put
 * where the player is (16 lower); the count it keeps in cartridge RAM by
 * its kind stepped up (none on a retail cartridge: the read is open bus,
 * $70, so the count is $71); G_SHOWN cleared; then GuardRestart.
 */
int Kind08Start(Actor *a)
{
    if (a->resume == G_RESTART)
        return restart(a);
    if (a->resume != ACTOR_START)
        return GuardPieces(a);
    g.guard_kind_times2 = (uint8_t)(a->kind << 1);
    a->anim_flags |= 0x01;
    a->flags |= 0x90;
    a->redraw = 0xFF;
    a->facing = 0;
    a->flip = 0;
    a->probes = k08_probes();
    a->own_pictures = k08_pictures();
    a->x = with_pixels(a->x, pixels(g.actors[0].x));
    a->y = with_pixels(a->y, (uint16_t)(pixels(g.actors[0].y) + 0x10));
    g.guard_count = 0x71;
    a->own.bytes[G_SHOWN] = 0;
    return restart(a);
}

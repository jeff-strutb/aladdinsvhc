/*
 * kind7d.c: actor kind $7D, a spot that takes hold of the player: it keeps
 * its place, each frame follows its kept height as the view scrolls, steps
 * its state (take hold, keep him there or put him back, a pause), and ends
 * once it is far from the point actors are kept in range of.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "map/map.h"
#include "player/player.h"

int Kind7EStart(Actor *a);      /* kind $7E's start (kind7e.c): this kind's second entry */

#define STATE OWN_13            /* +$13: its state: 0 waiting, 1 holding him, 2 a pause */
#define PAUSE OWN_42            /* +$42: frames left in state 2 */

enum { KEEP = ACTOR_RESUME_OWN };

/*
 * @routine 84:90CE Kind7DKeepSpot
 * WHAT IT DOES: Actor kind $7D: flags $80, its probe list, its +$07 the low
 * six bits of its y (the spot) and the height it keeps from the table by it
 * (its own +$40-$41); each frame it keeps its place in +$4C and +$4E
 * (ActorKeepSpot), for good. Its second entry (83:92A4) goes on as kind
 * $7E's start (Kind7EStart).
 */
int Kind7DKeepSpot(Actor *a)
{
    uint16_t height;

    switch (a->resume) {
    case ACTOR_START:
        a->flags = 0x80;
        a->probes = k7d_probes();
        a->spawn_byte = (uint8_t)(pixels(a->y) & 0x3F);
        height = k7d_height(a->spawn_byte);
        a->own.bytes[OWN_40] = (uint8_t)height;
        a->own.bytes[OWN_41] = (uint8_t)(height >> 8);
        a->resume = KEEP;
        return 1;
    case ACTOR_START_SECOND:
        return ActorRunAs(a, Kind7EStart, ACTOR_START);
    default:
        ActorKeepSpot(a);
        return 1;
    }
}

/* whether ground is in front of it at probe `side` */
static bool ground_in_front(Actor *a, uint8_t side)
{
    MapPoint p = { 0 };

    a->probe_at = side;
    return GroundInFront(a, &p) != 0;
}

/* State 0: once he can take hold of it and no ground is in front of it
   either side, he is held (his code at PlayerHeld, the spot kept). */
static void take_hold(Actor *a)
{
    if (!ActorGrabbableByPlayer(a) || ground_in_front(a, 0) || ground_in_front(a, 4))
        return;
    g.actors[0].resume = PLAYER_AT_HELD;
    g.player_saved_x = pixels(a->x);
    g.player_saved_y = pixels(a->y);
    a->own.bytes[STATE]++;
}

/* State 1: ground in front of it either side puts him back where he was
   as the frame began; else, kept (g.player_no_actor_landing), he is held at
   its place, and it stays in state 1. */
static void hold(Actor *a)
{
    Actor *p = &g.actors[0];

    if (ground_in_front(a, 0) || ground_in_front(a, 4)) {
        p->x = with_pixels(p->x, pixels(g.frame_start_x));
        p->y = with_pixels(p->y, pixels(g.frame_start_y));
        g.player_let_go++;
    } else if ((uint8_t)g.player_no_actor_landing) {
        p->x = with_pixels(p->x, pixels(a->x));
        p->y = with_pixels(p->y, pixels(a->y));
        return;
    }
    a->own.bytes[STATE]++;
    a->own.bytes[PAUSE] = 0x10;
}

/*
 * @routine 84:9120 Kind7DStep
 * WHAT IT DOES: Kind $7D's state step: 0 waits for him to step onto it and
 * takes hold of him; 1 keeps him there, or puts him back where he was when
 * ground is in front of it; 2 a 16-frame pause, then 0 again.
 */
static void Kind7DStep(Actor *a)
{
    switch (a->own.bytes[STATE] & 0x7F) {
    case 0:
        take_hold(a);
        break;
    case 1:
        hold(a);
        break;
    case 2:
        if (!--a->own.bytes[PAUSE])
            a->own.bytes[STATE] = 0;
        break;
    default:                            /* past the table's three states: nothing sets one */
        break;
    }
}

/*
 * @routine 84:90F4 Kind7DWait
 * WHAT IT DOES: Kind $7D's wait routine (also 83:929F ActorKind7DWait, its
 * stub): its y the kept height less the backdrop's offset (the word at
 * $1A2C, the pixels of g.backdrop_offset_y), a step of its state, and it ends
 * once more than 224 pixels from the point actors are kept in range of.
 */
int Kind7DWait(Actor *a)
{
    uint16_t base = (uint16_t)(a->own.bytes[OWN_40] | a->own.bytes[OWN_41] << 8);

    a->y = with_pixels(a->y, (uint16_t)(base - (uint16_t)(int16_t)(int8_t)(g.backdrop_offset_y >> 8)));
    Kind7DStep(a);
    if ((uint16_t)(pixels(a->x) - g.range_x + 0xE0) < 0x01C0)
        return ACTOR_GOES_ON;
    return ActorEndPlaced(a);
}

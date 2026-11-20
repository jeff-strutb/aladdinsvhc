/*
 * actor_84_misc.c: steps of bank $84's actors.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "player/player.h"

/*
 * @routine 84:832B ActorAnimFreeFar
 * WHAT IT DOES: Plays the actor's animation, lets it hurt or be stomped
 * by the player (ActorStompOrHurt), and frees it when it is out of range
 * by box $58; then its parent (the record its own $4C names, as its index
 * in g.actors) counts one fewer in its own $42. ACTOR_ENDED when freed.
 */
int ActorAnimFreeFar(Actor *a)
{
    AnimStep(a);
    (void)ActorStompOrHurt(a);
    if (!ActorFreeOutOfRange(a, 0x58))
        return ACTOR_GOES_ON;
    g.actors[a->own.bytes[OWN_4C]].own.bytes[OWN_42]--;
    return ACTOR_ENDED;
}

/* the floor line: the text map's last row, 32 words of the heights of an
   actor's past (video's ActorYHistoryPush), at the word `at` */
#define FLOOR_WORDS_AT (32 * 31)

/*
 * @routine 84:8F3C ActorClampAboveFloor
 * WHAT IT DOES: Keeps the actor's y plus `offset` above the floor line at
 * its x: true (and its y moved up to the floor less the clearance) when it
 * reached it. The floor is the line's word for its x (x less $200, over 8
 * pixels), 9 bits signed, taken from $B0.
 */
static bool ActorClampAboveFloor(Actor *a, uint16_t offset)
{
    uint16_t column = (uint16_t)(((pixels(a->x) - 0x0200) >> 2) & 0x3E) >> 1;
    uint16_t raw = (uint16_t)(g.text_map[FLOOR_WORDS_AT + column] & 0x01FF);
    uint16_t floor, at;

    if (raw >= 0x0100)
        raw |= 0xFF00;
    floor = (uint16_t)(0xB0 - raw);
    at = (uint16_t)(pixels(a->y) + offset);
    if ((at & 0x8000) || at < floor)
        return false;
    a->y = with_pixels(a->y, (uint16_t)(floor - floor_clearance()));
    return true;
}

/*
 * @routine 84:8F2F ActorClampAboveFloor12
 * WHAT IT DOES: ActorClampAboveFloor with the offset 12.
 */
bool ActorClampAboveFloor12(Actor *a)
{
    return ActorClampAboveFloor(a, 0x0C);
}

/*
 * @routine 82:88CF ActorGrabTest
 * WHAT IT DOES: Tells whether the actor can grab the player: he may be
 * grabbed (no landing barred, not hurt, in the air), touches it
 * (ActorHitsTestBoxIfActive), is below it, and is within its reach on the
 * side he faces: left of him he must face right and his x pass its x plus
 * its hit box's half-width less 15; right of him, facing left, his x not
 * pass its x less the half-width plus 15.
 */
bool ActorGrabTest(Actor *a)
{
    const Actor *p = &g.actors[0];
    uint16_t ax = pixels(a->x), px = pixels(p->x);
    uint16_t half_w = hit_box(a->kind).half_w;

    if ((g.player_no_landing & 0x80) || g.player_hurt_count || !g.player_in_air)
        return false;
    if (!ActorHitsTestBoxIfActive(a))
        return false;
    if (pixels(a->y) >= pixels(p->y))
        return false;
    if (ax < px) {                      /* it is left of him: he must face right */
        if (!p->flip)
            return false;
        return (uint16_t)(ax + half_w - 0x0F) < px;
    }
    if (p->flip)                        /* right of him: he must face left */
        return false;
    return (uint16_t)(ax - half_w + 0x0F) >= px;
}

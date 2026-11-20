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

/*
 * @routine 84:8EF7 PlayerClampAboveFloor
 * WHAT IT DOES: Keeps Aladdin above the floor line once he is at x $200 or
 * more, by an offset of 30 pixels (24 with no state $8C counted and he in
 * the air); not while he rises, except with no state $8C counted and not in
 * the air. A hold of the line counts g.player_on_actor.
 */
static void PlayerClampAboveFloor(void)
{
    Actor *p = &g.actors[0];
    uint16_t offset = 0x001E;

    if (pixels(p->x) < 0x0200)
        return;
    if (g.player_hurt_count) {
        if (p->speed_y < 0)
            return;
    } else if (g.player_in_air) {
        if (p->speed_y < 0)
            return;
        offset = 0x0018;
    }
    if (ActorClampAboveFloor(p, offset))
        g.player_on_actor++;
}

/*
 * @routine 84:8F81 PlayerFallCheck
 * WHAT IT DOES: With the player's fall check on (g.chain_head_struck, $1B2B,
 * clear) and him at or below y $B8, puts him into state $8C facing as he is:
 * the fall. `by` is the actor whose code runs it.
 */
static void PlayerFallCheck(const Actor *by)
{
    uint16_t y = pixels(g.actors[0].y);

    if (g.chain_head_struck || (y & 0x8000))
        return;
    if (y >= 0x00B8)
        (void)PlayerEnter8CFacing(by);
}

/*
 * @routine 84:8EF0 Kind88Work
 * WHAT IT DOES: What actors of kind $88 do each frame they wait: keep
 * Aladdin above the floor line, and put him into his fall below it.
 */
int Kind88Work(Actor *a)
{
    PlayerClampAboveFloor();
    PlayerFallCheck(a);
    return ACTOR_GOES_ON;
}

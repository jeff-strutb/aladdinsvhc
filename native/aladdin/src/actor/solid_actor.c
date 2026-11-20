/*
 * solid_actor.c: an actor Aladdin cannot pass through: he stands on it,
 * bumps his head under it, or is pushed out of its side.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"

/* his offset from the actor plus `half` below `whole` (unsigned) */
static bool inside(uint16_t mine, uint16_t his, uint16_t half, uint16_t whole)
{
    return (uint16_t)(mine - his + half) < whole;
}

/*
 * @routine 83:865E ActorSolidForPlayer
 * WHAT IT DOES: Keeps Aladdin out of a solid actor, by box `at` of the
 * solid boxes. Nothing when it cannot be hit or he stands on an actor
 * already. With his x before this frame's move inside the box and his y
 * inside it but not in its upper part less 8 pixels: above its middle and
 * not rising, he stands on its top (one pixel above; 4 lower with no
 * health), counted on an actor and no longer in the air (ON_TOP); below
 * it, rising and in the air, he bumps his head: just under it, his
 * vertical speed stopped (UNDER); otherwise UNDER and nothing moves.
 * Else, his x now inside the box and his y within it, he is put just
 * outside the side he is on (BESIDE).
 * Also 83:8657 ActorSolidForPlayerLong, for a caller in another bank.
 */
ActorSolid ActorSolidForPlayer(Actor *a, uint8_t at)
{
    HitBox b = solid_box(at);
    Actor *p = &g.actors[0];
    uint16_t ax = pixels(a->x), ay = pixels(a->y), px = pixels(p->x), py = pixels(p->y);
    uint16_t upper = (uint16_t)(b.half_h - 8);

    if (!(a->flags & 0x40) || g.player_on_actor)
        return ACTOR_SOLID_BESIDE;
    if (inside(ax, pixels(g.frame_start_x), b.half_w, (uint16_t)(b.half_w << 1))
        && !inside(ay, py, upper, (uint16_t)(upper << 1))
        && inside(ay, py, b.half_h, (uint16_t)(b.half_h << 1))) {
        if (ay >= py) {
            /* above it: when not rising, he stands on it */
            if (p->speed_y & 0x8000)
                return ACTOR_SOLID_UNDER;
            p->y = with_pixels(p->y, (uint16_t)(ay + 1 - b.half_h));
            if (!g.health)
                p->y = with_pixels(p->y, (uint16_t)(pixels(p->y) + 4));
            g.player_on_actor++;
            g.player_in_air = 0;
            return ACTOR_SOLID_ON_TOP;
        }
        /* below its middle: rising in the air, he bumps his head */
        if (!g.player_in_air || p->speed_y >= 0)
            return ACTOR_SOLID_UNDER;
        p->y = with_pixels(p->y, (uint16_t)(ay + b.half_h));
        p->speed_y = 0;
        return ACTOR_SOLID_UNDER;
    }
    /* his x now inside the box and his y within it: pushed out sideways */
    if (inside(ax, px, b.half_w, (uint16_t)(b.half_w << 1))
        && inside((uint16_t)(ay + 1), py, b.half_h, (uint16_t)(b.half_h << 1)))
        p->x = with_pixels(p->x, ax >= px ? (uint16_t)(ax - b.half_w) : (uint16_t)(ax + 1 + b.half_w));
    return ACTOR_SOLID_BESIDE;
}

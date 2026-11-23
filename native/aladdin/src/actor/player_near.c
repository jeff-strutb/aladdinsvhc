/*
 * player_near.c: where an actor is against the player, or a point. All
 * by whole pixels; differences wrap at 16 bits, as the original's.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "actor/actor.h"

/* within: the distance plus the half-size below the size (unsigned) */
static bool within(uint16_t mine, uint16_t theirs, uint16_t half)
{
    return (uint16_t)(mine - theirs + half) < (uint16_t)(half << 1);
}

/*
 * @routine 81:AB62 ActorPlayerBelow
 * WHAT IT DOES: Says whether the player is level with or below the actor:
 * 1 when his y is at or below its y, else 0.
 */
uint8_t ActorPlayerBelow(const Actor *a)
{
    return pixels(g.actors[0].y) >= pixels(a->y);
}

/*
 * @routine 81:AB85 ActorSideOfPlayer
 * WHAT IT DOES: Says which way the actor must face to look at the player:
 * 0 (right) when his x is at or right of its x, else 1 (left).
 */
uint8_t ActorSideOfPlayer(const Actor *a)
{
    return pixels(g.actors[0].x) < pixels(a->x);
}

/*
 * @routine 81:ACDE ActorNearPlayer
 * WHAT IT DOES: Says whether the player is inside box `at` round the
 * actor: within its half-width across and its half-height up or down.
 */
bool ActorNearPlayer(const Actor *a, uint8_t at)
{
    HitBox b = near_box(at);
    const Actor *p = &g.actors[0];

    bool near = within(pixels(a->x), pixels(p->x), b.half_w) && within(pixels(a->y), pixels(p->y), b.half_h);

    DevTrigger(0, "near_box", a, near, (int16_t)(pixels(a->x) - pixels(p->x)), (int16_t)(pixels(a->y) - pixels(p->y)), -(int32_t)b.half_w,
               (int32_t)b.half_w - 1, -(int32_t)b.half_h, (int32_t)b.half_h - 1);
    return near;
}

/*
 * @routine 81:AD68 ActorNearPlayerX
 * WHAT IT DOES: Says whether the player is within span `at`'s half-width
 * of the actor across, whatever the height.
 */
bool ActorNearPlayerX(const Actor *a, uint8_t at)
{
    uint16_t half = near_span_x(at);
    bool near = within(pixels(a->x), pixels(g.actors[0].x), half);

    DevTrigger(1, "near_span_x", a, near, (int16_t)(pixels(a->x) - pixels(g.actors[0].x)), 0, -(int32_t)half, (int32_t)half - 1, 0, 0);
    return near;
}

/*
 * @routine 81:ADA4 ActorNearPlayerY
 * WHAT IT DOES: Says whether the player is within span `at`'s half-height
 * of the actor up or down, whatever the distance across.
 */
bool ActorNearPlayerY(const Actor *a, uint8_t at)
{
    uint16_t half = near_span_y(at);
    bool near = within(pixels(a->y), pixels(g.actors[0].y), half);

    DevTrigger(2, "near_span_y", a, near, 0, (int16_t)(pixels(a->y) - pixels(g.actors[0].y)), 0, 0, -(int32_t)half, (int32_t)half - 1);
    return near;
}

/*
 * @routine 81:AD17 ActorNearPoint
 * WHAT IT DOES: Says whether the actor is inside box `at` round the point
 * (x, y): within its half-width across and its half-height up or down.
 */
bool ActorNearPoint(const Actor *a, uint8_t at, uint16_t x, uint16_t y)
{
    HitBox b = near_box(at);

    bool near = within(pixels(a->x), x, b.half_w) && within(pixels(a->y), y, b.half_h);

    DevTrigger(3, "near_point", a, near, (int16_t)(pixels(a->x) - x), (int16_t)(pixels(a->y) - y), -(int32_t)b.half_w, (int32_t)b.half_w - 1,
               -(int32_t)b.half_h, (int32_t)b.half_h - 1);
    return near;
}

/*
 * @routine 81:AC68 ActorAimAt
 * WHAT IT DOES: The direction from the actor to the point (x, y), one of
 * 32: the octant from the signs of the differences (+4 the point above,
 * +2 left) and which is larger (+1 up or down), and the step in it from
 * the smaller against the larger (under 1/8, 3/8, 5/8, 7/8 of it, else 4);
 * the table's byte at the octant * 8 + the step.
 */
uint8_t ActorAimAt(const Actor *a, uint16_t x, uint16_t y)
{
    uint16_t dy = (uint16_t)(y - pixels(a->y));
    uint16_t dx = (uint16_t)(x - pixels(a->x));
    uint16_t large, small, eighth, reach;
    uint8_t octant = 0, step = 0;

    if (y < pixels(a->y)) {
        dy = (uint16_t)-dy;
        octant = 4;
    }
    if (x < pixels(a->x)) {
        dx = (uint16_t)-dx;
        octant += 2;
    }
    large = dx;
    small = dy;
    if (dx < dy) {
        octant++;
        large = dy;
        small = dx;
    }
    eighth = (uint16_t)(large >> 3);
    for (reach = eighth; reach < small; reach = (uint16_t)(reach + 2 * eighth))
        if (++step == 4)
            break;
    return direction_by_step((uint8_t)(octant * 8 + step));
}

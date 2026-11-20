/*
 * player_relation.c: where the player is from an actor: its direction, and
 * whether he is inside a box in front of the actor.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"

/*
 * @routine 81:ABFB ActorDirectionToPoint
 * WHAT IT DOES: The direction from the actor to the point (x, y), one of
 * 16: the quadrant from the signs of the differences (+4 the point above,
 * +2 left) and which is larger (+1 up or down), and the band: 0 when the
 * smaller difference is within a quarter of the larger, 1 within three
 * quarters, else 2; the table's byte at the quadrant * 4 + the band.
 */
uint8_t ActorDirectionToPoint(const Actor *a, uint16_t x, uint16_t y)
{
    uint16_t dy = (uint16_t)(y - pixels(a->y));
    uint16_t dx = (uint16_t)(x - pixels(a->x));
    uint16_t large, small, quarter;
    uint8_t quadrant = 0, band = 0;

    if (y < pixels(a->y)) {
        dy = (uint16_t)-dy;
        quadrant = 4;
    }
    if (x < pixels(a->x)) {
        dx = (uint16_t)-dx;
        quadrant += 2;
    }
    large = dx;
    small = dy;
    if (dx < dy) {
        quadrant++;
        large = dy;
        small = dx;
    }
    quarter = (uint16_t)(large >> 2);
    if (quarter < small) {
        band = 1;
        if ((uint16_t)(3 * quarter) < small)
            band = 2;
    }
    return direction_by_band((uint8_t)(quadrant * 4 + band));
}

/*
 * @routine 81:ABEA ActorDirectionToPlayer
 * WHAT IT DOES: The direction from the actor to the player
 * (ActorDirectionToPoint at his place).
 */
uint8_t ActorDirectionToPlayer(const Actor *a)
{
    return ActorDirectionToPoint(a, pixels(g.actors[0].x), pixels(g.actors[0].y));
}

/*
 * @routine 81:AEFD PlayerInFrontBox
 * WHAT IT DOES: Says whether the player is inside box `at` in front of
 * the actor: up or down within its half-height of the actor's y; across,
 * the actor's x plus the box's offset for its facing less his x, below
 * the box's width (the two offsets added), tested only when the height
 * passes.
 */
bool PlayerInFrontBox(const Actor *a, uint8_t at)
{
    FrontBox b = front_box(at);
    const Actor *p = &g.actors[0];
    uint16_t off = a->facing ? b.off_left : b.off_right;

    if ((uint16_t)(pixels(a->y) + b.half_h - pixels(p->y)) >= (uint16_t)(b.half_h << 1))
        return false;
    return (uint16_t)(pixels(a->x) + off - pixels(p->x)) < (uint16_t)(b.off_right + b.off_left);
}

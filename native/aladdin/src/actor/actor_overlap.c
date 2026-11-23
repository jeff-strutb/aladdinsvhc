/*
 * actor_overlap.c: an actor's box against the second pool's records (the
 * shots and the like), and the zones beside an actor against the test box.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "player/player.h"

#define ZONE_FIRST 0x64             /* ActorZoneHasTestPoint's zones start here in the table */

/* within: the distance plus the half-size below the size (unsigned) */
static bool within(uint16_t mine, uint16_t theirs, uint16_t half, uint16_t size)
{
    return (uint16_t)(mine - theirs + half) < size;
}

/*
 * @routine 83:8578 ActorTouchesSecondPool
 * WHAT IT DOES: The first of the five records of the second pool, in use,
 * able to be hit and not downed (bytes $08-$09 & $4080 = $4000), whose box
 * (by its kind's low 4 bits) overlaps the actor's hit box; NULL none. An
 * actor downed (its $08 bit 7) or that cannot be hit tests none. The
 * height's sum is taken less 4 as the half, but doubled whole.
 * Also 83:8570 ActorTouchesSecondPoolLong, for callers in other banks.
 */
Actor *ActorTouchesSecondPool(const Actor *a)
{
    HitBox mine = hit_box(a->kind);
    int i;

    if ((a->anim_flags & 0x80) || !actor_in_play(a))
        return NULL;
    for (i = 3; i < 3 + ACTOR_POOL_B_SIZE; i++) {
        Actor *s = &g.actors[i];
        HitBox b;
        uint16_t w, h;

        if (!s->state || (s->anim_flags & 0x80) || !actor_in_play(s))
            continue;
        b = second_pool_box(s->kind & 0x0F);
        w = (uint16_t)(b.half_w + mine.half_w);
        h = (uint16_t)(b.half_h + mine.half_h);
        if (within(pixels(a->x), pixels(s->x), w, (uint16_t)(w << 1))
            && within(pixels(a->y), pixels(s->y), (uint16_t)(h - 4), (uint16_t)(h << 1)))
            return s;
    }
    return NULL;
}

/* 83:879B ActorZonePlace: zone `at`'s point beside the actor: its place
   ahead along the actor's flip (added facing right, taken facing left),
   and down */
static void zone_place(const Actor *a, Zone z, uint16_t *x, uint16_t *y)
{
    *x = a->flip ? (uint16_t)(pixels(a->x) - (int16_t)z.ahead) : (uint16_t)(pixels(a->x) + (int16_t)z.ahead);
    *y = (uint16_t)(pixels(a->y) + (int16_t)z.down);
}

/*
 * @routine 83:8752 ActorZoneHasTestBox
 * WHAT IT DOES: Tells whether the test box overlaps zone `at` beside
 * the actor (ActorZonePlace, 83:879B): the half-sizes added, across then
 * down.
 * Also 83:879B ActorZonePlace, where the zone is (zone_place).
 */
bool ActorZoneHasTestBox(const Actor *a, uint8_t at)
{
    Zone z = zone_at(at);
    uint16_t x, y, w = (uint16_t)(z.half_w + g.test_box_half_w), h = (uint16_t)(z.half_h + g.test_box_half_h);

    zone_place(a, z, &x, &y);
    return within(x, g.test_box_x, w, (uint16_t)(w << 1)) && within(y, g.test_box_y, h, (uint16_t)(h << 1));
}

/*
 * @routine 83:8743 ActorZoneHurtsPlayer
 * WHAT IT DOES: When the test box overlaps zone `at` beside the
 * actor, the player is hurt (PlayerEnter8CFacing); says whether they met.
 * Also 83:873F ActorZoneHurtsPlayerLong, for callers in other banks.
 */
bool ActorZoneHurtsPlayer(Actor *a, uint8_t at)
{
    if (!ActorZoneHasTestBox(a, at))
        return false;
    PlayerEnter8CFacing(a);
    return true;
}

/*
 * @routine 83:87EC ActorZoneHasTestPoint
 * WHAT IT DOES: Tells whether the test box's centre is inside zone
 * `at` (of the zones from ZONE_FIRST) beside the actor: across within
 * its half-width of the zone's point, and from the point down less than
 * its height (its second byte, not doubled).
 * Also 83:87E8 ActorZoneHasTestPointLong, for callers in other banks.
 */
bool ActorZoneHasTestPoint(const Actor *a, uint8_t at)
{
    Zone z = zone_at((uint8_t)(at + ZONE_FIRST));
    uint16_t x, y, across;

    zone_place(a, z, &x, &y);
    across = (uint16_t)(g.test_box_x + z.half_w);
    if (across < x || (uint16_t)(across - x) >= (uint16_t)(z.half_w << 1))
        return false;
    return g.test_box_y >= y && (uint16_t)(g.test_box_y - y) < z.half_h;
}

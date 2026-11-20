/*
 * hit_box.c: whether an actor's box overlaps the box actors are tested
 * against (g.test_box_*, the player's).
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "player/player.h"

/* Both axes: the half-sizes added; the boxes overlap on an axis when the
   difference of centres plus the sum is below twice the sum (unsigned). */
static bool overlaps(const Actor *a, TestBoxTouch *t)
{
    HitBox b = hit_box(a->kind);
    uint16_t half_w = (uint16_t)(b.half_w + g.test_box_half_w);

    if (!(a->flags & 0x40))
        return false;
    if ((uint16_t)(pixels(a->x) - g.test_box_x + half_w) >= (uint16_t)(half_w << 1))
        return false;
    t->half_h = (uint16_t)(b.half_h + g.test_box_half_h);
    t->height = (uint16_t)(t->half_h << 1);
    t->y_sum = (uint16_t)(pixels(a->y) - g.test_box_y + t->half_h);
    return t->y_sum < t->height;
}

/*
 * @routine 83:8312 ActorHitsTestBox
 * WHAT IT DOES: Tells whether the actor, when it can be hit (bit 6 of its
 * flags), overlaps the test box: its kind's hit box round its place
 * against the test box round its centre, across first.
 * Also 83:8300 ActorHitsTestBoxLong, for callers in other banks.
 */
bool ActorHitsTestBox(const Actor *a)
{
    TestBoxTouch t;

    return overlaps(a, &t);
}

bool actor_touches_test_box(const Actor *a, TestBoxTouch *t)
{
    return !(a->anim_flags & 0x80) && overlaps(a, t);
}

/*
 * @routine 83:830E ActorHitsTestBoxIfActive
 * WHAT IT DOES: ActorHitsTestBox, for an actor whose byte at $08 has bit
 * 7 clear (not downed); otherwise no.
 * Also 83:82F8 ActorHitsTestBoxIfActiveLong, for callers in other banks.
 */
bool ActorHitsTestBoxIfActive(const Actor *a)
{
    TestBoxTouch t;

    return actor_touches_test_box(a, &t);
}

/*
 * @routine 83:8304 ActorHitsTestBoxIfPlayerFree
 * WHAT IT DOES: ActorHitsTestBoxIfActive, while Aladdin can be hit: bit 7
 * of his byte at $08 clear and no state $8C counted; otherwise no.
 * Also 83:82F4 ActorHitsTestBoxIfPlayerFreeLong (hit_box_long.c), for
 * callers in other banks.
 */
bool ActorHitsTestBoxIfPlayerFree(const Actor *a)
{
    if (g.actors[0].anim_flags & 0x80)
        return false;
    if (g.player_hurt_count)
        return false;
    return ActorHitsTestBoxIfActive(a);
}

/*
 * @routine 83:80A5 ActorHurtsPlayerOnTouch
 * WHAT IT DOES: When the actor touches Aladdin while he can be hit, hurts
 * him (state $8C), turned away from the actor. Says whether it did.
 * Also 83:80A1 ActorHurtsPlayerOnTouchLong (player_touch.c), for callers
 * in other banks.
 */
bool ActorHurtsPlayerOnTouch(Actor *a)
{
    if (!ActorHitsTestBoxIfPlayerFree(a))
        return false;
    return PlayerEnter8CFacing(a) != 0;
}

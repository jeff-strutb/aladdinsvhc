/*
 * player_landing.c: Aladdin landing on an actor's top, and being put
 * beside one.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "player/player.h"

#define BOX_NO_LANDING 0x3C

/*
 * @routine 83:84D5 PlayerStandOnActor
 * WHAT IT DOES: Stands Aladdin on the actor he has come down on: his code
 * goes on at PlayerBounce, his y is the actor's less the two boxes'
 * half-heights, and he has landed. (Its way out with a state $8C counted
 * is @unreachable: each caller has just found none.)
 */
void PlayerStandOnActor(const Actor *a, const TestBoxTouch *t)
{
    Actor *p = &g.actors[0];

    p->resume = PLAYER_AT_BOUNCE;
    p->y = with_pixels(p->y, (uint16_t)(pixels(a->y) - t->half_h));
    g.player_landed = 1;
}

/*
 * @routine 83:848D PlayerLandsOnActor
 * WHAT IT DOES: Tells whether Aladdin has come down on top of the actor
 * (touching the test box less than 10 pixels deep, not rising, alive, not
 * hurt, in the air, landing not barred, his box not the one that lands on
 * nothing), and if so stands him on it with an actor of kind $11 at his
 * feet.
 */
bool PlayerLandsOnActor(Actor *a)
{
    TestBoxTouch t;

    if (g.player_box_wanted == BOX_NO_LANDING || !actor_touches_test_box(a, &t))
        return false;
    if (!touch_from_above(&t, 10) || g.actors[0].speed_y < 0)
        return false;
    if (!g.health || g.player_hurt_count || !g.player_in_air || (g.player_no_landing & 0x80))
        return false;
    PlayerStandOnActor(a, &t);
    ActorSpawnKind11AtPlayer();
    return true;
}

/*
 * @routine 83:84F4 ActorSpawnKind11AtPlayer
 * WHAT IT DOES: Starts an actor of kind $11 (a landing's mark) at
 * Aladdin's feet: at his x, 16 pixels below his y, in a record of the
 * second pool; none free: nothing.
 */
void ActorSpawnKind11AtPlayer(void)
{
    const Actor *p = &g.actors[0];
    Actor *c = ActorAllocB();

    if (!c)
        return;
    c->state = ACTOR_STATE_NEW;
    c->kind = 0x11;
    c->spawn_byte = 0;
    c->anim_flags = 0x80;
    c->x = with_pixels(c->x, pixels(p->x));
    c->y = with_pixels(c->y, (uint16_t)(pixels(p->y) + 16));
}

/*
 * @routine 83:80C3 PlayerPushAside
 * WHAT IT DOES: Puts Aladdin beside the actor, just out of its box: on
 * its left when he is left of it (or at its x), else on its right: its
 * hit box's half-width plus 1 plus the test box's from its x (on the
 * left, one pixel nearer).
 * Also 83:80B5 PlayerPushAsideLong, for callers in other banks.
 */
void PlayerPushAside(const Actor *a)
{
    Actor *p = &g.actors[0];
    uint16_t ax = pixels(a->x);
    uint16_t d = (uint16_t)(hit_box(a->kind).half_w + 1 + g.test_box_half_w);

    if (ax >= pixels(p->x))         /* he is left of it, or at its x: on its left */
        d = (uint16_t)(1 - d);
    p->x = with_pixels(p->x, (uint16_t)(d + ax));
}

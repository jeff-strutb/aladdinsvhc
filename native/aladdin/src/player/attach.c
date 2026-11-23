/*
 * attach.c: Aladdin against an actor (the record's player/land_on_actor.c,
 * player_ride.c, player_attach.c): landing on its top, going along as it
 * moves, put at an offset from it or only that spot worked out.
 *
 * Not converted, dead: 82:8A42 ActorNearEscortMark (no code reaches it;
 * the record's proof).
 */
#include "player/player_own.h"

/*
 * @routine 82:89E3 PlayerLandOnActor
 * WHAT IT DOES: Lands him on the actor when he falls (or is still) onto its
 * top inside its landing box `box`: his feet are put on it, his fall
 * stopped, he is marked on an actor and no longer in the air. It can be
 * hit (+$09 bit 6), he is on no actor yet and may land on one. Returns 1
 * when he landed. (The original also counts a box matched in its caller's
 * scratch, $000C, which no caller here keeps.)
 */
int PlayerLandOnActor(const Actor *a, uint16_t box)
{
    Actor *p = &g.actors[0];
    LandBox b;

    if (!actor_in_play(a) || g.player_on_actor || no_actor_landing() || p->speed_y < 0)
        return 0;
    b = land_box(box);
    if ((uint16_t)(x_pixels(a) - x_pixels(p) + b.x_offset) >= b.width)
        return 0;
    if ((uint16_t)(y_pixels(a) - y_pixels(p) + b.y_offset) >= b.height)
        return 0;
    if ((uint16_t)(y_pixels(p) + b.below) >= y_pixels(a))
        return 0;
    set_y_pixels(p, (uint16_t)(y_pixels(a) - b.y_offset + 1));
    p->speed_y = 0;
    g.player_on_actor++;
    g.player_in_air = 0;
    return 1;
}

/*
 * @routine 81:9D63 PlayerMoveWithActor
 * WHAT IT DOES: Moves him by as much as the actor has moved since its place
 * was last kept in its own +$4C (x) and +$4E (y), so he goes along with it.
 */
void PlayerMoveWithActor(Actor *a)
{
    Actor *p = &g.actors[0];
    uint16_t kept_x = (uint16_t)(a->own.bytes[OWN_4C] | a->own.bytes[OWN_4D] << 8);
    uint16_t kept_y = (uint16_t)(a->own.bytes[OWN_4E] | a->own.bytes[OWN_4F] << 8);

    set_y_pixels(p, (uint16_t)(y_pixels(p) + (uint16_t)(y_pixels(a) - kept_y)));
    set_x_pixels(p, (uint16_t)(x_pixels(p) + (uint16_t)(x_pixels(a) - kept_x)));
}

/* his x: the actor's plus the offset's x while his flip is set, else less */
static void x_by_side(const Actor *a, uint8_t n)
{
    Actor *p = &g.actors[0];

    if (p->flip)
        set_x_pixels(p, (uint16_t)(x_pixels(a) + attach_offset_x(n)));
    else
        set_x_pixels(p, (uint16_t)(x_pixels(a) - attach_offset_x(n)));
}

/*
 * @routine 82:BB3A PlayerToActorAbove
 * WHAT IT DOES: Puts him at offset `n` from the actor: above it by the
 * offset's y, across by its x (to the right while his flip is set).
 */
void PlayerToActorAbove(const Actor *a, uint8_t n)
{
    set_y_pixels(&g.actors[0], (uint16_t)(y_pixels(a) - attach_offset_y(n)));
    x_by_side(a, n);
}

/*
 * @routine 82:BB26 PlayerToActorBelow
 * WHAT IT DOES: The same, below it by the offset's y.
 */
void PlayerToActorBelow(const Actor *a, uint8_t n)
{
    set_y_pixels(&g.actors[0], (uint16_t)(y_pixels(a) + attach_offset_y(n)));
    x_by_side(a, n);
}

/* where `to` would put him, kept in g.player_saved_x, _y; he stays */
static void spot(const Actor *a, uint8_t n, void (*to)(const Actor *, uint8_t))
{
    Actor *p = &g.actors[0];
    uint16_t x = x_pixels(p), y = y_pixels(p);

    to(a, n);
    g.player_saved_x = x_pixels(p);
    g.player_saved_y = y_pixels(p);
    set_y_pixels(p, y);
    set_x_pixels(p, x);
}

/*
 * @routine 82:BB73 PlayerSpotAboveActor
 * WHAT IT DOES: Where PlayerToActorAbove would put him for offset `n`, kept
 * in g.player_saved_x and _y; he stays where he is.
 */
void PlayerSpotAboveActor(const Actor *a, uint8_t n)
{
    spot(a, n, PlayerToActorAbove);
}

/*
 * @routine 82:BB9C PlayerSpotBelowActor
 * WHAT IT DOES: The same for PlayerToActorBelow.
 */
void PlayerSpotBelowActor(const Actor *a, uint8_t n)
{
    spot(a, n, PlayerToActorBelow);
}

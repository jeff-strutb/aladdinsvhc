/*
 * hurt.c: ground that hurts Aladdin (the record's player/hurt_ground.c): a
 * tile whose behaviour has bit 5 set puts him into state $8C, and his code
 * goes on at once at PlayerKnockback.
 *
 * Not converted, dead (nothing calls them; the record says so): 81:DCE4
 * PlayerOutOfWallsLedgeHurt (no JSR, JMP, JSL, JML or word names it) and
 * 81:DD88 PlayerSettleOrHurt (no code reaches it).
 */
#include "player/player_own.h"

MapPoint player_point;

/* The ground last probed hurts him: its behaviour's bit 5, unless his +$08
   bit 7 (hit already) or bit 4 (blinking) is set. */
static bool ground_hurts(const Actor *a)
{
    if (a->anim_flags & 0x90)
        return false;
    return (player_point.behaviour & TILE_HURTS) != 0;
}

/* hit by the ground: state $8C, and on at once in the knockback */
static int hurt(Actor *a)
{
    PlayerEnter8C();
    return PlayerGoTo(a, PLAYER_AT_KNOCKBACK);
}

/*
 * @routine 81:DCBC PlayerGroundSnapHurt
 * WHAT IT DOES: Stands him on the ground just under him; when that ground
 * hurts him, he is hit. While he stands on an actor, only marks ground
 * under him (the tile's kind 1).
 */
int PlayerGroundSnapHurt(Actor *a)
{
    if (g.player_on_actor) {
        player_point.kind = 0x01;
        return PLAYER_RETURNED;
    }
    snap_to_ground(a);
    return ground_hurts(a) ? hurt(a) : PLAYER_RETURNED;
}

/*
 * @routine 81:DD01 PlayerOutOfWallsHurt
 * WHAT IT DOES: Moves him out of a wall in front of him; when the ground
 * last probed hurts him, he is hit.
 */
int PlayerOutOfWallsHurt(Actor *a)
{
    out_of_walls(a);
    return ground_hurts(a) ? hurt(a) : PLAYER_RETURNED;
}

/*
 * @routine 81:DD1E PlayerGroundHurtTest
 * WHAT IT DOES: When the ground last probed hurts him, he is hit.
 */
int PlayerGroundHurtTest(Actor *a)
{
    return ground_hurts(a) ? hurt(a) : PLAYER_RETURNED;
}

/*
 * @routine 81:DD38 PlayerGroundHurtJump
 * WHAT IT DOES: The same, from the hover.
 */
int PlayerGroundHurtJump(Actor *a)
{
    return ground_hurts(a) ? hurt(a) : PLAYER_RETURNED;
}

/*
 * @routine 81:DD50 PlayerLandOrHurt
 * WHAT IT DOES: Stands him on a floor under him and stops his fall, unless
 * the ground in front of him at probe $1C is ground he passes through. A
 * floor that hurts him (and he is not hit already) hits him.
 */
int PlayerLandOrHurt(Actor *a)
{
    a->probe_at = 0x1C;
    if (GroundInFront(a, &player_point) && (player_point.behaviour & TILE_THROUGH))
        return PLAYER_RETURNED;
    if (!settle_on_floor(a) || (a->anim_flags & 0x80))
        return PLAYER_RETURNED;
    if (player_point.behaviour & TILE_HURTS)
        return hurt(a);
    a->speed_y = with_speed_word(a->speed_y, 0);
    return PLAYER_RETURNED;
}

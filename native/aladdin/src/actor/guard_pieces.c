/*
 * guard_pieces.c: actor kind $08's moves, the pieces its code words send
 * it to (stop, run, jump, give up, walk, the chase's pose), the run's
 * throw of up to three kind $90 fliers to find its way down, its climb of
 * a wall and its landing: GuardPieces takes every resume point past its
 * start. The routines only the guard calls are here too: GuardStepAhead,
 * GuardSnapToTile and ActorLedgeAhead.
 *
 * Dead, with its proof: A5:EAB3 ActorFlyFallCapped (guard_moves.c): no
 * JSR, JSL or table word in the record names it.
 */
#include "actor/guard.h"
#include "actor/kinds_d.h"
#include "data/kinds_d_tables.h"

/* 1 when its x is at or right of the player's (the original's carry, as
   LDA #0, ROL gives the facing toward him) */
static uint8_t side_of(const Actor *a)
{
    return pixels(a->x) >= pixels(g.actors[0].x) ? 1 : 0;
}

/*
 * @routine A5:EAC9 GuardSnapToTile
 * WHAT IT DOES: Sets it at the centre of the tile the last probe found
 * (the point in `p`): its x the tile's middle, its y 8 above the tile's
 * top.
 */
static void snap_to_tile(Actor *a, const MapPoint *p)
{
    a->x = with_pixels(a->x, (uint16_t)((p->x & 0xFFF0) + 8));
    a->y = with_pixels(a->y, (uint16_t)((p->y & 0xFFF0) - 8));
}

/*
 * @routine A5:EA2C ActorLedgeAhead
 * WHAT IT DOES: Looks for ground 48 pixels ahead of it with room above it,
 * from 32 pixels below its y up: true when found within four tiles (up
 * through empty tiles to a ground tile, then up through ground until a
 * tile is empty). Only the guard calls it.
 */
static bool ledge_ahead(Actor *a)
{
    MapPoint p;
    uint8_t left = 4;

    p.x = (uint16_t)(pixels(a->x) + (a->facing ? 0xFFD0 : 0x0030));
    p.y = (uint16_t)(pixels(a->y) + 0x20);
    for (;;) {
        (void)PointInGround(a, &p);
        if (p.behaviour & 0x07)
            break;
        if (!--left)
            return false;
        p.y = (uint16_t)(p.y - 0x10);
    }
    do {
        p.y = (uint16_t)(p.y - 0x10);
        (void)PointInGround(a, &p);
        if (!(p.behaviour & 0x07))
            return true;
    } while (--left);
    return false;
}

/*
 * @routine A5:EAE6 GuardStepAhead
 * WHAT IT DOES: The running guard's look ahead: the point ahead of it by
 * its speed's middle byte and its facing (tables at $80:CFF4, $80:D010); a
 * ground tile there: it jumps (speed entry 9 up) and its caller goes on at
 * GuardJump. True when it jumps.
 */
static bool step_ahead(Actor *a)
{
    uint8_t at = (uint8_t)((uint8_t)((uint8_t)(((a->speed_x >> 8) & 0xFF) << 1) ^ a->facing) << 1);
    MapPoint p;

    p.x = (uint16_t)(k08_step_x(at) + pixels(a->x));
    p.y = (uint16_t)(k08_step_y(at) + pixels(a->y));
    (void)PointInGround(a, &p);
    if (!(p.behaviour & 0x07))
        return false;
    ActorSpeedYFromA5(a, 0x09);
    return true;
}

/* A5:E5B6 GuardStop: on at its loop, whose first is a frame of waiting. */
int guard_stop(Actor *a)
{
    a->resume = G_STOP;
    return 1;
}

/* A5:E6A0 GuardJump's entry: under way, gravity $38, then in the air. */
static int jump(Actor *a)
{
    a->own.bytes[G_MOVING]++;
    a->accel_y = 0x38;
    a->resume = G_JUMP_AIR;
    return 1;
}

/* Into the air with motion preset `preset` (GuardRunDown's end). */
static int air_with(Actor *a, uint8_t preset)
{
    ActorMotionPreset(a, preset);
    a->resume = G_JUMP_AIR;
    return 1;
}

/*
 * @routine A5:E618 GuardRunDown
 * WHAT IT DOES: The guard off an edge (GuardRun's drop, also GuardFlip's):
 * with no ledge ahead below (ActorLedgeAhead), it drops with preset $80;
 * at one, it throws up to three kind $90 fliers ($07 0, 1, 2; each its
 * slot in g.flier_slots counted up) to find its way, and leaps by the
 * first that reports (a high nibble): preset $7A + its slot, or $7D when
 * none does.
 */
static int run_down(Actor *a)
{
    a->own.bytes[G_IN_AIR] = 0;
    a->own.bytes[G_MOVING]++;
    if (ledge_ahead(a))
        return air_with(a, 0x80);
    a->resume = G_RUN_DOWN_SPAWN;
    return 1;
}

/*
 * @routine A5:E774 GuardFlip
 * WHAT IT DOES: The guard turned back: facing and flip swapped, animation
 * 8, and down off the edge (GuardRunDown).
 */
static int flip(Actor *a)
{
    a->facing ^= 0x01;
    a->flip = a->facing;
    AnimStart(a, 8);
    return run_down(a);
}

/*
 * @routine A5:E73B GuardClimb
 * WHAT IT DOES: The guard up a wall: animation $0B, speed entries 0; each
 * frame it moves; over the top (no wall at probe 4) it jumps on (speed
 * entries 3); with ground 24 above it, it turns back (GuardFlip).
 */
static int climb(Actor *a)
{
    a->own.bytes[G_MOVING]++;
    ActorAnimChange(a, 0x0B);
    ActorSpeedXFromA5(a, 0x00);
    ActorSpeedYFromA5(a, 0x00);
    a->resume = G_CLIMB;
    return 1;
}

/*
 * @routine A5:E7EB GuardLand
 * WHAT IT DOES: The guard landed on a slope or ledge: set at the tile's
 * centre (GuardSnapToTile), pose 7 for 32 frames, its speed cleared and a
 * small hop (speed entry 6 down), then stopped.
 */
static int land(Actor *a, const MapPoint *p)
{
    a->own.bytes[G_MOVING]++;
    snap_to_tile(a, p);
    ActorAnimChange(a, 0x07);
    a->resume = G_LAND_WAIT;
    return 0x20;
}

/* GuardJump's landing: 10 frames to settle (animation $0A, no speed
   across), each on the ground, while Aladdin is within 48. */
static int landed(Actor *a)
{
    a->own.bytes[G_LANDING] = 0x0A;
    ActorAnimChange(a, 0x0A);
    ActorSpeedXClear(a);
    (void)ActorSnapToGround(a, NULL);
    a->resume = G_SETTLE;
    return 1;
}

/* GuardJump's frame in the air ($A5:E6A6). */
static int jump_air(Actor *a)
{
    MapPoint p;
    bool slope = false;

    ActorAnimRiseFall(a);
    ActorFly(a);
    if ((a->speed_y >> 16) & 0x80) {
        if (ActorSettleOnFloor(a, &p) && (p.behaviour & 0x08)) {
            a->flip = (uint8_t)(p.behaviour >> 7);
            a->own.bytes[G_MOVING] = 0;
            return land(a, &p);
        }
    }
    if (ActorOutOfWallAt(a, 0x08, &p)) {
        if (p.behaviour & 0x08) {
            slope = true;
        } else if (!(p.behaviour & 0x40)) {
            if (a->own.bytes[G_BACK_OFF])
                return flip(a);
            a->own.bytes[G_MOVING] = 0;
            return climb(a);
        }
    }
    if (slope || GroundBehindBit3(a, &p)) {
        if ((uint8_t)(p.behaviour >> 7) == a->flip) {
            a->own.bytes[G_MOVING] = 0;
            return land(a, &p);
        }
    }
    if ((a->speed_y >> 16) & 0x80)
        return 1;
    if (!ActorOutOfGround(a))
        return 1;
    return landed(a);
}

/*
 * @routine A5:E5E8 GuardRun
 * WHAT IT DOES: The guard running (its code word $E5E8): speed entry 9,
 * animation 1; each frame it runs along the ground posed by the tile under
 * it (ActorAnimByTile); at a wall (probe 8) it climbs; where the ground
 * ahead rises it jumps (GuardStepAhead); off an edge (animation $0A, a
 * frame in the air) it goes down (GuardRunDown).
 * Also A5:E5B6 GuardStop (each frame on the ground: animation $16 at an
 * edge tile, else 0 facing Aladdin or 6 facing away), A5:E6A0 GuardJump
 * (in the air: posed rising or falling; falling onto a slope or a ledge's
 * tile with behaviour bit 3, it lands there (GuardLand) turned down it; at
 * a wall it climbs, or turns back (G_BACK_OFF), unless the wall's tile is
 * a slope's or a ledge's; landed on the ground, it settles up to 10 frames
 * while Aladdin is within 48, then stops), A5:E794 GuardGiveUp (out of
 * sight and not hittable; once nothing holds Aladdin it is put at him, and
 * once it can be hit again it comes back facing him and leaps, speed
 * entries 3), A5:E807 GuardWalk (animation 4 for good, once Aladdin has no
 * health) and A5:E812 GuardChasePose (pose $14 for 96 frames, then $15 for
 * good).
 */
int GuardPieces(Actor *a)
{
    MapPoint p;
    int i;

    switch (a->resume) {
    case G_WALK_IN:
        ActorMoveX(a);
        (void)ActorSnapToGround(a, NULL);
        if (--a->own.bytes[G_LANDING])
            return 1;
        AnimStart(a, 0);
        ActorSpeedXClear(a);
        return guard_stop(a);

    case G_STOP:
        (void)ActorSnapToGround(a, &p);
        if (p.behaviour & 0x20)
            ActorAnimChange(a, 0x16);
        else
            ActorAnimChange(a, (side_of(a) ^ a->flip) ? 0x06 : 0x00);
        return 1;

    case G_RUN:
        ActorSpeedXFromA5(a, 0x09);
        ActorAnimChange(a, 0x01);
        a->resume = G_RUN_ON;
        return 1;
    case G_RUN_ON:
        ActorMoveX(a);
        if (ActorOutOfWall(a, 0x08))
            return climb(a);
        if (!ActorSnapToGround(a, &p)) {
            AnimStart(a, 0x0A);
            a->own.bytes[G_IN_AIR]++;
            a->resume = G_RUN_EDGE;
            return 1;
        }
        ActorAnimByTile(a, p.behaviour);
        if (step_ahead(a))
            return jump(a);
        return 1;
    case G_RUN_EDGE:
        return run_down(a);
    case G_RUN_DOWN_SPAWN:
        a->spawn_at = 0;
        for (i = 0; i < 3; i++)
            g.flier_slots[i] = 0;
        for (i = 0; i < 3; i++) {
            Actor *flier = ActorSpawn(a, 0x90);

            if (!flier)
                break;
            flier->spawn_byte = (uint8_t)i;
            g.flier_slots[i]++;
        }
        a->resume = G_RUN_DOWN_WAIT;
        return 1;
    case G_RUN_DOWN_WAIT: {
        uint8_t seen = (uint8_t)(g.flier_slots[0] | g.flier_slots[1] | g.flier_slots[2]);

        if (!seen)
            return air_with(a, 0x7D);
        if (!(seen & 0xF0))
            return 1;
        for (i = 0; i < 3; i++)
            g.flier_slots[i] &= 0xF0;
        for (i = 0; i < 3; i++)
            if (g.flier_slots[i])
                return air_with(a, (uint8_t)(0x7A + i));
        return air_with(a, 0x7D);
    }

    case G_JUMP:
        return jump(a);
    case G_JUMP_AIR:
        return jump_air(a);
    case G_SETTLE: {
        uint16_t d = (uint16_t)(pixels(g.actors[0].x) - pixels(a->x) + 0x30);

        if (d < 0x60 && --a->own.bytes[G_LANDING]) {
            (void)ActorSnapToGround(a, NULL);
            return 1;
        }
        a->own.bytes[G_MOVING] = 0;
        return guard_stop(a);
    }

    case G_CLIMB:
        ActorMoveY(a);
        ActorMoveX(a);
        if (!ActorOutOfWallProbe4(a)) {
            a->own.bytes[G_MOVING] = 0;
            ActorSpeedXFromA5(a, 0x03);
            ActorSpeedYFromA5(a, 0x03);
            return jump(a);
        }
        p.x = pixels(a->x);
        p.y = (uint16_t)(pixels(a->y) - 0x18);
        if (PointInGround(a, &p))
            return flip(a);
        return 1;

    case G_GIVE_UP:
        a->flags &= 0x7F;
        a->anim_flags &= 0xF7;
        /* fall through - its wait while something holds Aladdin */
    case G_GIVE_UP_HIDDEN:
        if (g.player_in_air | (uint8_t)g.player_no_actor_landing) {
            a->resume = G_GIVE_UP_HIDDEN;
            return 1;
        }
        a->x = with_pixels(a->x, pixels(g.actors[0].x));
        a->y = with_pixels(a->y, pixels(g.actors[0].y));
        a->resume = G_GIVE_UP_BACK;
        return 1;
    case G_GIVE_UP_BACK:
        if (a->flags & 0x40)
            return 1;
        a->anim_flags |= 0x08;
        a->flags |= 0x80;
        a->facing = a->flip = side_of(a);
        a->own.bytes[G_MOVING] = 0;
        ActorSpeedXFromA5(a, 0x03);
        ActorSpeedYFromA5(a, 0x03);
        return jump(a);

    case G_LAND_WAIT:
        ActorSpeedXClear(a);
        ActorSpeedYFromA5(a, 0x06);
        a->own.bytes[G_MOVING] = 0;
        return guard_stop(a);

    case G_WALK:
        a->own.bytes[G_MOVING]++;
        ActorAnimChange(a, 0x04);
        a->resume = G_STILL;
        return 1;
    case G_CHASE_POSE:
        a->own.bytes[G_MOVING]++;
        ActorAnimChange(a, 0x14);
        a->resume = G_CHASE_POSE_ON;
        return 0x60;
    case G_CHASE_POSE_ON:
        ActorAnimChange(a, 0x15);
        a->resume = G_STILL;
        return 1;

    default:                            /* G_STILL, G_NOT_CONVERTED */
        return 1;
    }
}

/*
 * hurt_ground.c: ground that hurts Aladdin: a tile whose behaviour has bit
 * 5 set puts him into state $8C (hurt) and his code starts over.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_player.h"
#include "actor.h"

void abi_ActorSnapToGround(void);
void abi_PlayerEnter8CLong(void);
void abi_ActorOutOfWalls(void);
void abi_GroundInFrontLong(void);
void abi_ActorSettleOnFloor(void);

#define TILE_BEHAVIOUR 0x0012u      /* the last probed tile's behaviour (bit 5: it hurts) */
#define TILE_HURTS 0x20
#define TILE_THROUGH 0x40           /* behaviour bit: he passes through it */
#define ACTOR_PROBE_AT 0x18u        /* the probe the ground tests use */
#define V_0013 0x0013u              /* set to 1 instead while V_1428 is set */
#define PLAYER_CODE 0x81DADDu       /* the player's code: where it starts over */

/* Hurt by the ground: not when bit 7 or bit 4 of ACTOR_08 is set (A
   tests them); else the tile's behaviour bit 5. */
static int ground_hurts(void)
{
    uint8_t flags = rd8(dpa(ACTOR_08)), tile;

    a8_load(flags);
    if (flags & 0x80)
        return 0;
    flag_set(P_Z, !(flags & 0x10));
    if (flags & 0x10)
        return 0;
    tile = rd8(dba(TILE_BEHAVIOUR));
    a8_load(tile);
    flag_set(P_Z, !(tile & TILE_HURTS));
    return (tile & TILE_HURTS) != 0;
}



/*
 * @routine 81:DD01 PlayerOutOfWallsHurt
 * WHAT IT DOES: Moves Aladdin out of a wall in front of him; when the
 * ground last probed hurts him, puts him into state $8C and starts his
 * code over.
 * HOW: ActorOutOfWalls; ACTOR_08 bit 7 or bit 4 set, or TILE_BEHAVIOUR
 * bit 5 clear: back. Else, a path no script runs: PlayerEnter8CLong, the
 * call's return address dropped (PLA PLA) and on at PLAYER_CODE.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code at its own level (81:CD90, 81:D3E4). Exit:
 * M and X as ActorOutOfWalls left them; A low, N and Z as the last test
 * left them. Hurt: never returns.
 */
void abi_PlayerOutOfWallsHurt(void)
{
    abi_ActorOutOfWalls();
    if (!ground_hurts())
        return;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return dropped */
    pla8();
    actor_leave(PLAYER_CODE);
}

/*
 * @routine 81:DD50 PlayerLandOrHurt
 * WHAT IT DOES: Stands Aladdin on a floor under him and stops his fall,
 * unless the ground in front of him at probe $1C is ground he passes
 * through; a floor that hurts him puts him into state $8C and starts his
 * code over.
 * HOW: ACTOR_PROBE_AT = $1C, GroundInFrontLong; in ground and
 * TILE_BEHAVIOUR bit 6 set: A = 0. Else ActorSettleOnFloor; Z set (not
 * settled), or ACTOR_08 bit 7 set: back. TILE_BEHAVIOUR bit 5 set, a path
 * no script runs: PlayerEnter8CLong, this call's return address and its
 * caller's dropped (four PLAs), and on at PLAYER_CODE. Else the low word
 * of the vertical speed cleared and A = 1.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:DB37) and from 81:D411. Exit: M 8-bit; A
 * low = 1 when he landed, 0 when the ground in front is passed through,
 * else as ActorSettleOnFloor or ACTOR_08 left it; N and Z from it; X as
 * ActorSettleOnFloor left it. Hurt: never returns.
 */
void abi_PlayerLandOrHurt(void)
{
    uint8_t v;

    a8_load(0x1C);
    wr8(dpa(ACTOR_PROBE_AT), 0x1C);
    abi_GroundInFrontLong();
    if (cpu.p & P_C) {
        v = rd8(dba(TILE_BEHAVIOUR));
        a8_load(v);
        flag_set(P_Z, !(v & TILE_THROUGH));
        if (v & TILE_THROUGH) {
            a8_load(0x00);
            return;
        }
    }
    abi_ActorSettleOnFloor();
    if (cpu.p & P_Z)
        return;
    v = rd8(dpa(ACTOR_08));
    a8_load(v);
    if (v & 0x80)
        return;
    v = rd8(dba(TILE_BEHAVIOUR));
    a8_load(v);
    flag_set(P_Z, !(v & TILE_HURTS));
    if (v & TILE_HURTS) {
        abi_PlayerEnter8CLong();
        pla8();                           /* its own return and its caller's dropped */
        pla8();
        pla8();
        pla8();
        actor_leave(PLAYER_CODE);
        return;
    }
    wr8(dpa(ACTOR_SPEED_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y + 1), 0);
    a8_load(0x01);
}

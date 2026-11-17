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
#define PLAYER_CODE 0x81DADDu       /* PlayerKnockback: the player's code in state $8C */

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



/* Each test's work below, apart from its abi_ entry: 0 when it returns,
   else PLAYER_CODE, where the player's code goes on, with the return
   addresses it drops already pulled. The player's states (player_states.c)
   call these and go on at PlayerKnockback in their own C, as the original
   goes on there by the jump within the same actor code; only the test's
   own call (abi_) ends at the jump. */
uint32_t ground_snap_hurt(void)
{
    uint8_t on = rd8(dba((uint16_t)V_1428));

    a8_load(on);
    if (on) {
        a8_load(0x01);
        wr8(dba(V_0013), 0x01);
        return 0;
    }
    abi_ActorSnapToGround();
    if (!ground_hurts())
        return 0;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return dropped */
    pla8();
    return PLAYER_CODE;
}

/*
 * @routine 81:DCBC PlayerGroundSnapHurt
 * WHAT IT DOES: Stands Aladdin on the ground just under him, and when that
 * ground hurts him, puts him into state $8C and starts his code over.
 * With V_1428 set, only sets V_0013 to 1.
 * HOW: V_1428 nonzero: V_0013 = 1. Else ActorSnapToGround; ACTOR_08 bit 7
 * or bit 4 set, or TILE_BEHAVIOUR bit 5 clear: back. Else
 * PlayerEnter8CLong, the call's return address dropped (PLA PLA) and on
 * at PlayerKnockback.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code at its own level (81:CCD4, 81:CD93). Exit: M
 * and X 8-bit; A low, N and Z as the last test left them, or A = 1. Hurt:
 * never returns.
 */
void abi_PlayerGroundSnapHurt(void)
{
    if (ground_snap_hurt())
        actor_leave(PLAYER_CODE);
}

uint32_t out_of_walls_hurt(void)
{
    abi_ActorOutOfWalls();
    if (!ground_hurts())
        return 0;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return dropped */
    pla8();
    return PLAYER_CODE;
}

/*
 * @routine 81:DD01 PlayerOutOfWallsHurt
 * WHAT IT DOES: Moves Aladdin out of a wall in front of him; when the
 * ground last probed hurts him, puts him into state $8C and starts his
 * code over.
 * HOW: ActorOutOfWalls; ACTOR_08 bit 7 or bit 4 set, or TILE_BEHAVIOUR
 * bit 5 clear: back. Else, a path no script runs: PlayerEnter8CLong, the
 * call's return address dropped (PLA PLA) and on at PlayerKnockback.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code at its own level (81:CD90, 81:D3E4). Exit:
 * M and X as ActorOutOfWalls left them; A low, N and Z as the last test
 * left them. Hurt: never returns.
 */
void abi_PlayerOutOfWallsHurt(void)
{
    if (out_of_walls_hurt())
        actor_leave(PLAYER_CODE);
}

uint32_t ground_hurt_test(void)
{
    if (!ground_hurts())
        return 0;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return and its caller's dropped */
    pla8();
    pla8();
    pla8();
    return PLAYER_CODE;
}

/*
 * @routine 81:DD1E PlayerGroundHurtTest
 * WHAT IT DOES: When the ground last probed hurts Aladdin, puts him into
 * state $8C and starts his code over.
 * HOW: ACTOR_08 bit 7 or bit 4 set, or TILE_BEHAVIOUR bit 5 clear: back
 * (by the RTS that ends 81:DD01). Else PlayerEnter8CLong, the return addresses of this call and its
 * caller's dropped (four PLAs) and on at PlayerKnockback.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from 81:D418, itself called by JSR from the player's code. Exit: M
 * and X 8-bit; A low, N and Z as the last test left them. Hurt: never
 * returns.
 */
void abi_PlayerGroundHurtTest(void)
{
    if (ground_hurt_test())
        actor_leave(PLAYER_CODE);
}

uint32_t land_or_hurt(void)
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
            return 0;
        }
    }
    abi_ActorSettleOnFloor();
    if (cpu.p & P_Z)
        return 0;
    v = rd8(dpa(ACTOR_08));
    a8_load(v);
    if (v & 0x80)
        return 0;
    v = rd8(dba(TILE_BEHAVIOUR));
    a8_load(v);
    flag_set(P_Z, !(v & TILE_HURTS));
    if (v & TILE_HURTS) {
        abi_PlayerEnter8CLong();
        pla8();                           /* its own return and its caller's dropped */
        pla8();
        pla8();
        pla8();
        return PLAYER_CODE;
    }
    wr8(dpa(ACTOR_SPEED_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y + 1), 0);
    a8_load(0x01);
    return 0;
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
 * caller's dropped (four PLAs), and on at PlayerKnockback. Else the low word
 * of the vertical speed cleared and A = 1.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:DB37) and from 81:D411. Exit: M 8-bit; A
 * low = 1 when he landed, 0 when the ground in front is passed through,
 * else as ActorSettleOnFloor or ACTOR_08 left it; N and Z from it; X as
 * ActorSettleOnFloor left it. Hurt: never returns.
 */
void abi_PlayerLandOrHurt(void)
{
    if (land_or_hurt())
        actor_leave(PLAYER_CODE);
}

uint32_t ground_hurt_jump(void)
{
    if (!ground_hurts())
        return 0;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return dropped */
    pla8();
    return PLAYER_CODE;
}

/*
 * @routine 81:DD38 PlayerGroundHurtJump
 * WHAT IT DOES: When the ground last probed hurts Aladdin, puts him into
 * state $8C and starts his code over. No script runs it.
 * HOW: ACTOR_08 bit 7 or bit 4 set, or TILE_BEHAVIOUR bit 5 clear: back
 * (by the RTS that ends 81:DD01). Else PlayerEnter8CLong, the call's
 * return address dropped (PLA PLA) and on at PlayerKnockback.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the hover (81:D898) at the actor's level. Exit: M and X 8-bit;
 * A low, N and Z as the last test left them. Hurt: never returns.
 */
void abi_PlayerGroundHurtJump(void)
{
    if (ground_hurt_jump())
        actor_leave(PLAYER_CODE);
}

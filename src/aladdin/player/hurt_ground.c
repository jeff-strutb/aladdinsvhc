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

#define TILE_BEHAVIOUR 0x0012u      /* the last probed tile's behaviour (bit 5: it hurts) */
#define TILE_HURTS 0x20
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
 * @routine 81:DCBC PlayerGroundSnapHurt
 * WHAT IT DOES: Stands Aladdin on the ground just under him, and when that
 * ground hurts him, puts him into state $8C and starts his code over.
 * With V_1428 set, only sets V_0013 to 1.
 * HOW: V_1428 nonzero: V_0013 = 1. Else ActorSnapToGround; ACTOR_08 bit 7
 * or bit 4 set, or TILE_BEHAVIOUR bit 5 clear: back. Else
 * PlayerEnter8CLong, the call's return address dropped (PLA PLA) and on
 * at PLAYER_CODE.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; called by
 * JSR from the player's code at its own level (81:CCD4, 81:CD93). Exit: M
 * and X 8-bit; A low, N and Z as the last test left them, or A = 1. Hurt:
 * never returns.
 */
void abi_PlayerGroundSnapHurt(void)
{
    uint8_t on = rd8(dba((uint16_t)V_1428));

    a8_load(on);
    if (on) {
        a8_load(0x01);
        wr8(dba(V_0013), 0x01);
        return;
    }
    abi_ActorSnapToGround();
    if (!ground_hurts())
        return;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return dropped */
    pla8();
    actor_leave(PLAYER_CODE);
}

/*
 * @routine 81:DD1E PlayerGroundHurtTest
 * WHAT IT DOES: When the ground last probed hurts Aladdin, puts him into
 * state $8C and starts his code over.
 * HOW: ACTOR_08 bit 7 or bit 4 set, or TILE_BEHAVIOUR bit 5 clear: back
 * (by the RTS that ends 81:DD01). Else PlayerEnter8CLong, the return addresses of this call and its
 * caller's dropped (four PLAs) and on at PLAYER_CODE.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; called by
 * JSR from 81:D418, itself called by JSR from the player's code. Exit: M
 * and X 8-bit; A low, N and Z as the last test left them. Hurt: never
 * returns.
 */
void abi_PlayerGroundHurtTest(void)
{
    if (!ground_hurts())
        return;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return and its caller's dropped */
    pla8();
    pla8();
    pla8();
    actor_leave(PLAYER_CODE);
}

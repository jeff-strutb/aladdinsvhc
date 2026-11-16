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



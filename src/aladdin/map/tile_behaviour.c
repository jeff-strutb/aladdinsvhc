/*
 * tile_behaviour.c: the behaviour of each map tile (solid, slope ...),
 * read by the collision code from TILE_BEHAVIOUR.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"

/* Two 4 KB behaviour tables in bank $82: the first for the map set
   $9C, the second's last 4 KB for the others. */
#define TILE_BEHAVIOUR_SETS 0x82E000u
#define MAP_SET 0x005Cu            /* which map set the level uses */
#define MAP_SET_FIRST 0x9C

/*
 * @routine 81:BEC5 TileBehaviourLoad
 * WHAT IT DOES: Loads the tile behaviour table of the level's map set.
 * HOW: With the data bank at $7E, copies 4 KB from bank $82's tables into
 * TILE_BEHAVIOUR, last byte first: from $82:E000 for map set $9C, from
 * $82:F000 for the others.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: X and Y 8-bit = $FF; A
 * low byte = the first byte copied; DB as at entry, N and Z from it (PLB);
 * C from the map-set compare; V unchanged.
 */
void abi_TileBehaviourLoad(void)
{
    int x, y;
    uint8_t set, v = 0;

    op_rep(P_X);
    op_push8(cpu.db);                         /* PHB, then the data bank = $7E */
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7E);
    op_push8(0x7E);
    cpu.db = op_pull8();
    cpu.p = nz8(cpu.p, cpu.db);
    set = rd8(0x7E0000u | MAP_SET);
    cpu.p = cmp8_flags(cpu.p, set, MAP_SET_FIRST);
    x = set == MAP_SET_FIRST ? 0x0FFF : 0x1FFF;
    for (y = 0x0FFF; y >= 0; y--, x--) {
        v = rd8(TILE_BEHAVIOUR_SETS + x);
        wr8(TILE_BEHAVIOUR + y, v);
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = 0xFFFF;
    cpu.y = 0xFFFF;
    op_sep(P_X);
    cpu.db = op_pull8();                      /* PLB */
    cpu.p = nz8(cpu.p, cpu.db);
}

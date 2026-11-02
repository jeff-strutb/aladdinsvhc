/*
 * spawn_pair.c: two children of hit box $A3 started at the spawner's
 * spawn point and moved to the point it keeps at $48.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorSpawn(void);
void abi_ActorSpawnPairA3(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define PAIR_SPAWNING 0x43u  /* set while the pair is started */
#define PAIR_AT_X 0x48u      /* where they start: x, then y pixels (words) */
#define PAIR_AT_Y 0x4Au

/* One kind $A3 at PAIR_AT_X, PAIR_AT_Y with $07 = `which`; 0 when none
   was free. */
static int pair_one(uint8_t which)
{
    uint32_t rec;
    uint16_t y;

    wr8(dpa(ACTOR_SPAWN_AT), 0);
    a8_load(0xA3);
    abi_ActorSpawn();
    if (cpu.p & P_N)
        return 0;
    rec = dba(0) + cpu.x;
    a8_load(which);
    wr8(rec + ACTOR_07, which);
    op_rep(P_M);
    wr16(rec + ACTOR_X + 1, dp16(PAIR_AT_X));
    y = dp16(PAIR_AT_Y);
    wr16(rec + ACTOR_Y + 1, y);
    a16_load(y);
    op_sep(0x30);
    return 1;
}

/*
 * @routine 84:8173 ActorSpawnPairA3
 * WHAT IT DOES: Starts two children with hit box $A3, their $07 0 and 1,
 * each placed at the spawner's point in its $48 (x) and $4A (y); while it
 * runs the spawner's $43 is set.
 * HOW: $43 up; for each child the spawn point 0 ($21), ActorSpawn, its
 * $07, and the whole pixels of its x and y from the two words; a spawn
 * with no record free ends it; then $43 cleared.
 * Entry: M 8-bit, X 8-bit; D = the spawner; DB = $81; called by JSR.
 * Exit: M 8-bit, X 8-bit (spawned) or as ActorSpawn left them; $43 = 0.
 */
void abi_ActorSpawnPairA3(void)
{
    wr8(dpa(PAIR_SPAWNING), (uint8_t)(rd8(dpa(PAIR_SPAWNING)) + 1));
    if (pair_one(0))
        pair_one(1);
    wr8(dpa(PAIR_SPAWNING), 0);
}

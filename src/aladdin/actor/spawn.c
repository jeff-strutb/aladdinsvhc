/*
 * spawn.c: an actor spawning another (a projectile, a piece, a child).
 *
 * The spawning actor is the record the direct page is; the child's record
 * comes from the free list (ActorAlloc) and is reached at X. The child
 * starts new ($0C), placed at the spawner's spawn offset, facing the same
 * way, sharing its animated-tile slot.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"

void abi_ActorAlloc(void);

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* The child's field `off` (absolute, X the record). */
static uint32_t child(uint8_t off)
{
    return abs_db((uint16_t)(cpu.x + off));
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

#define ACTOR_NEW 0x0C
/* Spawn offsets (words, Y the spawner's ACTOR_SPAWN_AT): across, taken
   away when bit 0 of the spawner's flip is set; down. */
#define SPAWN_OFFSET_X 0xDBBBu
#define SPAWN_OFFSET_Y 0xDC51u

/*
 * @routine 81:AA81 ActorSpawnPlace
 * WHAT IT DOES: Places a child at its spawner's spawn point.
 * HOW: The spawner's spawn offset index picks an offset across and one
 * down; across is added to the spawner's x, or taken away when bit 0 of
 * its flip is set; down is added to its y.
 * Entry: M 8-bit, X 16-bit; D = the spawner; X = the child; DB = $81.
 * Exit: M 8-bit; A = the child's y (16 bits); Y = the index; N, V, Z, C
 * from the y ADC; X unchanged.
 */
void abi_ActorSpawnPlace(void)
{
    uint16_t v;

    cpu.a = 0x0000;
    op_xba();
    set_a8(rd8(dp(ACTOR_SPAWN_AT)));
    cpu.y = cpu.a;
    cpu.p = nz16(cpu.p, cpu.y);
    op_rep(P_M);
    v = rd16(dp(ACTOR_FLIP));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 1));
    if (!(v & 1)) {
        cpu.p &= (uint8_t)~P_C;
        v = op_adc16(rd16(dp(ACTOR_X + 1)), rd16(abs_db((uint16_t)(SPAWN_OFFSET_X + cpu.y))), &cpu.p);
    } else {
        cpu.p |= P_C;
        v = op_sbc16(rd16(dp(ACTOR_X + 1)), rd16(abs_db((uint16_t)(SPAWN_OFFSET_X + cpu.y))), &cpu.p);
    }
    wr16(child(ACTOR_X + 1), v);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(rd16(dp(ACTOR_Y + 1)), rd16(abs_db((uint16_t)(SPAWN_OFFSET_Y + cpu.y))), &cpu.p);
    wr16(child(ACTOR_Y + 1), cpu.a);
    op_sep(P_M);
}

/*
 * @routine 81:AA11 ActorSpawnInit
 * WHAT IT DOES: Starts a child as a new actor at its spawner's spawn
 * point, facing the spawner's way.
 * HOW: The child's state = new; ActorSpawnPlace; the spawner's flip
 * becomes both the child's facing and flip; $34 = $FF; $07 is the
 * spawner's.
 * Entry: M 8-bit, X 16-bit; D = the spawner; X = the child; DB = $81;
 * called by JSR. Exit: A low byte = the spawner's $07; N, Z from it;
 * Y = the spawn offset index; X, C, V as ActorSpawnPlace leaves them.
 */
void abi_ActorSpawnInit(void)
{
    uint8_t v;

    set_a8(ACTOR_NEW);
    wr8(child(ACTOR_STATE), ACTOR_NEW);
    abi_ActorSpawnPlace();
    v = rd8(dp(ACTOR_FLIP));
    set_a8(v);
    wr8(child(ACTOR_FACING), v);
    wr8(child(ACTOR_FLIP), v);
    set_a8(0xFF);
    wr8(child(ACTOR_34), 0xFF);
    v = rd8(dp(ACTOR_07));
    set_a8(v);
    wr8(child(ACTOR_07), v);
}

/*
 * @routine 81:AA0E ActorSpawnTyped
 * WHAT IT DOES: Starts a child with hit box A at its spawner's spawn point.
 * HOW: Sets the child's $06 (its hit box), then goes on into ActorSpawnInit.
 * Entry: as ActorSpawnInit; A low byte = the hit box. Exit: as
 * ActorSpawnInit.
 */
void abi_ActorSpawnTyped(void)
{
    wr8(child(ACTOR_06), (uint8_t)cpu.a);
    abi_ActorSpawnInit();
}

/*
 * @routine 81:A9FA ActorTileSlotShare
 * WHAT IT DOES: Gives a child its spawner's animated-tile slot, and counts
 * it among the slot's holders.
 * HOW: With a slot (nonzero), the child's slot = it and the slot's count
 * of holders steps up.
 * Entry: M 8-bit, X 16-bit; D = the spawner; X = the child; DB a bank
 * whose $0000-$1FFF is work RAM; called by JSR. Exit: no slot: A low
 * byte = 0, Z set. Else: A = the new count (high byte 0), Y = the slot,
 * N, Z from the count. X, C, V unchanged.
 */
void abi_ActorTileSlotShare(void)
{
    uint8_t g = rd8(dp(ACTOR_TILE_SLOT)), n;

    set_a8(g);
    if (g == 0)
        return;
    wr8(child(ACTOR_TILE_SLOT), g);
    op_xba();
    set_a8(0);
    op_xba();
    cpu.y = cpu.a;
    cpu.p = nz16(cpu.p, cpu.y);
    n = (uint8_t)(rd8(abs_db((uint16_t)(ACTOR_TILE_SLOT_USERS + cpu.y))) + 1);
    set_a8(n);
    wr8(abs_db((uint16_t)(ACTOR_TILE_SLOT_USERS + cpu.y)), n);
}

/*
 * @routine 81:A9BF ActorSpawn
 * WHAT IT DOES: Spawns a child with hit box A at the actor's spawn point,
 * sharing its tile slot, if a record is free.
 * HOW: ActorAlloc; with a record, ActorSpawnTyped and ActorTileSlotShare.
 * Entry: M 8-bit, X 8-bit; D = the spawner; A low byte = the hit box; DB =
 * $81. Exit: none free: N set, as ActorAlloc leaves it. Spawned: X 16-bit
 * = the child; A low byte = 0; Z set, N clear; Y as ActorTileSlotShare leaves
 * it.
 */
void abi_ActorSpawn(void)
{
    abi_ActorAlloc();
    if (cpu.p & P_N)
        return;
    abi_ActorSpawnTyped();
    abi_ActorTileSlotShare();
    set_a8(0);
}

/*
 * @routine 81:A9CE ActorSpawnSameLook
 * WHAT IT DOES: Spawns a child with hit box A that looks like its spawner,
 * at the actor's spawn point, sharing its tile slot, if a record is free.
 * HOW: As ActorSpawn, with the spawner's sprite attributes copied to the
 * child before it takes the slot; returns with 8-bit registers.
 * Entry: as ActorSpawn. Exit: none free: as ActorSpawn. Spawned: M and X
 * 8-bit; X = the child's address's low byte; A low byte = 0; Z set, N
 * clear; Y as ActorTileSlotShare leaves it (low byte).
 */
void abi_ActorSpawnSameLook(void)
{
    abi_ActorAlloc();
    if (cpu.p & P_N)
        return;
    abi_ActorSpawnTyped();
    set_a8(rd8(dp(ACTOR_LOOK)));
    wr8(child(ACTOR_LOOK), (uint8_t)cpu.a);
    set_a8(rd8(dp(ACTOR_LOOK + 1)));
    wr8(child(ACTOR_LOOK + 1), (uint8_t)cpu.a);
    abi_ActorTileSlotShare();
    op_sep(P_M | P_X);
    set_a8(0);
}

/*
 * @routine 81:AA2D ActorSpawnFacing
 * WHAT IT DOES: Spawns a child with hit box A at the actor's spawn point,
 * facing X, with $07 = Y, if a record is free.
 * HOW: Keeps X and Y in scratch $0D and $0E; ActorAlloc; with a record,
 * the child's hit box and state (new), ActorSpawnPlace, then its facing and
 * flip = the kept X and its $07 = the kept Y.
 * Entry: M 8-bit, X 8-bit; D = the spawner; A low byte = the hit box; X =
 * the facing; Y = the $07 value; DB = $81. Exit: none free: N set, as
 * ActorAlloc leaves it. Spawned: M and X 8-bit; X = the child's address's
 * low byte; A low byte = the kept Y; Z set; N from it; Y = the spawn
 * offset index (low byte); C, V from ActorSpawnPlace.
 */
void abi_ActorSpawnFacing(void)
{
    uint8_t v;

    wr8(abs_db(SCRATCH + 1), (uint8_t)cpu.x);
    wr8(abs_db(SCRATCH + 2), (uint8_t)cpu.y);
    abi_ActorAlloc();
    if (cpu.p & P_N)
        return;
    wr8(child(ACTOR_06), (uint8_t)cpu.a);
    set_a8(ACTOR_NEW);
    wr8(child(ACTOR_STATE), ACTOR_NEW);
    abi_ActorSpawnPlace();
    v = rd8(abs_db(SCRATCH + 1));
    set_a8(v);
    wr8(child(ACTOR_FACING), v);
    wr8(child(ACTOR_FLIP), v);
    v = rd8(abs_db(SCRATCH + 2));
    set_a8(v);
    wr8(child(ACTOR_07), v);
    op_sep(P_M | P_X);
    cpu.p |= P_Z;
}

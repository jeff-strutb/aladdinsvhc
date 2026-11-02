/*
 * spawn_simple.c: a child started at its spawner's spawn point with only a
 * hit box (and the spawner's tile slot), or with a facing; and every
 * record put back free.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorSpawnShared(void);
void abi_ActorSpawnFacingY(void);
void abi_ActorsFreeAll(void);
void abi_ActorTileSlotShare(void);
void abi_ActorSpawnTyped(void);
void abi_ActorSpawnPlace(void);
void abi_ActorFreePlacedAtYLong(void);
void abi_ActorAlloc(void);
#include "actor.h"
#include "wram.h"

/*
 * @routine 81:A9E9 ActorSpawnShared
 * WHAT IT DOES: Starts a child with hit box A at the spawner's spawn point,
 * sharing the spawner's animated-tile slot, if a record is free.
 * HOW: ActorAlloc; then ActorSpawnTyped and ActorTileSlotShare.
 * Entry: M 8-bit, X 8-bit; A low byte = the hit box; D = the spawner; DB =
 * $81; called by JSL. Exit: none free: as ActorAlloc (N set). Started: M
 * 8-bit, X 8-bit; A low byte = 0, Z set; X = the child's low byte.
 */
void abi_ActorSpawnShared(void)
{
    abi_ActorAlloc();
    if (cpu.p & P_N)
        return;
    abi_ActorSpawnTyped();
    abi_ActorTileSlotShare();
    op_sep(0x30);
    a8_load(0x00);
}

/*
 * @routine 81:AA57 ActorSpawnFacingY
 * WHAT IT DOES: Starts a child with hit box A at the spawner's spawn point,
 * facing X, with Y as its $07, if a record is free.
 * HOW: ActorAlloc; the child's hit box, state $0C (new), ActorSpawnPlace;
 * its facing and flip from X, its $07 from Y.
 * Entry: M 8-bit, X 8-bit; A low byte = the hit box; X = the facing; Y =
 * the child's $07; D = the spawner; DB = $81; called by JSL. Exit: none
 * free: as ActorAlloc (N set). Started: M 8-bit, X 16-bit, X = the child;
 * Z set (SEP #$22); A low byte = Y at entry.
 */
void abi_ActorSpawnFacingY(void)
{
    uint32_t child;
    uint8_t v;

    wr8(dba(SCRATCH + 1), (uint8_t)cpu.x);
    wr8(dba(SCRATCH + 2), (uint8_t)cpu.y);
    abi_ActorAlloc();
    if (cpu.p & P_N)
        return;
    child = dba(0) + cpu.x;
    wr8(child + ACTOR_06, (uint8_t)cpu.a);
    a8_load(0x0C);
    wr8(child + ACTOR_STATE, 0x0C);
    abi_ActorSpawnPlace();
    child = dba(0) + cpu.x;
    v = rd8(dba(SCRATCH + 1));
    a8_load(v);
    wr8(child + ACTOR_FACING, v);
    wr8(child + ACTOR_FLIP, v);
    v = rd8(dba(SCRATCH + 2));
    a8_load(v);
    wr8(child + ACTOR_07, v);
    op_sep(0x22);
}

/*
 * @routine 81:9D82 ActorsFreeAll
 * WHAT IT DOES: Puts every record of the pool back on the free list.
 * HOW: The free list emptied (top $FFFE); ActorFreePlacedAtYLong for each
 * record from $13CB down to $0B5B; then the top set to $36 (28 entries).
 * Entry: M 8-bit; DB = $00-$3F or $80-$BF; called by JSL. Exit: M 8-bit,
 * X 8-bit; X = $36; Y = $0B (low byte of $0B0B).
 */
void abi_ActorsFreeAll(void)
{
    op_rep(P_X);
    cpu.x = 0xFFFE;                       /* the free list empty while they go */
    wr16(dba(ACTOR_FREE_TOP), 0xFFFE);
    cpu.y = 0x13CB;
    do {                                  /* each record from the last down */
        phy16();
        abi_ActorFreePlacedAtYLong();
        op_rep(P_M);
        flag_set(P_C, 1);
        pla16();
        cpu.a = op_sbc16(cpu.a, 0x0050, &cpu.p);
        cpu.y = cpu.a;
        op_sep(P_M);
        cmp16_of(cpu.y, 0x0B0B);
    } while (cpu.y != 0x0B0B);
    cpu.x = 0x0036;                       /* the 28 pool records free */
    nz16_of(0x0036);
    wr16(dba(ACTOR_FREE_TOP), 0x0036);
    op_sep(P_X);
}

/*
 * spawn_simple.c: a child started at its spawner's spawn point with only a
 * hit box (and the spawner's tile slot), or with a facing; and every
 * record put back free.
 */
#include "machine.h"
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
    abi_ActorAlloc();  /* 81A9E9 jsl */
    if (IF_N) goto L_81A9F9;  /* 81A9ED bmi */
    abi_ActorSpawnTyped();  /* 81A9EF jsr */
    abi_ActorTileSlotShare();  /* 81A9F2 jsr */
    op_sep(0x30);  /* 81A9F5 sep */
    lda8_v(0x00);  /* 81A9F7 lda */
L_81A9F9:
    return;  /* 81A9F9 rtl */
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
    stx8(ea_abs(SCRATCH + 1));  /* 81AA57 stx */
    sty8(ea_abs(SCRATCH + 2));  /* 81AA5A sty */
    abi_ActorAlloc();  /* 81AA5D jsl */
    if (IF_N) goto L_81AA80;  /* 81AA61 bmi */
    sta8(ea_absx(ACTOR_06));  /* 81AA63 sta */
    lda8_v(0x0C);  /* 81AA66 lda */
    sta8(ea_absx(0x0000));  /* 81AA68 sta */
    abi_ActorSpawnPlace();  /* 81AA6B jsl */
    lda8(ea_abs(SCRATCH + 1));  /* 81AA6F lda */
    sta8(ea_absx(0x0014));  /* 81AA72 sta */
    sta8(ea_absx(0x0015));  /* 81AA75 sta */
    lda8(ea_abs(SCRATCH + 2));  /* 81AA78 lda */
    sta8(ea_absx(0x0007));  /* 81AA7B sta */
    op_sep(0x22);  /* 81AA7E sep */
L_81AA80:
    return;  /* 81AA80 rtl */
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
    op_rep(0x10);  /* 819D82 rep */
    ldx16_v(0xFFFE);  /* 819D84 ldx */
    stx16(ea_abs(ACTOR_FREE_TOP));  /* 819D87 stx */
    ldy16_v(0x13CB);  /* 819D8A ldy */
L_819D8D:
    phy16();  /* 819D8D phy */
    abi_ActorFreePlacedAtYLong();  /* 819D8E jsl */
    op_rep(0x20);  /* 819D92 rep */
    sec();  /* 819D94 sec */
    pla16();  /* 819D95 pla */
    sbc16_v(0x0050);  /* 819D96 sbc */
    tay16();  /* 819D99 tay */
    op_sep(0x20);  /* 819D9A sep */
    cpy16_v(0x0B0B);  /* 819D9C cpy */
    if (!IF_Z) goto L_819D8D;  /* 819D9F bne */
    ldx16_v(0x0036);  /* 819DA1 ldx */
    stx16(ea_abs(ACTOR_FREE_TOP));  /* 819DA4 stx */
    op_sep(0x10);  /* 819DA7 sep */
    return;  /* 819DA9 rtl */
}

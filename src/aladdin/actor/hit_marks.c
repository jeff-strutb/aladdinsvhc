/*
 * hit_marks.c: the small actors (kinds $11 and $13) that mark a hit, each
 * started in a record of the second pool at another record's place.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAllocBLong(void);

/* Spawn11AtY's tail from the kind on, shared with Spawn13AtY: the new
   record (X) gets the kind in A, state $0C, $07 = 0, $08 = $80 and record
   Y's place; X and Y are pulled back. */
static void hit_mark_start(uint8_t kind)
{
    uint32_t rec = dba(0) + cpu.x;
    uint16_t y;

    wr8(rec + ACTOR_06, kind);
    wr8(rec + ACTOR_STATE, 0x0C);
    wr8(rec + ACTOR_07, 0);
    a8_load(0x80);
    wr8(rec + ACTOR_08, 0x80);
    ply16();                              /* the record it is put at */
    wr16(rec + ACTOR_X + 1, rd16(dba(ACTOR_X + 1) + cpu.y));
    y = rd16(dba(ACTOR_Y + 1) + cpu.y);
    wr16(rec + ACTOR_Y + 1, y);
    cpu.a = y;
    plx16();
}

/* None free: X and Y pulled back at 16 bits. */
static void hit_mark_none(void)
{
    op_rep(0x10);
    ply16();
    plx16();
}

/*
 * @routine 83:827D Spawn11AtY
 * WHAT IT DOES: Starts an actor of kind $11 at record Y's place, when the
 * second pool has a free record.
 * HOW: Keeps X and Y; ActorAllocBLong; with a record: kind $11 in its $06,
 * state $0C (new), $07 cleared, $08 = $80, and record Y's x and y pixels
 * copied into it; X and Y put back. The tail from the kind on is shared
 * with Spawn13AtY.
 * Entry: M 8-bit, X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: M 8-bit, X 16-bit; X, Y as on entry; N, Z
 * from X. Started: A = record Y's y pixels (16 bits, M 8-bit). None free:
 * A unchanged. C, V unchanged.
 */
void abi_Spawn11AtY(void)
{
    phx16();
    phy16();
    op_sep(0x10);
    jsl_frame(0x838281u);
    abi_ActorAllocBLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_N) {
        hit_mark_none();
        return;
    }
    hit_mark_start(0x11);
}

/*
 * @routine 83:82B5 Spawn13AtY
 * WHAT IT DOES: Starts an actor of kind $13 at record Y's place, when the
 * second pool has a free record.
 * HOW: As Spawn11AtY with kind $13: keeps X and Y, ActorAllocBLong, then
 * Spawn11AtY's own tail.
 * Entry: M 8-bit, X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: as Spawn11AtY.
 */
void abi_Spawn13AtY(void)
{
    phx16();
    phy16();
    op_sep(0x10);
    jsl_frame(0x8382B9u);
    abi_ActorAllocBLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_N) {
        hit_mark_none();
        return;
    }
    hit_mark_start(0x13);
}

void abi_Spawn11At1(void);

/*
 * @routine 83:82C3 LoneReturn82C3
 * WHAT IT DOES: Nothing: a lone RTS after Spawn13AtY's BRA. Nothing in the
 * game reaches it (no branch, JSR, JSL, JMP, pointer or word names it):
 * unit_bank83_pools runs it.
 * HOW: RTS.
 * Entry: any; called by JSR. Exit: all unchanged.
 */
void abi_LoneReturn82C3(void)
{
}

/*
 * @routine 83:82C4 Spawn11At1Long
 * WHAT IT DOES: Spawn11At1 for callers in other banks. Nothing in the game
 * calls it (no JSL, JMP, pointer or word names it): unit_bank83_pools runs
 * it.
 * HOW: JSR Spawn11At1, then RTL.
 * Entry: as Spawn11At1; called by JSL. Exit: as Spawn11At1.
 */
void abi_Spawn11At1Long(void)
{
    call_frame(0x82C6, abi_Spawn11At1);
}

/*
 * @routine 83:82C8 Spawn11At1
 * WHAT IT DOES: Starts an actor of kind $11 with $07 1 at record Y's place,
 * when the second pool has a free record. Nothing in the game calls it but
 * its long entry, which nothing calls.
 * HOW: Keeps X and Y; ActorAllocBLong; none free: Spawn11AtY's way out.
 * Else state $0C, kind $11, $07 1, record Y's x and y pixels copied into
 * it; X and Y put back.
 * Entry: M 8-bit, X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: as Spawn11AtY.
 */
void abi_Spawn11At1(void)
{
    phx16();
    phy16();
    op_sep(0x10);
    jsl_frame(0x8382CCu);
    abi_ActorAllocBLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_N) {
        hit_mark_none();
        return;
    }
    lda8_v(0x0C);
    sta8(ea_absx(ACTOR_STATE));
    lda8_v(0x11);
    sta8(ea_absx(ACTOR_06));
    lda8_v(0x01);
    sta8(ea_absx(ACTOR_07));
    ply16();
    op_rep(0x20);
    lda16(ea_absy(ACTOR_X + 1));
    sta16(ea_absx(ACTOR_X + 1));
    lda16(ea_absy(ACTOR_Y + 1));
    sta16(ea_absx(ACTOR_Y + 1));
    op_sep(0x20);
    plx16();
}

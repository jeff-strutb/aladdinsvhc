/*
 * hit_marks.c: the small actors (kinds $11 and $13) that mark a hit, each
 * started in a record of the second pool at another record's place.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAllocBLong(void);

/* Spawn11AtY's tail from the kind on, shared with Spawn13AtY: the new
   record (X) gets the kind in A, state $0C, $07 = 0, $08 = $80 and record
   Y's place; X and Y are pulled back. */
static void hit_mark_start(void)
{
    sta8(ea_absx(ACTOR_06));
    lda8_v(0x0C);
    sta8(ea_absx(ACTOR_STATE));
    stz8(ea_absx(ACTOR_07));
    lda8_v(0x80);
    sta8(ea_absx(ACTOR_08));
    ply16();
    op_rep(0x20);
    lda16(ea_absy(ACTOR_X + 1));
    sta16(ea_absx(ACTOR_X + 1));
    lda16(ea_absy(ACTOR_Y + 1));
    sta16(ea_absx(ACTOR_Y + 1));
    op_sep(0x20);
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
    abi_ActorAllocBLong();
    if (IF_N) {
        hit_mark_none();
        return;
    }
    lda8_v(0x11);
    hit_mark_start();
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
    abi_ActorAllocBLong();
    if (IF_N) {
        hit_mark_none();
        return;
    }
    lda8_v(0x13);
    hit_mark_start();
}

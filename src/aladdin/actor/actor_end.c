/*
 * actor_end.c: an actor's end: its record cleared and given back, its
 * animated-tile slot let go, and the actor loop goes on without it.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "ops65.h"

/* Where the actor loop takes the next record (docs/aladdin/ACTORS.md) */
#define ACTOR_LOOP_NEXT 0x8394BFu
#define ACTOR_FREE_LIST 0x0015A2u   /* the first pool's free records (words), a stack */
#define ACTOR_FREE_TOP 0x0015E4u    /* byte offset of its top entry */
#define ACTOR_SAVED_HIGH 0x35u   /* ActorJumpSaved: its address less one, high byte */
#define ACTOR_SAVED_LOW 0x36u    /* and low byte */

static uint16_t dp(uint16_t off)
{
    return (uint16_t)(cpu.d + off);
}

void abi_ActorClearFields(void);
void abi_ActorJumpSaved(void);

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 83:975B ActorClear
 * WHAT IT DOES: Clears an actor's record: it is free, still, drawn no
 * more, and one fewer of its kind is live.
 * HOW: Its kind's live count steps down; its state, flags, facing and
 * the other fields the actors use stay set are zeroed (on into
 * ActorClearFields, 83:9768).
 * Entry: M either, X 8-bit; D = the actor's record. Exit: M 8-bit; A low
 * = the kind's new count; X = the kind; N and Z from the count; the
 * rest unchanged.
 */
void abi_ActorClear(void)
{
    op_sep(0x20);
    cpu.x = rd8(dp(ACTOR_06));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    set_a8(rd8(KIND_COUNT + cpu.x));
    set_a8((uint8_t)(cpu.a - 1));
    wr8(KIND_COUNT + cpu.x, (uint8_t)cpu.a);
    abi_ActorClearFields();             /* on into it */
}

/*
 * @routine 83:9768 ActorClearFields
 * WHAT IT DOES: ActorClear's fields part: the record is free, still and
 * drawn no more, its kind's live count left as it is. ActorClear runs on
 * into it; ActorEndKeepCount (83:9798) calls it.
 * HOW: Its state, flags, facing and the other fields the actors use stay
 * set are zeroed (STZ each).
 * Entry: M 8-bit; D = the actor's record. Exit: all registers and flags
 * unchanged.
 */
void abi_ActorClearFields(void)
{
    static const uint8_t fields[] = {
        ACTOR_34, ACTOR_STATE, ACTOR_08, ACTOR_09, ACTOR_0A, ACTOR_FLIP, ACTOR_FACING, ACTOR_12, ACTOR_LOOK,
        ACTOR_2C, ACTOR_NEXT, ACTOR_38, ACTOR_13, ACTOR_REDRAW, ACTOR_33,
    };
    unsigned i;

    for (i = 0; i < sizeof fields; i++)
        wr8(dp(fields[i]), 0);
}

/*
 * @routine 83:95E5 ActorFree
 * WHAT IT DOES: Ends an actor of the second pool: clears its record and
 * puts it back on the pool's free stack.
 * HOW: ActorClear, then the record's address goes on top of the stack.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: A = the record's address, bytes
 * swapped; Y = the stack's new top; X = the kind; N and Z from the
 * address's high byte; the rest unchanged.
 */
void abi_ActorFree(void)
{
    uint8_t top;

    call_frame(0x95E7, abi_ActorClear);
    top = (uint8_t)(rd8_rmw(ACTOR_POOL2_TOP) + 1);
    wr8(ACTOR_POOL2_TOP, top);
    top = (uint8_t)(rd8_rmw(ACTOR_POOL2_TOP) + 1);
    wr8(ACTOR_POOL2_TOP, top);
    cpu.y = rd8(ACTOR_POOL2_TOP);
    op_lda16_imm(cpu.d);
    wr8(ACTOR_POOL2 + cpu.y, (uint8_t)cpu.a);
    op_xba();
    wr8(ACTOR_POOL2 + 1 + cpu.y, (uint8_t)cpu.a);
}

/*
 * @routine 83:9787 ActorJumpSaved
 * WHAT IT DOES: Turns the actor's wait routine on ($09 bit 7) and goes on
 * at the address after the one its $35 and $36 hold (high byte at $35).
 * Nothing in the game calls it (no JSR, JSL, JMP, pointer or word in the
 * ROM names it; the BRA at 83:9796 is the only way in):
 * unit_bank83_keep_paths runs it with an RTS's address less one there.
 * HOW: $09 ORA $80. $35, then $36 pushed; RTS (to that word plus 1, in
 * bank $83).
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: as the code at that address leaves it,
 * with the caller's return still on the stack.
 */
void abi_ActorJumpSaved(void)
{
    uint16_t ret;

    lda8(ea_dp(ACTOR_09));
    ora8_v(0x80);
    sta8(ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_SAVED_HIGH));
    pha8();
    lda8(ea_dp(ACTOR_SAVED_LOW));
    pha8();
    ret = pull16();                     /* the RTS */
    jump_routine(0x830000u | (uint16_t)(ret + 1));
}

/*
 * @routine 83:9794 ActorJumpSavedDrop2
 * WHAT IT DOES: ActorJumpSaved with its caller's return address dropped
 * first. Nothing in the game reaches it.
 * HOW: Two bytes pulled, then BRA to ActorJumpSaved.
 * @unreachable 83:9794-9797 It drops a caller's return that no caller
 * pushes: no JSR, JSL, JMP, branch, pointer or word in the ROM names
 * 83:9794 (the byte search's one hit, at 87:CB92, follows $8B, no jump
 * opcode), and a unit call at it would pull the stub's own return
 * (master, 2026-11-19).
 * Entry: M 8-bit; D = the actor's record; called by JSR from a routine
 * called by JSR. Exit: as ActorJumpSaved.
 */
void abi_ActorJumpSavedDrop2(void)
{
    pla8();
    pla8();
    abi_ActorJumpSaved();
}

/*
 * @routine 83:9798 ActorEndKeepCount
 * @actor
 * WHAT IT DOES: Ends an actor of the first pool without counting it off
 * its kind or freeing its tile slot: its record goes back on the free
 * list, its fields cleared, and the actor loop goes on to the next
 * record. No branch, jump, word or resume address in the ROM leads to it:
 * cov_actor_end_keep_count resumes a live record here.
 * HOW: The free list's top up by 2 and the record's address (D) put
 * there, low byte then high; ActorClearFields; JMP into the actor loop
 * where it takes the next record.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's RTI at the
 * record's resume address. Exit: never returns; in the actor loop.
 */
void abi_ActorEndKeepCount(void)
{
    uint8_t top;

    top = (uint8_t)(rd8_rmw(ACTOR_FREE_TOP) + 1);
    wr8(ACTOR_FREE_TOP, top);
    top = (uint8_t)(rd8_rmw(ACTOR_FREE_TOP) + 1);
    wr8(ACTOR_FREE_TOP, top);
    cpu.y = rd8(ACTOR_FREE_TOP);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
    op_lda16_imm(cpu.d);
    wr8(ACTOR_FREE_LIST + cpu.y, (uint8_t)cpu.a);
    op_xba();
    wr8(ACTOR_FREE_LIST + 1 + cpu.y, (uint8_t)cpu.a);
    call_frame(0x97AB, abi_ActorClearFields);
    actor_leave(ACTOR_LOOP_NEXT);
}

/*
 * @routine 83:9706 ActorTileSlotRelease
 * WHAT IT DOES: An ending actor lets go of its animated-tile slot; when it
 * was the slot's last user, the slot and the map cell that claimed it
 * are freed.
 * HOW: The slot's user count steps down; at 0 the cell's attribute word
 * is cleared, and the cell's further slots with it: the next one, or the
 * next two (by the cell's count of further slots). The actor's slot
 * becomes 0.
 * The cell's number is kept on the stack (PHA, PLX) across the clear.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: M and X 8-bit; A, X and the flags as
 * the last instructions leave them; Y unchanged.
 */
void abi_ActorTileSlotRelease(void)
{
    ldx8(ea_dp(ACTOR_TILE_SLOT));
    if (IF_Z)
        goto done;
    MEM8(dec, ea_absx((uint16_t)ACTOR_TILE_SLOT_USERS));
    if (!IF_Z)
        goto done;
    lda8(ea_absx((uint16_t)(TILE_SLOT_CELL - 1)));
    pha8();
    op_rep(0x30);
    ACC16(asl);
    and16_v(0x01FF);
    tax16();
    lda16_v(0x0000);
    sta16(ea_longx(CELL_ATTRS));
    op_sep(0x30);
    plx8();
    lda8(ea_longx(CELL_EXTRA_SLOTS));
    if (IF_Z)
        goto done;
    ldx8(ea_dp(ACTOR_TILE_SLOT));
    ACC8(dec);
    if (!IF_Z)
        stz8(ea_absx((uint16_t)(ACTOR_TILE_SLOT_USERS + 2)));
    stz8(ea_absx((uint16_t)(ACTOR_TILE_SLOT_USERS + 1)));
done:
    stz8(ea_dp(ACTOR_TILE_SLOT));
}

/*
 * @routine 83:95B8 ActorEnd
 * @actor
 * WHAT IT DOES: Ends an actor of the second pool for good: the actor
 * loop goes on to the next record.
 * HOW: ActorFree, ActorTileSlotRelease, then into the actor loop where it
 * takes the next record; the actor's code is left there.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by a jump from the actor's code, at
 * the actor loop's stack level. Exit: never returns; in the actor loop
 * with the registers ActorTileSlotRelease leaves.
 */
void abi_ActorEnd(void)
{
    call_frame(0x95BA, abi_ActorFree);
    call_frame(0x95BD, abi_ActorTileSlotRelease);
    actor_leave(ACTOR_LOOP_NEXT);
}

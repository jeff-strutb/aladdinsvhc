/*
 * actor_end.c: an actor's end: its record cleared and given back, its
 * animated-tile slot let go, and the actor loop goes on without it.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

/* Where the actor loop takes the next record (docs/aladdin/ACTORS.md) */
#define ACTOR_LOOP_NEXT 0x8394BFu

static uint16_t dp(uint16_t off)
{
    return (uint16_t)(cpu.d + off);
}

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
 * the other fields the actors use stay set are zeroed.
 * Entry: M either, X 8-bit; D = the actor's record. Exit: M 8-bit; A low
 * = the kind's new count; X = the kind; N and Z from the count; the
 * rest unchanged.
 */
void abi_ActorClear(void)
{
    static const uint8_t fields[] = {
        ACTOR_34, ACTOR_STATE, ACTOR_08, ACTOR_09, ACTOR_0A, ACTOR_FLIP, ACTOR_FACING, ACTOR_12, ACTOR_LOOK,
        ACTOR_2C, ACTOR_NEXT, ACTOR_38, ACTOR_13, ACTOR_REDRAW, ACTOR_33,
    };
    unsigned i;

    op_sep(0x20);
    cpu.x = rd8(dp(ACTOR_06));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    set_a8(rd8(KIND_COUNT + cpu.x));
    set_a8((uint8_t)(cpu.a - 1));
    wr8(KIND_COUNT + cpu.x, (uint8_t)cpu.a);
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

    abi_ActorClear();
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
 * @routine 83:9706 ActorTileSlotRelease
 * WHAT IT DOES: An ending actor lets go of its animated-tile slot; when it
 * was the slot's last user, the slot and the map cell that claimed it
 * are freed.
 * HOW: The slot's user count steps down; at 0 the cell's attribute word
 * is cleared, and the cell's further slots with it: the next one, or the
 * next two (by the cell's count of further slots). The actor's slot
 * becomes 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: M and X 8-bit; A, X and the flags as
 * the last instructions leave them; Y unchanged.
 */
void abi_ActorTileSlotRelease(void)
{
    uint8_t id, v;

    cpu.x = rd8(dp(ACTOR_TILE_SLOT));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    if (cpu.x == 0)
        goto done;
    v = (uint8_t)(rd8_rmw(ACTOR_TILE_SLOT_USERS + cpu.x) - 1);
    wr8(ACTOR_TILE_SLOT_USERS + cpu.x, v);
    cpu.p = nz8(cpu.p, v);
    if (v != 0)
        goto done;
    set_a8(rd8(TILE_SLOT_CELL - 1 + cpu.x));
    id = (uint8_t)cpu.a;
    op_rep(0x30);
    op_asl16();
    op_and16(0x01FF);
    op_tax16();
    op_lda16_imm(0);
    wr16(CELL_ATTRS + cpu.x, 0);
    op_sep(0x30);
    cpu.x = id;
    cpu.p = nz8(cpu.p, id);
    set_a8(rd8(CELL_EXTRA_SLOTS + cpu.x));
    if ((uint8_t)cpu.a == 0)
        goto done;
    cpu.x = rd8(dp(ACTOR_TILE_SLOT));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    set_a8((uint8_t)(cpu.a - 1));
    if ((uint8_t)cpu.a != 0)
        wr8(ACTOR_TILE_SLOT_USERS + 2 + cpu.x, 0);
    wr8(ACTOR_TILE_SLOT_USERS + 1 + cpu.x, 0);
done:
    wr8(dp(ACTOR_TILE_SLOT), 0);
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
    abi_ActorFree();
    abi_ActorTileSlotRelease();
    actor_leave(ACTOR_LOOP_NEXT);
}

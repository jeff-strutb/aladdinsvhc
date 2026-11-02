/*
 * actor_range.c: actors of the first pool that move out of range of the
 * camera are ended, and their placement may place them again.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "ops65.h"

void abi_ActorClear(void);
void abi_ActorTileSlotRelease(void);

/* Where the actor loop takes the next record (docs/aladdin/ACTORS.md) */
#define ACTOR_LOOP_NEXT 0x8394BFu
#define PLACED_FLAGS 0x7EC600u     /* a byte per placement: bit 7 set, never placed again */
#define RANGE_X 0x001A44u          /* the point actors are kept in range of (words) */
#define RANGE_Y 0x001A46u
/* In the bank DB names, 8 bytes an entry by X: added to the x and y
   differences, then the spans they must be below (words). */
#define RANGE_BOXES 0xCE73u

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/*
 * @routine 83:962C ActorInRange
 * WHAT IT DOES: Tells whether the actor is out of range of the camera's
 * point: C set when it is, on either axis, by the box X picks.
 * HOW: For x then y: the point less the actor's pixels, plus the box's
 * offset, compared unsigned with its span. In range: out through the
 * SEP #$30; RTS at $83:9629, the end of the routine before.
 * Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
 * boxes' bank, whose $0000-$1FFF is work RAM. Exit: A = the last sum (16
 * bits); N, Z and C from its compare; V from the last addition. Out of
 * range: M 16-bit. In range: M and X 8-bit. X, Y and the other flags
 * unchanged.
 */
void abi_ActorInRange(void)
{
    uint16_t x = (uint8_t)cpu.x;

    op_rep(P_M);
    cpu.a = rd16(RANGE_X);
    op_sub16(rd16(dp(ACTOR_X + 1)));
    op_add16(rd16(in_db(RANGE_BOXES + x)));
    op_cmp16(rd16(in_db(RANGE_BOXES + 2 + x)));
    if (cpu.p & P_C)
        return;
    cpu.a = rd16(RANGE_Y);
    op_sub16(rd16(dp(ACTOR_Y + 1)));
    op_add16(rd16(in_db(RANGE_BOXES + 4 + x)));
    op_cmp16(rd16(in_db(RANGE_BOXES + 6 + x)));
    if (!(cpu.p & P_C))
        op_sep(P_M | P_X);          /* to $83:9629: SEP #$30; RTS */
}

/*
 * @routine 83:96E2 ActorFreePlaced
 * WHAT IT DOES: Ends an actor of the first pool: its placement may place
 * it again (unless marked never), its record is cleared and goes back on
 * the free stack, and its animated-tile slot is let go.
 * HOW: The placed flag (by the actor's byte at $34) is cleared unless bit
 * 7 is set; ActorClear; the record's address pushed (2 bytes) on the free
 * stack; then on into ActorTileSlotRelease.
 * Entry: any widths; D = the actor's record; DB a bank whose $0000-$1FFF
 * is work RAM. Exit: as ActorTileSlotRelease.
 */
void abi_ActorFreePlaced(void)
{
    uint8_t x, top;

    op_sep(P_M | P_X);
    x = rd8(dp(ACTOR_34));
    cpu.x = x;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(PLACED_FLAGS + x));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    if (!(cpu.a & 0x80)) {
        cpu.a &= 0xFF00;
        cpu.p = nz8(cpu.p, 0);
        wr8(PLACED_FLAGS + x, 0);
    }
    abi_ActorClear();
    top = (uint8_t)(rd8_rmw(ACTOR_FREE_TOP) + 1);
    wr8(ACTOR_FREE_TOP, top);
    top = (uint8_t)(rd8_rmw(ACTOR_FREE_TOP) + 1);
    wr8(ACTOR_FREE_TOP, top);
    cpu.y = rd8(ACTOR_FREE_TOP);
    op_lda16_imm(cpu.d);
    wr8(ACTOR_FREE_LIST + cpu.y, (uint8_t)cpu.a);
    op_xba();
    wr8(ACTOR_FREE_LIST + 1 + cpu.y, (uint8_t)cpu.a);
    abi_ActorTileSlotRelease();
}

/*
 * @routine 83:96BE ActorEndOutOfRange
 * WHAT IT DOES: Ends the actor for good when it is out of range of the
 * camera's point by box X; otherwise returns to it.
 * HOW: ActorInRange; out of range, the call's return address is dropped,
 * ActorFreePlaced, and into the actor loop for the next record.
 * Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
 * boxes' bank, whose $0000-$1FFF is work RAM; called by JSR from the
 * actor's code at its own level. Exit: in range: as ActorInRange, M and X
 * 8-bit. Out of range: never returns; in the actor loop with the registers
 * ActorTileSlotRelease leaves.
 */
void abi_ActorEndOutOfRange(void)
{
    abi_ActorInRange();
    if (IF_C) goto out;
    op_sep(0x30);
    return;
out:
    op_sep(0x30);
    pla8();
    pla8();
    abi_ActorFreePlaced();
    actor_leave(ACTOR_LOOP_NEXT);
}

/*
 * @routine 83:96CA ActorEndPlaced
 * @actor
 * WHAT IT DOES: Ends an actor of the first pool for good: its placement
 * may place it again, and the actor loop goes on to the next record.
 * HOW: ActorFreePlaced, then into the actor loop where it takes the next
 * record; the actor's code is left there.
 * Entry: D = the actor's record; DB a bank whose $0000-$1FFF is work RAM;
 * reached by a jump from the actor's code, at the actor loop's stack
 * level. Exit: never returns; in the actor loop with the registers
 * ActorTileSlotRelease leaves.
 */
void abi_ActorEndPlaced(void)
{
    abi_ActorFreePlaced();
    actor_leave(ACTOR_LOOP_NEXT);
}

/*
 * @routine 83:966C ActorFreeOutOfRange
 * WHAT IT DOES: Ends the actor (ActorFreePlaced) when it is out of range
 * of the camera's point by box X.
 * HOW: ActorInRange, then ActorFreePlaced when C is set; 8-bit registers
 * either way.
 * Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
 * boxes' bank, whose $0000-$1FFF is work RAM. Exit: C set when it was
 * ended; M and X 8-bit; the other registers as ActorInRange or
 * ActorTileSlotRelease leave them.
 */
void abi_ActorFreeOutOfRange(void)
{
    abi_ActorInRange();
    if (cpu.p & P_C)
        abi_ActorFreePlaced();
    op_sep(P_M | P_X);
}

/*
 * @routine 83:9668 ActorFreeOutOfRangeLong
 * WHAT IT DOES: ActorFreeOutOfRange for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorFreeOutOfRange; called by JSL. Exit: as
 * ActorFreeOutOfRange.
 */
void abi_ActorFreeOutOfRangeLong(void)
{
    abi_ActorFreeOutOfRange();
}

/*
 * @routine 83:96D4 ActorFreePlacedAtY
 * WHAT IT DOES: ActorFreePlaced for the actor whose record Y points at,
 * from another actor's code.
 * HOW: P and D are kept on the stack; D = Y for the call.
 * Entry: X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: as ActorFreePlaced, with D and P as at entry.
 */
void abi_ActorFreePlacedAtY(void)
{
    php();
    phd();
    phy16();
    pld();
    abi_ActorFreePlaced();
    pld();
    plp();
}

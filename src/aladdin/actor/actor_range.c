/*
 * actor_range.c: actors of the first pool that move out of range of the
 * camera are ended, and their placement may place them again.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"
#include "ops65.h"

void abi_ActorClear(void);
void abi_ActorTileSlotRelease(void);
void abi_ActorFreePlacedAtY(void);
void abi_ActorEndPlaced(void);

/* Where the actor loop takes the next record (docs/aladdin/ACTORS.md) */
#define ACTOR_LOOP_NEXT 0x8394BFu
#define PLACED_FLAGS 0x7EC600u     /* a byte per placement: bit 7 set, never placed again */
#define RANGE_X 0x001A44u          /* the point actors are kept in range of (words) */
#define RANGE_Y 0x001A46u
#define CHILD_A 0x40u              /* ActorEndFarWithChildren: its first child's record (word) */
#define CHILD_B 0x42u              /* its second child's record (word) */
#define CHILD_COUNT 0x44u          /* how many children it has (1 or 2) */
#define ACTOR_END_PLACED 0x8396CAu
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
    call_frame(0x96F4, abi_ActorClear);
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
    call_frame(0x96C0, abi_ActorInRange);
    if (IF_C) goto out;
    op_sep(0x30);
    return;
out:
    op_sep(0x30);
    pla8();
    pla8();
    call_frame(0x96CC, abi_ActorFreePlaced);
    actor_leave(ACTOR_LOOP_NEXT);
}

/*
 * @routine 83:9653 ActorInRange8
 * WHAT IT DOES: ActorInRange, with M and X 8-bit after it either way.
 * No caller in the ROM (2026-11-18): no JSR, JMP, JSL or JML to it, no word
 * $9653 (a table entry) or $9652 (a pushed RTS address) in bank $83, and
 * none of the 233 scripts' traces runs it; unit_actor_in_range8 calls it.
 * HOW: JSR ActorInRange; SEP #$30; RTS.
 * Entry: as ActorInRange; called by JSR. Exit: C set when out of range;
 * M and X 8-bit; A, N, V and Z as ActorInRange leaves them.
 */
void abi_ActorInRange8(void)
{
    call_frame(0x9655, abi_ActorInRange);
    op_sep(0x30);
}

/*
 * @routine 83:9659 ActorEndOutOfRangeLong
 * WHAT IT DOES: Ends the actor for good when it is out of range of the
 * camera's point by box X; otherwise returns to it. For callers in other
 * banks.
 * HOW: ActorInRange (JSR); in range: SEP #$30, RTL. Out of range: SEP
 * #$30, the JSL's three return bytes pulled, on into ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
 * boxes' bank, whose $0000-$1FFF is work RAM; called by JSL from the
 * actor's code at its own level (84:9235, 84:849B), the JSL's return
 * address on the stack (jsl_frame). Exit: in range: as ActorInRange, M
 * and X 8-bit. Out of range: never returns; as ActorEndPlaced.
 */
void abi_ActorEndOutOfRangeLong(void)
{
    call_frame(0x965B, abi_ActorInRange);
    op_sep(0x30);
    if (!IF_C)
        return;
    pla8();
    pla8();
    pla8();
    /* BRA into ActorEndPlaced, at the actor loop's stack level: this call
       ends there, past its exit level, as the original's does, and
       ActorEndPlaced runs as its own (as KindA1Touch's end) */
    actor_leave(ACTOR_END_PLACED);
}

/*
 * @routine 83:96C6 ActorEndDropReturn
 * WHAT IT DOES: Ends the actor for good from a routine its code called:
 * the call's return address dropped, then as ActorEndPlaced.
 * HOW: 8-bit registers; two bytes pulled (the JSR's return address); on
 * into ActorEndPlaced. ActorEndOutOfRange's own way out.
 * Entry: D = the actor's record; DB a bank whose $0000-$1FFF is work RAM;
 * reached by a jump from a routine the actor's code called by JSR, one
 * return address above the actor loop's stack level. Exit: never returns;
 * as ActorEndPlaced.
 */
void abi_ActorEndDropReturn(void)
{
    op_sep(0x30);
    pla8();
    pla8();
    /* on into ActorEndPlaced by falling through, not a jump: the call
       goes on through it to the actor loop */
    abi_ActorEndPlaced();
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
    /* JSR ActorFreePlaced (its last byte $83:96CC): its return address on
       the stack while it runs, pulled by its RTS */
    call_frame(0x96CC, abi_ActorFreePlaced);
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
    call_frame(0x966E, abi_ActorInRange);
    if (cpu.p & P_C)
        call_frame(0x9673, abi_ActorFreePlaced);
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
    call_frame(0x966A, abi_ActorFreeOutOfRange);
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
    call_frame(0x96DA, abi_ActorFreePlaced);
    pld();
    plp();
}

/*
 * @routine 83:C4AB ActorEndFarWithChildren
 * WHAT IT DOES: Ends an actor and its one or two children for good once it
 * is 256 pixels or more from the range point, across or up and down,
 * unless it can be hit (bit 6 of $09).
 * HOW: BIT $09, V set: back. Else RANGE_X less its x plus $100 and
 * RANGE_Y less its y plus $100, each below $200: back (M 8-bit). Far:
 * ActorFreePlacedAtY for the child at CHILD_A, and for CHILD_B when
 * CHILD_COUNT is not 1 (no script runs this); its return dropped and on
 * into ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from the actor's code at its own
 * level. Exit: near: M 8-bit, N, Z and C from the y test (V set: N, V, Z
 * from the BIT). Far: never returns; as ActorEndPlaced.
 */
void abi_ActorEndFarWithChildren(void)
{
    bit8_v(rd8(ea_dp(ACTOR_09)));  /* 83C4AB bit */
    if (IF_V) return;  /* 83C4AD bvs (83C4EB rts) */
    op_rep(0x20);  /* 83C4AF rep */
    sec();  /* 83C4B1 sec */
    lda16(ea_abs(RANGE_X));  /* 83C4B2 lda */
    sbc16_v(rd16(ea_dp(ACTOR_X + 1)));  /* 83C4B5 sbc */
    clc();  /* 83C4B7 clc */
    adc16_v(0x0100);  /* 83C4B8 adc */
    cmp16_v(0x0200);  /* 83C4BB cmp */
    if (IF_C) goto far;  /* 83C4BE bcs */
    sec();  /* 83C4C0 sec */
    lda16(ea_abs(RANGE_Y));  /* 83C4C1 lda */
    sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));  /* 83C4C4 sbc */
    clc();  /* 83C4C6 clc */
    adc16_v(0x0100);  /* 83C4C7 adc */
    cmp16_v(0x0200);  /* 83C4CA cmp */
    if (!IF_C) {  /* 83C4CD bcc */
        op_sep(0x20);  /* 83C4E9 sep */
        return;  /* 83C4EB rts */
    }
far:
    op_sep(0x20);  /* 83C4CF sep */
    op_rep(0x10);  /* 83C4D1 rep */
    ldy16(ea_dp(CHILD_A));  /* 83C4D3 ldy */
    call_frame(0xC4D7, abi_ActorFreePlacedAtY);  /* 83C4D5 jsr */
    lda8(ea_dp(CHILD_COUNT));  /* 83C4D8 lda */
    ACC8(dec);  /* 83C4DA dec */
    if (IF_Z) goto one;  /* 83C4DB beq */
    ldy16(ea_dp(CHILD_B));  /* 83C4DD ldy */
    call_frame(0xC4E1, abi_ActorFreePlacedAtY);  /* 83C4DF jsr */
one:
    op_sep(0x10);  /* 83C4E2 sep */
    pla8();  /* 83C4E4 pla */
    pla8();  /* 83C4E5 pla */
    actor_leave(ACTOR_END_PLACED);  /* 83C4E6 jmp */
}

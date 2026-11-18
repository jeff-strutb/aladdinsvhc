/*
 * kind86.c: actor kind $86 (stage a5 s0), a pair that drops and rises
 * again: the placed one ($07 0) and its child ($07 4) kept $2B above it.
 * Here its wait (the hold, the hurting zone, the child kept above, the end
 * out of view) and its second state's end. Its start (84:962D) is not
 * filed yet.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorHoldPlayerLong(void);
void abi_ActorZoneHurtsPlayerLong(void);
void abi_AnimStep(void);
void abi_ActorFreePlacedAtYLong(void);
void abi_ActorEndPlaced(void);
void abi_Kind86WaitWork(void);
void abi_Kind86ChildAbove(void);
void abi_Kind86OutOfView(void);

#define K86_HOLD_AT 0x13u       /* ActorHoldPlayer's offset for the player */
#define K86_LEFT 0x4Au          /* the drop's frames left (its start's count) */
#define K86_PAIR 0x4Bu          /* the placed one: its child's record (word) */
#define K86_ABOVE 0x002Bu       /* how far above it the child is kept (pixels) */
#define K86_ZONE 0x5Cu          /* its zone that hurts the player (ActorZoneHurtsPlayer's Y) */
#define K86_REACH_X 0x00E0u     /* the box round the range point it stays in: offset, span across */
#define K86_SPAN_X 0x01C0u
#define K86_REACH_Y 0x0100u     /* and up and down */
#define K86_SPAN_Y 0x0200u
#define RANGE_X 0x1A44u         /* the point actors are kept in range of (words) */
#define RANGE_Y 0x1A46u

/*
 * @routine 83:9303 ActorKind86Wait
 * WHAT IT DOES: The routine actors of kind $86 run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind86WaitWork.
 * HOW: JSL Kind86WaitWork ($84:9766), then RTS; its C makes the JSL's
 * return frame (jsl_frame).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind86WaitWork leaves them.
 */
void abi_ActorKind86Wait(void)
{
    jsl_frame(0x839303u);
    abi_Kind86WaitWork();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:9766 Kind86WaitWork
 * WHAT IT DOES: What a kind $86 pair does each frame it waits: the child
 * nothing; the placed one may hold the player, hurts him in its zone
 * (which ends its drop), animates, keeps its child $2B above it, and once
 * out of view frees the child and goes to its second state ($8C).
 * HOW: $07 bit 2 set: RTL. K86_HOLD_AT 6; ActorHoldPlayerLong;
 * ActorZoneHurtsPlayerLong with zone $5C; hurt (C clear): K86_LEFT 1.
 * AnimStep; Kind86ChildAbove; Kind86OutOfView; out (C set): 16-bit X,
 * ActorFreePlacedAtYLong with Y = K86_PAIR, X 8-bit, state bits $8C set
 * (TSB); RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSL from ActorKind86Wait. Exit: M 8-bit, X 8-bit; as the last call
 * leaves the rest.
 */
void abi_Kind86WaitWork(void)
{
    lda8(ea_dp(ACTOR_07));
    and8_v(0x04);
    if (!IF_Z)
        return;
    lda8_v(0x06);
    sta8(ea_dp(K86_HOLD_AT));
    abi_ActorHoldPlayerLong();
    ldy8_v(K86_ZONE);
    abi_ActorZoneHurtsPlayerLong();
    if (!IF_C) {
        lda8_v(0x01);
        sta8(ea_dp(K86_LEFT));
    }
    abi_AnimStep();
    call_frame(0x9786u, abi_Kind86ChildAbove);
    call_frame(0x9789u, abi_Kind86OutOfView);
    if (!IF_C)
        return;
    op_rep(0x10);
    ldy16(ea_dp(K86_PAIR));
    abi_ActorFreePlacedAtYLong();
    op_sep(0x10);
    lda8_v(0x8C);
    tsb8(ea_dp(ACTOR_STATE));
}

/*
 * @routine 84:97BB Kind86ChildAbove
 * WHAT IT DOES: Keeps a kind $86's child $2B pixels above it.
 * HOW: 16-bit: the child's (K86_PAIR's) y pixels = its own less $2B; M and
 * X 8-bit.
 * Entry: D = the placed one's record; DB a bank whose $0000-$1FFF is work
 * RAM; called by JSR. Exit: M and X 8-bit; A = the child's new y; X =
 * the child's record's low byte.
 */
void abi_Kind86ChildAbove(void)
{
    op_rep(0x30);
    ldx16(ea_dp(K86_PAIR));
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(K86_ABOVE);
    sta16(ea_absx(ACTOR_Y + 1));
    op_sep(0x30);
}

/*
 * @routine 84:97EE Kind86OutOfView
 * WHAT IT DOES: Tells whether a kind $86 is out of view: C set when it is
 * more than $E0 across or $100 up or down from the range point.
 * HOW: 16-bit: RANGE_X less its x, plus $E0, compared with $1C0; below:
 * RANGE_Y less its y, plus $100, compared with $200. M 8-bit.
 * Entry: D = the actor's record; called by JSR. Exit: M 8-bit; C set when
 * out of view; A the last sum.
 */
void abi_Kind86OutOfView(void)
{
    op_rep(0x20);
    sec();
    lda16(ea_abs(RANGE_X));
    sbc16_v(rd16(ea_dp(ACTOR_X + 1)));
    clc();
    adc16_v(K86_REACH_X);
    cmp16_v(K86_SPAN_X);
    if (!IF_C) {
        sec();
        lda16(ea_abs(RANGE_Y));
        sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));
        clc();
        adc16_v(K86_REACH_Y);
        cmp16_v(K86_SPAN_Y);
    }
    op_sep(0x20);
}

/*
 * @routine 84:980F Kind86End
 * @actor
 * WHAT IT DOES: Kind $86's second state (the placed one, out of view, its
 * child freed): it ends.
 * HOW: JML to ActorEndPlaced.
 * Entry: D = the actor's record; reached through kind $86's second-state
 * entry ($83:9308) when the actor loop finds its state $8C. Exit: never
 * returns: as ActorEndPlaced.
 */
void abi_Kind86End(void)
{
    abi_ActorEndPlaced();
}

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
#define K86_COUNT 0x40u         /* frames left of each step of its cycle */
#define K86_HOME_Y 0x43u        /* its y pixels as it started: the rise ends above it (word) */
#define K86_LAST_Y 0x47u        /* its y pixels as this frame began (word) */
#define K86_STOOD 0x49u         /* nonzero: the player stands on it this frame */
#define K86_STAND_BOX 0x24u     /* its box for ActorSolidForPlayer */
#define K86_LOOK 0x0600u        /* its sprite attributes */
#define K86_4F 0x4Fu            /* cleared as it starts; purpose not yet established */

void abi_ActorSpawnPlace(void);
void abi_ActorSpawn(void);
void abi_AnimStart(void);
void abi_ActorMotionReset(void);
void abi_ActorFall(void);
void abi_SoundQueueAdd(void);
void abi_ActorSpeedYSet(void);
void abi_ActorStepYBack(void);
void abi_ActorSolidForPlayerLong(void);
void abi_Kind86HeightKeep(void);
void abi_Kind86Carry(void);
void abi_Kind86StandCheck(void);

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* A frame of its cycle: the yield (BRK at brk_at), the height kept, the
   stand check; JSRs at brk_at + 2 and + 5. */
static void k86_turn(uint32_t brk_at)
{
    actor_yield(brk_at);
    call_frame((uint16_t)(brk_at + 4), abi_Kind86HeightKeep);
    call_frame((uint16_t)(brk_at + 7), abi_Kind86StandCheck);
}

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
    jsl_frame(0x849770u);
    abi_ActorHoldPlayerLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    ldy8_v(K86_ZONE);
    jsl_frame(0x849776u);
    abi_ActorZoneHurtsPlayerLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (!IF_C) {
        lda8_v(0x01);
        sta8(ea_dp(K86_LEFT));
    }
    jsl_frame(0x849780u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0x9786u, abi_Kind86ChildAbove);
    call_frame(0x9789u, abi_Kind86OutOfView);
    if (!IF_C)
        return;
    op_rep(0x10);
    ldy16(ea_dp(K86_PAIR));
    jsl_frame(0x849790u);
    abi_ActorFreePlacedAtYLong();
    cpu.s = (uint16_t)(cpu.s + 3);
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

/*
 * @routine 84:962D Kind86Start
 * @actor
 * WHAT IT DOES: Kind $86's start: the placed one ($07 bit 2 clear) makes
 * its child ($07 bit 2 set, kept $2B above it), then forever: a hold of
 * 16 frames, a drop ($20 frames, $14 with $07 bits 0-1 set) with sound
 * $39 as it ends, a short bounce (a pixel up and back), $40 frames still,
 * then the rise (sound $38) until it is back above where it started.
 * The player standing on it is carried with it. The child only animates.
 * HOW: Look $0600, $08 = 2, $09 = $90, $12 = 8. Child ($07 bit 2):
 * AnimStart $0F, then a yield each frame for ever. Placed: K86_4F and
 * its spawn offset cleared; ActorSpawnPlace, ActorSpawn (N clear: the child's kind and $07
 * with bit 2 set, its facing cleared, its look; K86_PAIR = it); a yield;
 * AnimStart $0E; K86_HOME_Y = its y. The cycle: each frame a yield,
 * Kind86HeightKeep and Kind86StandCheck, then: 16 frames; ActorMotionReset
 * $28 and K86_LEFT frames of ActorFall and Kind86Carry; SoundQueueAdd $39;
 * 2 frames, a frame with y up one, 2 frames, a frame with y back, $40
 * frames (each with Kind86Carry); SoundQueueAdd $38, ActorSpeedYSet $7B,
 * and ActorStepYBack with Kind86Carry each frame while its y is at or
 * below K86_HOME_Y.
 * Entry: D = the actor's record; M 8-bit, X 8-bit; DB = $80; reached by
 * JML from kind $86's start entry ($83:92FF). Exit: never returns.
 * (The ten bytes at $84:968E-9697, after the BRA at $84:968C, are reached
 * by nothing: no branch or jump in the routine targets them, and no JML,
 * long pointer or table in the ROM names them.)
 */
void abi_Kind86Start(void)
{
    op_rep(0x20);
    lda16_v(K86_LOOK);
    sta16(ea_dp(ACTOR_LOOK));
    op_sep(0x20);
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x08);
    sta8(ea_dp(ACTOR_12));
    lda8(ea_dp(ACTOR_07));
    and8_v(0x04);
    if (!IF_Z) {
        /* the child: it only animates */
        lda8_v(0x0F);
        jsl_call(0x84969Au, abi_AnimStart);
        for (;;)
            actor_yield(0x849762u);
    }
    stz8(ea_dp(K86_4F));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    jsl_call(0x84964Cu, abi_ActorSpawnPlace);
    jsl_call(0x849650u, abi_ActorSpawn);
    if (!IF_N) {
        /* the child: this kind, $07 with bit 2 set, still, our look */
        lda8(ea_dp(ACTOR_06));
        sta8(ea_absx(ACTOR_06));
        lda8(ea_dp(ACTOR_07));
        and8_v(0xFB);
        ora8_v(0x04);
        sta8(ea_absx(ACTOR_07));
        stz8(ea_absx(ACTOR_FACING));
        stz8(ea_absx(ACTOR_FLIP));
        lda8(ea_dp(ACTOR_LOOK));
        sta8(ea_absx(ACTOR_LOOK));
        lda8(ea_dp(ACTOR_LOOK + 1));
        sta8(ea_absx(ACTOR_LOOK + 1));
        op_rep(0x20);
        op_sep(0x20);
        stx16(ea_dp(K86_PAIR));             /* X is 16-bit here (the SEP #$10 follows): both bytes */
    }
    op_sep(0x10);
    actor_yield(0x84967Cu);
    lda8_v(0x0E);
    jsl_call(0x849680u, abi_AnimStart);
    op_rep(0x20);
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_dp(K86_HOME_Y));
    op_sep(0x20);
    for (;;) {                                   /* each cycle */
        lda8_v(0x10);
        sta8(ea_dp(K86_COUNT));
        call_frame(0x96A7, abi_Kind86HeightKeep);
        do {                                     /* the hold */
            actor_yield(0x8496A8u);
            call_frame(0x96AC, abi_Kind86StandCheck);
            MEM8(dec, ea_dp(K86_COUNT));
        } while (!IF_Z);
        ldx8_v(0x20);
        lda8(ea_dp(ACTOR_07));
        and8_v(0x03);
        if (!IF_Z)
            ldx8_v(0x14);
        stx8(ea_dp(K86_LEFT));
        lda8_v(0x28);
        jsl_call(0x8496BFu, abi_ActorMotionReset);
        do {                                     /* the drop */
            k86_turn(0x8496C3u);
            jsl_call(0x8496CBu, abi_ActorFall);
            call_frame(0x96D1, abi_Kind86Carry);
            MEM8(dec, ea_dp(K86_LEFT));
        } while (!IF_Z);
        lda8_v(0x39);
        jsl_call(0x8496D8u, abi_SoundQueueAdd);
        lda8_v(0x02);
        sta8(ea_dp(K86_COUNT));
        do {
            k86_turn(0x8496E0u);
            call_frame(0x96EA, abi_Kind86Carry);
            MEM8(dec, ea_dp(K86_COUNT));
        } while (!IF_Z);
        k86_turn(0x8496EFu);                     /* a pixel up */
        op_rep(0x20);
        MEM16(dec, ea_dp(ACTOR_Y + 1));
        op_sep(0x20);
        call_frame(0x96FF, abi_Kind86Carry);
        lda8_v(0x02);
        sta8(ea_dp(K86_COUNT));
        do {
            k86_turn(0x849704u);
            call_frame(0x970E, abi_Kind86Carry);
            MEM8(dec, ea_dp(K86_COUNT));
        } while (!IF_Z);
        k86_turn(0x849713u);                     /* and back */
        op_rep(0x20);
        MEM16(inc, ea_dp(ACTOR_Y + 1));
        op_sep(0x20);
        call_frame(0x9723, abi_Kind86Carry);
        lda8_v(0x40);
        sta8(ea_dp(K86_COUNT));
        call_frame(0x972A, abi_Kind86HeightKeep);
        do {                                     /* still */
            k86_turn(0x84972Bu);
            call_frame(0x9735, abi_Kind86Carry);
            MEM8(dec, ea_dp(K86_COUNT));
        } while (!IF_Z);
        lda8_v(0x38);
        jsl_call(0x84973Cu, abi_SoundQueueAdd);
        ldy8_v(0x7B);
        jsl_call(0x849742u, abi_ActorSpeedYSet);
        do {                                     /* the rise */
            k86_turn(0x849746u);
            jsl_call(0x84974Eu, abi_ActorStepYBack);
            call_frame(0x9754, abi_Kind86Carry);
            op_rep(0x20);
            lda16(ea_dp(ACTOR_Y + 1));
            cmp16_v(rd16(ea_dp(K86_HOME_Y)));
            op_sep(0x20);
        } while (IF_C);
    }
}

/*
 * @routine 84:979B Kind86HeightKeep
 * WHAT IT DOES: Keeps a kind $86's height as the frame begins, and
 * clears its "stood on".
 * HOW: K86_LAST_Y = its y pixels; K86_STOOD = 0.
 * Entry: D = the actor's record; M 8-bit; called by JSR. Exit: M 8-bit.
 */
void abi_Kind86HeightKeep(void)
{
    op_rep(0x20);
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_dp(K86_LAST_Y));
    op_sep(0x20);
    stz8(ea_dp(K86_STOOD));
}

/*
 * @routine 84:97A6 Kind86Carry
 * WHAT IT DOES: Carries the player standing on a kind $86 with it.
 * HOW: K86_STOOD set: the player's y pixels plus its y less K86_LAST_Y.
 * Entry: D = the actor's record; M 8-bit; called by JSR. Exit: M 8-bit;
 * not stood on: Z set.
 */
void abi_Kind86Carry(void)
{
    lda8(ea_dp(K86_STOOD));
    if (IF_Z)
        return;
    op_rep(0x20);
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_dp(K86_LAST_Y)));
    clc();
    adc16_v(rd16(ea_abs(PLAYER + ACTOR_Y + 1)));
    sta16(ea_abs(PLAYER + ACTOR_Y + 1));
    op_sep(0x20);
}

/*
 * @routine 84:97CB Kind86StandCheck
 * WHAT IT DOES: Lets the player stand on a kind $86 (4 pixels above its
 * place), and marks it stood on when he does.
 * HOW: Its y pixels less 4; ActorSolidForPlayerLong with box $24; C
 * clear: K86_STOOD counted up; its y back.
 * Entry: D = the actor's record; M 8-bit, X 8-bit; called by JSR. Exit:
 * M 8-bit; Y = $24; A its y pixels.
 */
void abi_Kind86StandCheck(void)
{
    op_rep(0x20);
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(0x0004);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    ldy8_v(K86_STAND_BOX);
    jsl_call(0x8497D9u, abi_ActorSolidForPlayerLong);
    if (!IF_C)
        MEM8(inc, ea_dp(K86_STOOD));
    op_rep(0x20);
    clc();
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(0x0004);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
}

/*
 * @routine 84:968E Kind86Unused
 * @actor
 * WHAT IT DOES: Sets the actor's state bits $8C and waits for good. Code
 * that nothing reaches (2026-11-18): it follows Kind86Start's BRA at
 * 84:968C, and no JMP, JSR, JML, JSL, branch, or word $968E or $968D (a
 * table entry or a pushed return address) in the ROM leads to it; no
 * script's trace runs it. Filed with C as all code is.
 * HOW: X 8-bit; state bits $8C set (TSB); each frame (BRK) nothing.
 * Entry: D = the actor's record. Exit: never returns.
 */
void abi_Kind86Unused(void)
{
    op_sep(0x10);
    lda8_v(0x8C);
    tsb8(ea_dp(ACTOR_STATE));
    for (;;)
        actor_yield(0x849694u);
}

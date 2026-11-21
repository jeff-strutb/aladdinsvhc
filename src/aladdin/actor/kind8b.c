/*
 * kind8b.c: actor kind $8B, a cloud that rises as kind $5D's does, then bobs, takes hold of Aladdin
 * when he is within reach, lifts him and lets him go; ended once its maker has gone. Its wait is
 * Kind8BWaitWork.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorGrabbableByPlayerLong(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_Kind5DParentGoneLong(void);
void abi_Kind5DRise(void);
void abi_Kind8BGrab(void);
void abi_Kind8BStep(void);

#define K8B_PHASE 0x13u               /* Kind8BGrab's phase: 0 it waits for Aladdin, 1 it keeps him, 2 it waits to again */
#define K8B_GRABBED 0x40u             /* set once it has taken hold of Aladdin */
#define K8B_TIMER 0x41u               /* frames before it ends after that */
#define K8B_COUNT 0x46u               /* phase 2's frames */
#define K8B_ENDS 0x47u                /* set when it is to end: Kind8BWaitWork ends the actor */
#define K8B_STEP 0x48u                /* Kind8BStep's step, 0-8 */
#define K8B_Y_LIMIT 0x49u             /* the y (a word) Kind8BStep's steps move to */
#define K8B_HELD 0x1427u              /* nonzero: the player is kept on the lifting actor */
#define K8B_LET_GO 0x1458u            /* nonzero: what holds him lets him go */
#define K8B_HELD_RESUME 0xD6F8u       /* the player's code resumed while he is held */
#define K8B_SAVED_X 0x145Du           /* a position kept for the player: x, then y at +2 */
#define K8B_SAVED_Y 0x145Fu
#define PLAYER_RESUME 0x08DEu         /* where the player's code resumes (word) */
#define PLAYER_X 0x08E7u              /* the player's x pixels (word) */
#define PLAYER_Y 0x08EAu              /* and y pixels (word) */
#define K8B_STEPS 0x9881u             /* (bank $84) the steps' entries by K8B_STEP (words) */
#define K8B_PHASES 0x9906u            /* (bank $84) the phases' entries by K8B_PHASE (words) */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:9813 Kind8BStart
 * @actor
 * WHAT IT DOES: Actor kind $8B (placed once in levels $08 and $18, at x
 * 16383: made by other code): a cloud that rises as kind $5D's does
 * (Kind5DRise), then, kept by its wait (Kind8BWaitWork), bobs and takes
 * hold of Aladdin when he is within reach, lifts and lets him go; it
 * ends once its maker has gone.
 * HOW: Kind5DRise; $08 = 2, $09 = $80 (its kind's wait runs), the
 * palette byte ($2C) = 6; AnimStart $10; K8B_STEP = 1; K8B_Y_LIMIT (the
 * word $49) = its y pixels ($0F). Each frame (BRK)
 * Kind5DParentGoneLong: gone, to the end; else K8B_GRABBED ($40) clear:
 * on; set: K8B_TIMER ($41) = $FF and each frame (BRK) the same test,
 * K8B_TIMER down to 0. The end: $08 bit 4 set, a COP wait of $57
 * frames, K8B_ENDS ($47) up (the wait routine then ends the actor),
 * then BRK for ever.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML
 * of kind $8B's stub (Kind8BEnter, $83:932A) from the actor loop in the
 * start state.
 * Exit: never returns: ended by Kind8BWaitWork.
 */
void abi_Kind8BStart(void)
{
    jsl_call(0x849813u, abi_Kind5DRise);
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x06);
    sta8(ea_dp(ACTOR_LOOK + 1));
    lda8_v(0x10);
    jsl_call(0x849825u, abi_AnimStart);
    lda8_v(0x01);
    sta8(ea_dp(K8B_STEP));
    lda8(ea_dp(ACTOR_Y + 1));
    sta8(ea_dp(K8B_Y_LIMIT));
    lda8(ea_dp(ACTOR_Y + 2));
    sta8(ea_dp(K8B_Y_LIMIT + 1));
L_849835:
    actor_yield(0x849835u);
    jsl_call(0x849837u, abi_Kind5DParentGoneLong);
    if (!IF_Z)
        goto L_849851;
    lda8(ea_dp(K8B_GRABBED));
    if (IF_Z)
        goto L_849835;
    lda8_v(0xFF);
    sta8(ea_dp(K8B_TIMER));
L_849845:
    actor_yield(0x849845u);
    jsl_call(0x849847u, abi_Kind5DParentGoneLong);
    if (!IF_Z)
        goto L_849851;
    MEM8(dec, ea_dp(K8B_TIMER));
    if (!IF_Z)
        goto L_849845;
L_849851:
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x10);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x57);
    actor_wait(0x849859u);
    MEM8(inc, ea_dp(K8B_ENDS));
L_84985D:
    actor_yield(0x84985Du);
    goto L_84985D;
}

/*
 * @routine 84:9861 Kind8BWaitWork
 * WHAT IT DOES: What actors of kind $8B do each frame they wait:
 * Kind8BStep and Kind8BGrab, their animation steps, and once K8B_ENDS
 * is set the actor ends.
 * HOW: JSR Kind8BStep; JSR Kind8BGrab; AnimStep; K8B_ENDS ($47) zero:
 * RTL. Else the five bytes its caller (the actor loop, by the stub at
 * $83:932E) pushed pulled, and JML ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's wait stub
 * (ActorKind8BWait, $83:932E) through the actor loop's wait table.
 * Exit: K8B_ENDS clear: RTL, the actor goes on. Set: never returns, the
 * actor ended.
 */
void abi_Kind8BWaitWork(void)
{
    call_frame(0x9863u, abi_Kind8BStep);
    call_frame(0x9866u, abi_Kind8BGrab);
    jsl_call(0x849867u, abi_AnimStep);
    lda8(ea_dp(K8B_ENDS));
    if (IF_Z)
        return;
    pla8();
    pla8();
    pla8();
    pla8();
    pla8();
    actor_leave(0x8396CAu);                       /* ActorEndPlaced */
}

/*
 * @routine 84:987A Kind8BStep
 * WHAT IT DOES: One step of Kind8BWaitWork's motion: by K8B_STEP,
 * nothing (0); its y speed set to $5C (1) and then, each frame, it
 * moves against the speed until its y is above K8B_Y_LIMIT (2); the
 * speed set to $5D (3) and moving with it until its y is back at
 * K8B_Y_LIMIT (4, then step 1); to $5E (5) and moving against it until
 * its y speed is negative (6); K8B_Y_LIMIT 8 above its y and the speed
 * set to $5F (7) and moving with it until the speed is no longer
 * negative (8, then step 4, its acceleration $0A).
 * HOW: JMP through the nine words after it by K8B_STEP * 2. Steps 1, 3,
 * 5, 7: ActorMotionPreset with Y = $5C, $5D, $5E, $5F; K8B_STEP up; and
 * on into step 2, 4, 6, 8: ActorFlyAgainst (2, 6) or ActorFly (4, 8)
 * moves it by its speed, then step 2: y (16 bits) against K8B_Y_LIMIT,
 * K8B_STEP up unless it is at least; step 4: y against K8B_Y_LIMIT,
 * below it: K8B_STEP = 1; step 6: the speed's top byte ($1F) negative:
 * K8B_STEP up; step 8: the speed's top byte not negative: K8B_STEP = 4
 * and the acceleration ($20) = $0A. Step 7 first sets K8B_Y_LIMIT to y
 * less 8.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
 * Kind8BWaitWork. Exit: M and X 8-bit.
 */
void abi_Kind8BStep(void)
{
    lda8(ea_dp(K8B_STEP));
    ACC8(asl);
    tax8();
    switch (cpu.x) {                       /* JMP (K8B_STEPS,X) */
    case 0:  return;                       /* L_98AB: its RTS */
    case 2:  goto L_9893;
    case 4:  goto L_989B;
    case 6:  goto L_98AC;
    case 8:  goto L_98B4;
    case 10: goto L_98C7;
    case 12: goto L_98CF;
    case 14: goto L_98DA;
    case 16: goto L_98EE;
    default:                               /* past the table's 9 words: no step names one */
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(K8B_STEPS + cpu.x)));
        return;
    }
L_9893:
    ldy8_v(0x5C);
    jsl_call(0x849895u, abi_ActorMotionPreset);
    MEM8(inc, ea_dp(K8B_STEP));
L_989B:
    jsl_call(0x84989Bu, abi_ActorFlyAgainst);
    op_rep(0x20);
    lda16(ea_dp(ACTOR_Y + 1));
    cmp16_v(rd16(ea_dp(K8B_Y_LIMIT)));
    op_sep(0x20);
    if (IF_C)
        return;
    MEM8(inc, ea_dp(K8B_STEP));
    return;
L_98AC:
    ldy8_v(0x5D);
    jsl_call(0x8498AEu, abi_ActorMotionPreset);
    MEM8(inc, ea_dp(K8B_STEP));
L_98B4:
    jsl_call(0x8498B4u, abi_ActorFly);
    op_rep(0x20);
    lda16(ea_dp(ACTOR_Y + 1));
    cmp16_v(rd16(ea_dp(K8B_Y_LIMIT)));
    op_sep(0x20);
    if (!IF_C)
        return;
    lda8_v(0x01);
    sta8(ea_dp(K8B_STEP));
    return;
L_98C7:
    ldy8_v(0x5E);
    jsl_call(0x8498C9u, abi_ActorMotionPreset);
    MEM8(inc, ea_dp(K8B_STEP));
L_98CF:
    jsl_call(0x8498CFu, abi_ActorFlyAgainst);
    lda8(ea_dp(ACTOR_SPEED_Y + 2));
    if (!IF_N)
        return;
    MEM8(inc, ea_dp(K8B_STEP));
    return;
L_98DA:
    op_rep(0x20);
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(0x0008);
    sta16(ea_dp(K8B_Y_LIMIT));
    op_sep(0x20);
    ldy8_v(0x5F);
    jsl_call(0x8498E8u, abi_ActorMotionPreset);
    MEM8(inc, ea_dp(K8B_STEP));
L_98EE:
    jsl_call(0x8498EEu, abi_ActorFly);
    lda8(ea_dp(ACTOR_SPEED_Y + 2));
    if (IF_N)
        return;
    lda8_v(0x04);
    sta8(ea_dp(K8B_STEP));
    lda8_v(0x0A);
    sta8(ea_dp(ACTOR_ACCEL_Y));
}

/*
 * @routine 84:98FF Kind8BGrab
 * WHAT IT DOES: The other step of Kind8BWaitWork, by K8B_PHASE: 0, it
 * takes hold of Aladdin when he is within its reach (and no actor holds
 * him); 1, it keeps him at its place; 2, a wait of $10 frames before it
 * can again.
 * HOW: JMP through the three words after it by K8B_PHASE * 2. Phase 0
 * ($84:990C): $1427 nonzero, or ActorGrabbableByPlayerLong with carry
 * set: nothing. Else the player's resume address ($08DE) = $D6F8, the
 * saved position ($145D, $145F) = its x and y pixels, $1427 up,
 * K8B_GRABBED = 1, AnimStart $15, K8B_STEP = 5, K8B_PHASE up; with
 * K8B_ENDS nonzero, $1458 (let go) up. Phase 1 ($84:9946): $1427
 * nonzero: the player's x ($08E7) = its x and his y ($08EA) = its y +
 * 7, and with K8B_ENDS zero RTS there, else $1458 up; then (and at once
 * with $1427 zero) K8B_PHASE up, K8B_COUNT = $10, AnimStart $16. Phase
 * 2 ($84:9972): K8B_COUNT down; at 0, K8B_PHASE = 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
 * Kind8BWaitWork. Exit: M and X 8-bit.
 */
void abi_Kind8BGrab(void)
{
    lda8(ea_dp(K8B_PHASE));
    ACC8(asl);
    tax8();
    switch (cpu.x) {                       /* JMP (K8B_PHASES,X) */
    case 0: goto L_990C;
    case 2: goto L_9946;
    case 4: goto L_9972;
    default:                               /* past the table's 3 words: no phase names one */
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(K8B_PHASES + cpu.x)));
        return;
    }
L_990C:
    lda8(ea_abs(K8B_HELD));
    if (!IF_Z)
        return;
    jsl_call(0x849911u, abi_ActorGrabbableByPlayerLong);
    if (IF_C)
        return;
    op_rep(0x20);
    lda16_v(K8B_HELD_RESUME);
    sta16(ea_abs(PLAYER_RESUME));
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(K8B_SAVED_X));
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_abs(K8B_SAVED_Y));
    op_sep(0x20);
    MEM8(inc, ea_abs(K8B_HELD));
    lda8_v(0x01);
    sta8(ea_dp(K8B_GRABBED));
    lda8_v(0x15);
    jsl_call(0x849934u, abi_AnimStart);
    lda8_v(0x05);
    sta8(ea_dp(K8B_STEP));
    MEM8(inc, ea_dp(K8B_PHASE));
    lda8(ea_dp(K8B_ENDS));
    if (IF_Z)
        return;
    MEM8(inc, ea_abs(K8B_LET_GO));
    return;
L_9946:
    lda8(ea_abs(K8B_HELD));
    if (IF_Z)
        goto L_9965;
    op_rep(0x20);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(PLAYER_X));
    clc();
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(0x0007);
    sta16(ea_abs(PLAYER_Y));
    op_sep(0x20);
    lda8(ea_dp(K8B_ENDS));
    if (IF_Z)
        return;
    MEM8(inc, ea_abs(K8B_LET_GO));
L_9965:
    MEM8(inc, ea_dp(K8B_PHASE));
    lda8_v(0x10);
    sta8(ea_dp(K8B_COUNT));
    lda8_v(0x16);
    jsl_call(0x84996Du, abi_AnimStart);
    return;
L_9972:
    MEM8(dec, ea_dp(K8B_COUNT));
    if (!IF_Z)
        return;
    stz8(ea_dp(K8B_PHASE));
}

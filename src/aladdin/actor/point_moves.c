/*
 * point_moves.c: actor helpers that move an actor to a point over frames,
 * keeping their caller's return address while they wait (so the actor's
 * stack is as the actor loop expects at each BRK), and kind $94's
 * per-frame work.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"

void abi_ActorAimAt(void);
void abi_ActorMoveByVector(void);
void abi_ActorNearPoint(void);
void abi_AnimStep(void);
void abi_PlayerInReach(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);

#define GOTO_X 0x48u            /* the point it goes to (words) */
#define GOTO_Y 0x4Au
#define GOTO_SPEED 0x4Cu        /* its direction set's byte offset (ActorMoveByVector's X) */
#define GOTO_BACK 0x4Eu         /* the caller's return address, kept while it waits (word) */
#define AIM_X (SCRATCH + 4)     /* ActorAimAt's point (words) */
#define AIM_Y (SCRATCH + 6)
#define NEAR_X (SCRATCH + 8)    /* ActorNearPoint's point (words) */
#define NEAR_Y (SCRATCH + 10)
#define REACH_TABLE 0xD9B0u     /* 8 bytes an entry: x and y reach, rise speed, step, limit */
#define RISE_SPEED 0x1Du        /* the rise's speed, 16ths (word; $1F its third byte) */
#define RISE_STEP 0x20u         /* added to the speed each frame */
#define RISE_LIMIT 0x45u        /* the speed's top (word) */

/*
 * @routine 83:FC5D ActorGoToPoint
 * @actor
 * WHAT IT DOES: Moves the actor toward the point (GOTO_X, GOTO_Y) a step
 * a frame, facing the way it goes, until it is within box $20 of it; then
 * back to its caller.
 * HOW: Its return address pulled into GOTO_BACK. Each frame: the point to
 * scratch $10/$12, ActorAimAt (16-bit), the direction to the facing,
 * ActorMoveByVector with set GOTO_SPEED; BRK; the point to scratch
 * $14/$16, ActorNearPoint with box $20; outside (C set), again. GOTO_BACK
 * pushed back, M and X 8-bit, RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; called by
 * JSR (from $83:AE90 and $83:AECF). Exit: M and X 8-bit; A = GOTO_BACK; C
 * clear; Y as ActorNearPoint leaves it.
 */
void abi_ActorGoToPoint(void)
{
    uint16_t back;

    op_rep(0x20);
    back = pull16();
    a16_load(back);
    dp16_set(GOTO_BACK, back);
    do {
        op_rep(0x10);
        a16_load(dp16(GOTO_X));
        wr16(dba(AIM_X), cpu.a);
        a16_load(dp16(GOTO_Y));
        wr16(dba(AIM_Y), cpu.a);
        abi_ActorAimAt();
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
        x8_load(rd8(dpa(GOTO_SPEED)));
        abi_ActorMoveByVector();
        actor_yield(0x83FC7Au);
        op_rep(0x20);
        a16_load(dp16(GOTO_X));
        wr16(dba(NEAR_X), cpu.a);
        a16_load(dp16(GOTO_Y));
        wr16(dba(NEAR_Y), cpu.a);
        y8_load(0x20);
        abi_ActorNearPoint();
    } while (cpu.p & P_C);
    a16_load(dp16(GOTO_BACK));
    push16(cpu.a);
    op_sep(0x30);
}


/*
 * @routine 83:FB15 Kind94Wait
 * WHAT IT DOES: What actors of kind $94 do each frame they wait: animate.
 * HOW: AnimStep, RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSL from the kind's
 * stub ($83:939A) through the actor loop's wait table ($83:904F). Exit:
 * as AnimStep.
 */
void abi_Kind94Wait(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:FB4F ActorBackToCaller
 * @pushes_return
 * WHAT IT DOES: The end of a helper that kept its caller's return
 * address at GOTO_BACK while it waited: puts it back and returns there.
 * HOW: GOTO_BACK (16-bit) pushed; M 8-bit; RTS.
 * Entry: X 8-bit; D = the actor's record; reached by JMP ($83:FCEF).
 * Exit: M 8-bit; A = GOTO_BACK; N and Z from it.
 */
void abi_ActorBackToCaller(void)
{
    op_rep(0x20);
    a16_load(dp16(GOTO_BACK));
    push16(cpu.a);
    op_sep(0x20);
}

/*
 * @routine 83:FCA5 ActorRiseWhenInReach
 * @actor
 * WHAT IT DOES: When Aladdin is in reach by entry X of the reach table,
 * the actor rises a frame at a time, faster each frame up to a limit,
 * until it is out of the ground; then back to its caller with C clear.
 * Out of reach: C set at once.
 * HOW: PlayerInReach; C set: M 8-bit, RTS. Else its return address
 * pulled into GOTO_BACK; RISE_LIMIT = the entry's +4 byte * 16, RISE_SPEED
 * = its +2 byte * 16 (third byte 0), RISE_STEP = its +3 byte. Each frame:
 * BRK; RISE_SPEED + RISE_STEP, at most RISE_LIMIT; ActorMoveY;
 * ActorOutOfGround, again while Z. C clear, on into ActorBackToCaller.
 * Entry: M 8-bit, X 8-bit; X = the reach entry's offset (8 bytes an
 * entry); D = the actor's record; DB = $80; called by JSR ($83:AC54,
 * $83:AED6). Exit: in reach: as ActorBackToCaller, C clear; else M 8-bit,
 * C set, as PlayerInReach.
 */
void abi_ActorRiseWhenInReach(void)
{
    abi_PlayerInReach();
    if (IF_C) {
        op_sep(0x20);
        return;
    }
    pla16();
    sta16(ea_dp(GOTO_BACK));
    lda16(ea_absx(REACH_TABLE + 4));
    and16_v(0x00FF);
    ACC16(asl);
    ACC16(asl);
    ACC16(asl);
    ACC16(asl);
    sta16(ea_dp(RISE_LIMIT));
    lda16(ea_absx(REACH_TABLE + 2));
    and16_v(0x00FF);
    ACC16(asl);
    ACC16(asl);
    ACC16(asl);
    ACC16(asl);
    sta16(ea_dp(RISE_SPEED));
    op_sep(0x20);
    stz8(ea_dp(RISE_SPEED + 2));
    lda8(ea_absx(REACH_TABLE + 3));
    sta8(ea_dp(RISE_STEP));
    do {
        actor_yield(0x83FCCEu);
        op_rep(0x20);
        clc();
        lda16(ea_dp(RISE_STEP));
        and16_v(0x00FF);
        adc16_v(rd16(ea_dp(RISE_SPEED)));
        cmp16_v(rd16(ea_dp(RISE_LIMIT)));
        if (IF_C)
            lda16(ea_dp(RISE_LIMIT));   /* at the limit or past it: the limit */
        sta16(ea_dp(RISE_SPEED));
        op_sep(0x20);
        abi_ActorMoveY();
        abi_ActorOutOfGround();
    } while (IF_Z);
    clc();
    abi_ActorBackToCaller();
}

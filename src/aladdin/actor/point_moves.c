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
void abi_ActorFall(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSpawnSameLook(void);
void abi_PointInGroundLong(void);

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
#define SPAWN_LOOK 0x4Du        /* the look ActorSpawnLookWait spawns */
#define HOP_TABLE 0xD9BFu       /* 5 bytes an entry: $20, vertical speed (2), horizontal speed (2) */
#define PROBE_X SCRATCH         /* PointInGroundLong's point (words) */
#define PROBE_Y (SCRATCH + 2)
#define PROBE_FLAGS (SCRATCH + 6) /* its result: bits 0-2 set in the ground */

/* A JSL at `site` to a routine whose C uses the stack: its return address
   on the stack while the callee runs, as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

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
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
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

/*
 * @routine 83:FC0D ActorSpawnLookWait
 * @actor
 * WHAT IT DOES: Spawns an actor of the look in $4D next to the actor,
 * waiting a frame and trying again while no record is free; then back to
 * its caller.
 * HOW: Its return address pulled into GOTO_BACK ($4E); ActorSpawnSameLook
 * with A = $4D; N clear: on into ActorBackToCaller. N set (no record): BRK
 * ($83:FC1F) and again ($83:FC14); no script runs that.
 * Entry: M 8-bit; D = the actor's record; called by JSR ($83:ACAC).
 * Exit: as ActorBackToCaller.
 */
void abi_ActorSpawnLookWait(void)
{
    op_rep(0x20);
    pla16();
    sta16(ea_dp(GOTO_BACK));
    op_sep(0x20);
    for (;;) {
        lda8(ea_dp(SPAWN_LOOK));
        abi_ActorSpawnSameLook();
        if (!IF_N)
            break;
        actor_yield(0x83FC1Fu);         /* no record free: a frame, again */
    }
    abi_ActorBackToCaller();
}

/*
 * @routine 83:FD2C ActorHopFromTable
 * @actor
 * WHAT IT DOES: A hop by entry X of the hop table at $83:D9BF: the actor
 * moves and falls each frame until it is back on the ground; then back to
 * its caller.
 * HOW: Its return address pulled into GOTO_BACK ($4E); from the entry: $20,
 * the vertical speed $1D-$1E ($1F $FF: up), the horizontal speed $19-$1A
 * ($1B 0). Each frame: BRK; ActorMoveX, ActorFall, ActorOutOfWallProbe4;
 * again while $1F is negative or ActorOutOfGround says Z. Then on into
 * ActorBackToCaller.
 * Entry: M 8-bit, X 8-bit; X = the entry's offset; D = the actor's record;
 * DB = $80; called by JSR. Exit: as ActorBackToCaller.
 */
void abi_ActorHopFromTable(void)
{
    op_rep(0x20);
    pla16();
    sta16(ea_dp(GOTO_BACK));
    op_sep(0x20);
    lda8(ea_absx(HOP_TABLE));
    sta8(ea_dp(0x20));
    lda8(ea_absx(HOP_TABLE + 1));
    sta8(ea_dp(0x1D));
    lda8(ea_absx(HOP_TABLE + 2));
    sta8(ea_dp(0x1E));
    lda8_v(0xFF);
    sta8(ea_dp(0x1F));
    lda8(ea_absx(HOP_TABLE + 3));
    sta8(ea_dp(0x19));
    lda8(ea_absx(HOP_TABLE + 4));
    sta8(ea_dp(0x1A));
    stz8(ea_dp(0x1B));
    do {
        do {
            actor_yield(0x83FD52u);
            abi_ActorMoveX();
            abi_ActorFall();
            jsl_call(0x83FD5Cu, abi_ActorOutOfWallProbe4);
            lda8(ea_dp(0x1F));
        } while (IF_N);
        jsl_call(0x83FD64u, abi_ActorOutOfGround);
    } while (IF_Z);
    abi_ActorBackToCaller();
}

/*
 * @routine 83:FD6D ActorProbeAheadInGround
 * WHAT IT DOES: Tests two points for ground: X ahead of the actor (by its
 * facing) and Y above it, then Y above its own x: C set when either is in
 * the ground.
 * HOW: 16-bit: X negated when $14 is set; scratch $0C = it + the actor's x,
 * $0E = its y - Y; PointInGroundLong (P kept); $0012 AND 7 nonzero: C set
 * ($83:FDAE, no script runs that). Then $0C = its x; again. Both clear: M
 * 8-bit, C clear.
 * Entry: M 8-bit, X 8-bit; X, Y the offsets; D = the actor's record;
 * called by JSR ($83:ADF5). Exit: M 8-bit; C as above.
 */
void abi_ActorProbeAheadInGround(void)
{
    op_rep(0x20);
    txa16();
    ldx8(ea_dp(ACTOR_FACING));
    if (!IF_Z) {
        eor16_v(0xFFFF);
        ACC16(inc);
    }
    clc();
    adc16_v(rd16(ea_dp(0x0C)));
    sta16(ea_abs(PROBE_X));
    tya16();
    eor16_v(0xFFFF);
    ACC16(inc);
    clc();
    adc16_v(rd16(ea_dp(0x0F)));
    sta16(ea_abs(PROBE_Y));
    php();
    jsl_call(0x83FD8Au, abi_PointInGroundLong);
    plp();
    lda16(ea_abs(PROBE_FLAGS));
    bit16_i(0x0007);
    if (!IF_Z) goto ground;
    lda16(ea_dp(0x0C));
    sta16(ea_abs(PROBE_X));
    php();
    jsl_call(0x83FD9Du, abi_PointInGroundLong);
    plp();
    lda16(ea_abs(PROBE_FLAGS));
    bit16_i(0x0007);
    if (!IF_Z) goto ground;
    op_sep(0x20);
    clc();
    return;
ground:
    op_sep(0x20);
    sec();
}

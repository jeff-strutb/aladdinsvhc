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

#define GOTO_X 0x48u            /* the point it goes to (words) */
#define GOTO_Y 0x4Au
#define GOTO_SPEED 0x4Cu        /* its direction set's byte offset (ActorMoveByVector's X) */
#define GOTO_BACK 0x4Eu         /* the caller's return address, kept while it waits (word) */
#define AIM_X (SCRATCH + 4)     /* ActorAimAt's point (words) */
#define AIM_Y (SCRATCH + 6)
#define NEAR_X (SCRATCH + 8)    /* ActorNearPoint's point (words) */
#define NEAR_Y (SCRATCH + 10)

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

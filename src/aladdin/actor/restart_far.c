/*
 * restart_far.c: the escort waiting at a spot until Aladdin comes near,
 * then following him; sent back to its restart point ($4E) once he is far
 * to its left.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorPlaceAtSpot(void);
void abi_ActorNearPlayerX(void);
void abi_ActorSpeedXFromA5(void);
void abi_ActorAnimChange(void);
void abi_ActorMoveX(void);
void abi_ActorSnapToGround(void);
void abi_ActorMotionPreset(void);
void abi_ActorAnimRiseFall(void);
void abi_ActorFly(void);
void abi_ActorOutOfGround(void);
void abi_EscortStaySpot18(void);
void abi_EscortWaitSpot4(void);

#define ACTOR_RESTART 0x4Eu                 /* where its code starts again (word) */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)  /* the player's x, whole pixels (word) */
#define FAR_LEFT 0x00C0u                    /* how far left of the actor the player must be */
#define ESCORT_RESTART_A 0xEF2Bu            /* EscortWaitSpot0's restart point: EscortWaitSpot4 */
#define ESCORT_RESTART_C 0xEF0Cu            /* EscortWaitSpot0C's: EscortStaySpot18 */
#define ESCORT_RESTART_10 0xEFC5u           /* EscortWaitSpot10's: its wait at spot $14 */

static void escort_follow(void);
static void escort_spot10_restart(void);

/* ActorRestartIfPlayerFarLeft's work, run with its JSR's return on the
   stack: nonzero when the player is far left, that return replaced by the
   restart point less one (the caller's RTS then lands on it). */
/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

static int restart_far(void)
{
    uint16_t sum, ax;

    op_rep(0x20);
    sum = op_adc16(rd16(dba((uint16_t)PLAYER_X_AT)), FAR_LEFT, &cpu.p);
    cpu.a = sum;
    ax = dp16(ACTOR_X + 1);                      /* one read: its CMP */
    cmp16_of(sum, ax);
    if (sum < ax) {
        a16_load(pull16());                      /* PLA: its return */
        a16_load(dp16(ACTOR_RESTART));
        a16_load((uint16_t)(cpu.a - 1));
        push16(cpu.a);                           /* PHA: the restart point less one */
        op_sep(0x20);
        return 1;
    }
    op_sep(0x20);
    return 0;
}

/* Goes on where the escort restarts: the place its RTS landed. */
static void escort_restart_at(uint16_t at)
{
    if (at == ESCORT_RESTART_10)
        escort_spot10_restart();
    else if (at == ESCORT_RESTART_C)
        abi_EscortStaySpot18();
    else if (at == ESCORT_RESTART_A)
        abi_EscortWaitSpot4();
    else
        jump_routine(0xA50000u | at);
}

/* A JSR ActorRestartIfPlayerFarLeft at `site` in the follow loop: its
   return pushed, the work, and its RTS: back here, or on at the restart
   point. */
static void restart_check(uint16_t site)
{
    push16((uint16_t)(site + 2));
    if (restart_far()) {
        uint16_t back = pull16();

        escort_restart_at((uint16_t)(back + 1));
    }
    cpu.s = (uint16_t)(cpu.s + 2);
}

/*
 * @routine A5:EF83 ActorRestartIfPlayerFarLeft
 * WHAT IT DOES: When the player is more than 192 pixels left of the actor,
 * returns to the actor's restart point ($4E) instead of to the caller.
 * HOW: The player's x + $C0 (+ C at entry) against the actor's x: lower,
 * the return address pulled and $4E - 1 pushed in its place, so the RTS
 * goes to $4E.
 * Entry: C added in; D = the actor; called by JSR. Exit: M 8-bit; C
 * clear when restarting, A = $4E - 1; else C set, A = the sum.
 */
void abi_ActorRestartIfPlayerFarLeft(void)
{
    if (restart_far()) {
        uint16_t back = pull16();                /* its RTS, to the restart point */

        cpu.pc = 0xA50000u | (uint16_t)(back + 1);
        cpu.unwound = 1;
    }
}

/*
 * @routine A5:EF0C EscortStaySpot18
 * @actor
 * WHAT IT DOES: The escort stands at spot $18 for good.
 * HOW: ActorPlaceAtSpot with X = $18, then BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
 * through the escort's table at $A5:E56A, or as EscortWaitSpot0C's
 * restart point. Exit: never returns.
 */
void abi_EscortStaySpot18(void)
{
    x8_load(0x18);
    call_frame(0xEF10, abi_ActorPlaceAtSpot);
    for (;;)
        actor_yield(0xA5EF11u);
}

/* The three waits at a spot: flipped, first pose, placed, the restart
   point in A (its high byte XBA'd in), then the follow. */
static void escort_wait_spot(uint8_t spot, uint16_t restart, uint32_t anim_at, uint16_t place_ret)
{
    a8_load(0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    a8_load(0x00);
    jsl_call(anim_at, abi_AnimStart);
    x8_load(spot);
    call_frame(place_ret, abi_ActorPlaceAtSpot);
    a8_load((uint8_t)(restart >> 8));
    cpu.a = (uint16_t)(cpu.a << 8 | cpu.a >> 8);  /* XBA */
    nz8_of((uint8_t)cpu.a);
    a8_load((uint8_t)restart);
    escort_follow();
}

/*
 * @routine A5:EF15 EscortWaitSpot0
 * @actor
 * WHAT IT DOES: The escort, flipped, in its first pose at spot 0, then
 * following Aladdin, restarting at EscortWaitSpot4.
 * HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = 0; A = the restart
 * point (XBA'd in); on into EscortFollow.
 * Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
 * through the escort's table at $A5:E56A. Exit: never returns.
 */
void abi_EscortWaitSpot0(void)
{
    escort_wait_spot(0x00, ESCORT_RESTART_A, 0xA5EF1Bu, 0xEF23);
}

/*
 * @routine A5:EF2B EscortWaitSpot4
 * @actor
 * WHAT IT DOES: The escort, flipped, in its first pose at spot 4, then
 * following Aladdin, restarting at EscortStaySpot18: EscortWaitSpot0's
 * restart point, where it goes once he is far to its left.
 * HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = 4; A = the restart
 * point, EscortStaySpot18 (XBA'd in); on into EscortFollow.
 * Entry: M 8-bit, X 8-bit; D = the escort's record; reached by
 * ActorRestartIfPlayerFarLeft's RTS to the restart point EscortWaitSpot0
 * set. Exit: never returns.
 */
void abi_EscortWaitSpot4(void)
{
    escort_wait_spot(0x04, ESCORT_RESTART_C, 0xA5EF31u, 0xEF39);
}

/*
 * @routine A5:EF97 EscortWaitSpot0C
 * @actor
 * WHAT IT DOES: The escort, flipped, in its first pose at spot $0C, then
 * following Aladdin, restarting at EscortStaySpot18.
 * HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = $0C; A = the
 * restart point; on into EscortFollow.
 * Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
 * through the escort's table at $A5:E56A. Exit: never returns.
 */
void abi_EscortWaitSpot0C(void)
{
    escort_wait_spot(0x0C, ESCORT_RESTART_C, 0xA5EF9Du, 0xEFA5);
}

/*
 * @routine A5:EFAE EscortWaitSpot10
 * @actor
 * WHAT IT DOES: The escort, flipped, in its first pose at spot $10, then
 * following Aladdin; sent back, it waits at spot $14 until he is near
 * (and goes on in code no script runs).
 * HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = $10; A = the
 * restart point, $EFC5; on into EscortFollow. At the restart point: flip
 * 1, AnimStart 0, ActorPlaceAtSpot with X = $14, each frame (BRK first)
 * until ActorNearPlayerX with Y = 8 clears C; what follows ($A5:EFDE) no
 * script reaches: those bytes are left in the intake.
 * Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
 * through the escort's table at $A5:E56A, and at $EFC5 by
 * ActorRestartIfPlayerFarLeft's return. Exit: never returns.
 */
void abi_EscortWaitSpot10(void)
{
    escort_wait_spot(0x10, ESCORT_RESTART_10, 0xA5EFB4u, 0xEFBC);
}

static void escort_spot10_restart(void)
{
    a8_load(0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    a8_load(0x00);
    jsl_call(0xA5EFCBu, abi_AnimStart);
    x8_load(0x14);
    call_frame(0xEFD3, abi_ActorPlaceAtSpot);
    do {
        actor_yield(0xA5EFD4u);
        y8_load(0x08);
        jsl_call(0xA5EFD8u, abi_ActorNearPlayerX);
    } while (cpu.p & P_C);
    jump_routine(0xA5EFDEu);                     /* code no script runs */
}

/*
 * @routine A5:EF3F EscortFollow
 * @actor
 * WHAT IT DOES: The escort waits until Aladdin is near, then follows him
 * on foot (speed $12, animation 2), falling off ledges and landing again;
 * whenever he is more than 192 pixels to its left it goes back to its
 * restart point (A, kept at $4E).
 * HOW: $4E word = A; each frame (BRK first) until ActorNearPlayerX with
 * Y = 8 clears C; flip 0. Walk: ActorSpeedXFromA5 Y = $12, ActorAnimChange
 * 2; each frame ActorRestartIfPlayerFarLeft, ActorMoveX, until
 * ActorSnapToGround finds no ground (zero); then ActorMotionPreset $7E and
 * each frame ActorRestartIfPlayerFarLeft, ActorAnimRiseFall, ActorFly,
 * until ActorOutOfGround is nonzero: walk again.
 * Entry: M 8-bit, X 8-bit; A = the restart point; D = the escort's
 * record; reached from EscortWaitSpot0, 0C and 10. Exit: never returns.
 */
void abi_EscortFollow(void)
{
    escort_follow();
}

static void escort_follow(void)
{
    wr8(dpa(ACTOR_RESTART), (uint8_t)cpu.a);
    cpu.a = (uint16_t)(cpu.a << 8 | cpu.a >> 8);  /* XBA */
    nz8_of((uint8_t)cpu.a);
    wr8(dpa(ACTOR_RESTART + 1), (uint8_t)cpu.a);
    do {
        actor_yield(0xA5EF44u);
        y8_load(0x08);
        jsl_call(0xA5EF48u, abi_ActorNearPlayerX);
    } while (cpu.p & P_C);
    wr8(dpa(ACTOR_FLIP), 0);
    for (;;) {
        /* on foot */
        y8_load(0x12);
        call_frame(0xEF54, abi_ActorSpeedXFromA5);
        a8_load(0x02);
        call_frame(0xEF59, abi_ActorAnimChange);
        do {
            actor_yield(0xA5EF5Au);
            restart_check(0xEF5C);
            jsl_call(0xA5EF5Fu, abi_ActorMoveX);
            jsl_call(0xA5EF63u, abi_ActorSnapToGround);
        } while (!(cpu.p & P_Z));
        /* off a ledge: falling until on ground again */
        y8_load(0x7E);
        jsl_call(0xA5EF6Bu, abi_ActorMotionPreset);
        do {
            actor_yield(0xA5EF6Fu);
            restart_check(0xEF71);
            call_frame(0xEF76, abi_ActorAnimRiseFall);
            jsl_call(0xA5EF77u, abi_ActorFly);
            jsl_call(0xA5EF7Bu, abi_ActorOutOfGround);
        } while (cpu.p & P_Z);
    }
}

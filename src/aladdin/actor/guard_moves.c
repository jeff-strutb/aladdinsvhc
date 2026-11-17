/*
 * guard_moves.c: helpers of the bank $A5 guard's (actor kind $08) moves:
 * a fall held to a top speed, a snap to a tile's centre, and the running
 * guard's look at the ground ahead. The moves themselves are in
 * guard_moves.c.held.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorFly(void);
void abi_ActorSpeedYFromA5(void);
void abi_PointInGroundLong(void);
uint32_t guard_step_ahead(void);

#define G_POINT_X SCRATCH               /* the probe point PointInGround tests: x (word) */
#define G_POINT_Y (SCRATCH + 2)         /* its y (word) */
#define G_FALL_MAX 0x0580u              /* the fastest it falls */
#define G_TILE (SCRATCH + 6)            /* PointInGround's tile behaviour: bits 0-2 its kind of ground */
#define G_STEP_AHEAD_X 0xCFF4u          /* (data bank) by speed and facing: x of the point ahead (word) */
#define G_STEP_AHEAD_Y 0xD010u          /* (data bank) by speed and facing: y of the point ahead (word) */

/*
 * @routine A5:EAB3 ActorFlyFallCapped
 * WHAT IT DOES: ActorFly, with the fall held to G_FALL_MAX at most.
 * HOW: ActorFly; falling (the speed's top byte not negative): 16-bit, the
 * speed's low word G_FALL_MAX at most.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
 * M 8-bit.
 */
void abi_ActorFlyFallCapped(void)
{
    uint16_t speed;

    abi_ActorFly();
    a8_load(rd8(dpa(ACTOR_SPEED_Y + 2)));
    if (cpu.a & 0x80)
        return;
    op_rep(0x20);
    a16_load(G_FALL_MAX);
    speed = dp16(ACTOR_SPEED_Y);
    cmp16_of(G_FALL_MAX, speed);
    if (G_FALL_MAX < speed)
        dp16_set(ACTOR_SPEED_Y, G_FALL_MAX);
    op_sep(0x20);
}

/*
 * @routine A5:EAC9 GuardSnapToTile
 * WHAT IT DOES: Sets the guard at the centre of the tile the last probe
 * found: its x at the tile's middle, its y 8 above the tile's top.
 * HOW: 16-bit: x = (G_POINT_X AND $FFF0) + 8; y = (G_POINT_Y AND $FFF0)
 * - 8.
 * Entry: M 8-bit; D = the guard's record; called by JSR after a probe
 * (PointInGround). Exit: M 8-bit.
 */
void abi_GuardSnapToTile(void)
{
    op_rep(0x20);
    flag_set(P_C, 0);
    a16_load(op_adc16((uint16_t)(rd16(dba(G_POINT_X)) & 0xFFF0), 0x0008, &cpu.p));
    dp16_set(ACTOR_X + 1, cpu.a);
    flag_set(P_C, 1);
    a16_load(op_sbc16((uint16_t)(rd16(dba(G_POINT_Y)) & 0xFFF0), 0x0008, &cpu.p));
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(0x20);
}

/* GuardStepAhead's work, for a caller that has pushed its JSR's return:
   0 when the ground ahead is clear (the routine's RTS, the return not yet
   pulled); else, with the return pulled and the rise's speed set, where
   the guard goes on ($A5:E6A0, GuardJump). */
uint32_t guard_step_ahead(void)
{
    uint8_t x;

    x = (uint8_t)(((rd8(dpa(ACTOR_SPEED_X + 1)) << 1) ^ rd8(dpa(ACTOR_FACING))) << 1);
    a8_load(x);
    x8_load(x);
    op_rep(0x20);
    flag_set(P_C, 0);
    a16_load(op_adc16(rd16(dba((uint16_t)(G_STEP_AHEAD_X + x))), dp16(ACTOR_X + 1), &cpu.p));
    wr16(dba(G_POINT_X), cpu.a);
    flag_set(P_C, 0);
    a16_load(op_adc16(rd16(dba((uint16_t)(G_STEP_AHEAD_Y + x))), dp16(ACTOR_Y + 1), &cpu.p));
    wr16(dba(G_POINT_Y), cpu.a);
    abi_PointInGroundLong();                     /* leaves M 8-bit */
    a8_load((uint8_t)(rd8(dba(G_TILE)) & 0x07));
    if (!(uint8_t)cpu.a)
        return 0;
    a8_load((uint8_t)(pull16() >> 8));           /* PLA, PLA: its JSR's return; A its high byte */
    y8_load(0x09);
    abi_ActorSpeedYFromA5();
    return 0xA5E6A0u;
}

/*
 * @routine A5:EAE6 GuardStepAhead
 * WHAT IT DOES: The running guard's look ahead: where the ground rises in
 * front of it, it jumps (it leaves its caller and goes to GuardJump).
 * HOW: X = ((speed's middle byte * 2) EOR facing) * 2; the point = its x
 * + G_STEP_AHEAD_X, its y + G_STEP_AHEAD_Y (16-bit) by X;
 * PointInGroundLong; G_TILE's kind 0: RTS. Else its JSR's return pulled,
 * ActorSpeedYFromA5 9, on at GuardJump.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; called by JSR
 * from GuardRun. Exit: M 8-bit (no ground ahead); else never returns.
 */
void abi_GuardStepAhead(void)
{
    if (guard_step_ahead())
        actor_leave(0xA5E6A0u);
}

/*
 * guard_moves.c: helpers of the bank $A5 guard's (actor kind $08) moves:
 * a fall held to a top speed and a snap to a tile's centre. The moves
 * themselves, and the running guard's look ahead, are in
 * guard_pieces.c.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorFly(void);

#define G_POINT_X SCRATCH               /* the probe point PointInGround tests: x (word) */
#define G_POINT_Y (SCRATCH + 2)         /* its y (word) */
#define G_FALL_MAX 0x0580u              /* the fastest it falls */

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

/*
 * ground_push.c: ground that pushes the player along: tiles whose behaviour has bit 4
 * set. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_GroundBehindBit4(void);

#define GROUND_PUSHED 0x001429u     /* 2 while the player stands on ground that pushes, else 0 */
#define PUSH_TOP_SPEED 0x0280       /* the fastest the push carries him */

/*
 * @routine 81:CE86 PlayerGroundPush
 * WHAT IT DOES: On ground that pushes (tile behaviour bit 4), steps the
 * player's speed across by $80 one way or the other by its facing and the
 * tile's bit 7, up to $0280 and down to 0, and marks it pushed.
 * HOW: GROUND_PUSHED cleared; GroundBehindBit4 with probe $18: zero, done.
 * Else GROUND_PUSHED = 2; bit 7 clear (C clear): facing left adds $80 (at
 * most $0280), facing right takes $80 (not below 0); bit 7 set: the other
 * way round.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:CD87) in levels with $035C = 4 or level
 * $1A. Exit: M 8-bit; A low as the last step left it.
 */
void abi_PlayerGroundPush(void)
{
    int pushed_left, faster;
    uint16_t speed;

    wr8(dba(GROUND_PUSHED), 0);
    a8_load(0x18);
    wr8(dpa(ACTOR_18), 0x18);
    abi_GroundBehindBit4();
    if (cpu.p & P_Z)
        return;
    pushed_left = cpu.p & P_C;
    a8_load(0x02);
    wr8(dba(GROUND_PUSHED), 0x02);
    a8_load(rd8(dpa(ACTOR_FACING)));
    /* with the push: faster, to its top speed; against it: slower, to 0 */
    faster = pushed_left ? !(uint8_t)cpu.a : (uint8_t)cpu.a != 0;
    op_rep(P_M);
    if (faster) {
        flag_set(P_C, 0);
        speed = op_adc16(dp16(ACTOR_SPEED_X), 0x0080, &cpu.p);
        cpu.a = speed;
        cmp16_of(speed, PUSH_TOP_SPEED);
        if (speed >= PUSH_TOP_SPEED)
            a16_load(PUSH_TOP_SPEED);
        dp16_set(ACTOR_SPEED_X, cpu.a);
    } else {
        flag_set(P_C, 1);
        cpu.a = op_sbc16(dp16(ACTOR_SPEED_X), 0x0080, &cpu.p);
        if (cpu.p & P_C)
            dp16_set(ACTOR_SPEED_X, cpu.a);
    }
    op_sep(P_M);
}


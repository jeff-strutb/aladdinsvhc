/*
 * run_speed.c: the player's running speed from the joypad: up toward the top speed,
 * slowed with no direction, turned against. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_PadFacing(void);

/*
 * @routine 81:D1A0 PlayerRunSpeed
 * WHAT IT DOES: Steps the player's running speed by the joypad: held
 * toward its facing, up by the run rate to the top speed; no direction,
 * down by the slowing rate to 0 (unless held up); held the other way, down
 * by the turning rate, and past 0 it turns round.
 * HOW: PadFacing: the same as the facing, PLAYER_RUN_ACCEL added and
 * capped at PLAYER_TOP_SPEED; none, unless PLAYER_SLOWED, PLAYER_RUN_DECEL
 * added to a nonzero speed, zeroed when it goes negative; the other way,
 * PLAYER_TURN_DECEL added; negative, the speed negated and the facing
 * flipped.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code. Exit: M 8-bit; A as the last step left it.
 */
void abi_PlayerRunSpeed(void)
{
    uint16_t speed, top;
    uint8_t v;

    abi_PadFacing();
    if (cpu.p & P_N) {                    /* no direction held: slowing */
        v = rd8(dba(PLAYER_SLOWED));
        a8_load(v);
        if (v)
            return;
        op_rep(P_M);
        speed = dp16(ACTOR_SPEED_X);
        a16_load(speed);
        if (speed) {
            a16_load(rd16(dba(PLAYER_RUN_DECEL)));
            flag_set(P_C, 0);
            cpu.a = op_adc16(cpu.a, speed, &cpu.p);
            dp16_set(ACTOR_SPEED_X, cpu.a);
            if (cpu.a & 0x8000)
                dp16_set(ACTOR_SPEED_X, 0);
        }
        op_sep(P_M);
        return;
    }
    cmp8_of((uint8_t)cpu.a, rd8(dpa(ACTOR_FACING)));
    if (cpu.p & P_Z) {                    /* held his way: faster, to the top speed */
        op_rep(P_M);
        flag_set(P_C, 0);
        cpu.a = op_adc16(dp16(ACTOR_SPEED_X), rd16(dba(PLAYER_RUN_ACCEL)), &cpu.p);
        top = rd16(dba(PLAYER_TOP_SPEED));
        cmp16_of(cpu.a, top);
        if (cpu.a >= top)
            a16_load(top);
        dp16_set(ACTOR_SPEED_X, cpu.a);
        op_sep(P_M);
        return;
    }
    /* held the other way: slowing, and past 0 he turns */
    op_rep(P_M);
    a16_load(rd16(dba(PLAYER_TURN_DECEL)));
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, dp16(ACTOR_SPEED_X), &cpu.p);
    dp16_set(ACTOR_SPEED_X, cpu.a);
    if (!(cpu.a & 0x8000)) {
        op_sep(P_M);
        return;
    }
    a16_load((uint16_t)((cpu.a ^ 0xFFFF) + 1));
    dp16_set(ACTOR_SPEED_X, cpu.a);
    op_sep(P_M);
    v = (uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01);
    a8_load(v);
    wr8(dpa(ACTOR_FACING), v);
}


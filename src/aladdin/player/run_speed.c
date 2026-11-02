/*
 * run_speed.c: the player's running speed from the joypad: up toward the top speed,
 * slowed with no direction, turned against. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
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
    abi_PadFacing();  /* 81D1A0 jsr */
    if (IF_N) goto L_81D1BD;  /* 81D1A3 bmi */
    cmp8_v(rd8(ea_dp(0x14)));  /* 81D1A5 cmp */
    if (!IF_Z) goto L_81D1D6;  /* 81D1A7 bne */
    op_rep(0x20);  /* 81D1A9 rep */
    clc();  /* 81D1AB clc */
    lda16(ea_dp(0x19));  /* 81D1AC lda */
    adc16_v(rd16(ea_abs(PLAYER_RUN_ACCEL)));  /* 81D1AE adc */
    cmp16_v(rd16(ea_abs(PLAYER_TOP_SPEED)));  /* 81D1B1 cmp */
    if (!IF_C) goto L_81D1B9;  /* 81D1B4 bcc */
    lda16(ea_abs(PLAYER_TOP_SPEED));  /* 81D1B6 lda */
L_81D1B9:
    sta16(ea_dp(0x19));  /* 81D1B9 sta */
    goto L_81D1F2;  /* 81D1BB bra */
L_81D1BD:
    lda8(ea_abs(PLAYER_SLOWED));  /* 81D1BD lda */
    if (!IF_Z) goto L_81D1F4;  /* 81D1C0 bne */
    op_rep(0x20);  /* 81D1C2 rep */
    lda16(ea_dp(0x19));  /* 81D1C4 lda */
    if (IF_Z) goto L_81D1F2;  /* 81D1C6 beq */
    lda16(ea_abs(PLAYER_RUN_DECEL));  /* 81D1C8 lda */
    clc();  /* 81D1CB clc */
    adc16_v(rd16(ea_dp(0x19)));  /* 81D1CC adc */
    sta16(ea_dp(0x19));  /* 81D1CE sta */
    if (!IF_N) goto L_81D1F2;  /* 81D1D0 bpl */
    stz16(ea_dp(0x19));  /* 81D1D2 stz */
    goto L_81D1F2;  /* 81D1D4 bra */
L_81D1D6:
    op_rep(0x20);  /* 81D1D6 rep */
    lda16(ea_abs(PLAYER_TURN_DECEL));  /* 81D1D8 lda */
    clc();  /* 81D1DB clc */
    adc16_v(rd16(ea_dp(0x19)));  /* 81D1DC adc */
    sta16(ea_dp(0x19));  /* 81D1DE sta */
    if (!IF_N) goto L_81D1F2;  /* 81D1E0 bpl */
    eor16_v(0xFFFF);  /* 81D1E2 eor */
    ACC16(inc);  /* 81D1E5 inc */
    sta16(ea_dp(0x19));  /* 81D1E6 sta */
    op_sep(0x20);  /* 81D1E8 sep */
    lda8(ea_dp(0x14));  /* 81D1EA lda */
    eor8_v(0x01);  /* 81D1EC eor */
    sta8(ea_dp(0x14));  /* 81D1EE sta */
    goto L_81D1F4;  /* 81D1F0 bra */
L_81D1F2:
    op_sep(0x20);  /* 81D1F2 sep */
L_81D1F4:
    return;  /* 81D1F4 rts */
}


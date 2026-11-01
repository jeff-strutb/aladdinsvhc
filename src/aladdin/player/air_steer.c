/*
 * air_steer.c: the player's speed across while it is in the air, and a
 * few small player settings. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_ActorAccelerateX(void);
void abi_ActorDecelerateX(void);

/* Speed tables (in the data bank, $80), by direction and dash: the
   accelerations toward and against the facing, the speeds they stop at,
   and the speed the player settles to with no direction */
#define AIR_ACCEL_TOWARD 0xC781
#define AIR_ACCEL_AGAINST 0xC787
#define AIR_TOP_TOWARD 0xC78A
#define AIR_LEAST_AGAINST 0xC796
#define AIR_SETTLE 0xC7A2

/*
 * @routine 81:CF2E PlayerAirSteer
 * WHAT IT DOES: Steers the player across while it is in the air: held
 * toward its facing it speeds up, against it slows down, each to a limit;
 * with no direction its speed settles to a set speed.
 * HOW: The direction bits come from V_1434 (when V_1428 is set; 0 there
 * reads as 2) or V_0012. Bits 0-2 at 0, 2 or 3: no direction; the speed
 * is brought to the settle speed of the dash row (ActorAccelerateX by $28
 * when below, ActorDecelerateX by 8 when above, not under $0180). Else
 * bit 6 against the facing at $14 picks: the same way, ActorAccelerateX
 * by the row's rate, the speed cut to the row's top; the other way,
 * ActorDecelerateX by the row's rate, the speed raised to the row's
 * least. The rows are by the direction's low two bits and PLAYER_DASH.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80. Exit: M and
 * X 8-bit; A, X, Y and the flags as the path taken leaves them.
 */
void abi_PlayerAirSteer(void)
{
    lda8(ea_abs(V_1428));
    if (IF_Z) goto from_0012;
    ldx8(ea_abs(V_1434));
    if (!IF_Z) goto dir;
    ldx8_v(0x02);
    goto dir;
from_0012:
    ldx8(ea_abs(V_0012));
dir:
    txa8();
    and8_v(0x07);
    if (IF_Z) goto settle;
    cmp8_v(0x02);
    if (IF_Z) goto settle;
    cmp8_v(0x03);
    if (IF_Z) goto settle;
    txa8();
    ACC8(rol);
    ACC8(rol);
    and8_v(0x01);
    eor8_v(rd8(ea_dp(ACTOR_FACING)));
    if (IF_Z) goto against;
    txa8();
    and8_v(0x03);
    ACC8(asl);
    pha8();
    clc();
    adc8_v(rd8(ea_abs(PLAYER_DASH)));
    tay8();
    lda8(ea_absy(AIR_ACCEL_TOWARD));
    sta8(ea_dp(ACTOR_ACCEL_X));
    abi_ActorAccelerateX();
    pla8();
    ACC8(asl);
    clc();
    adc8_v(rd8(ea_abs(PLAYER_DASH)));
    clc();
    adc8_v(rd8(ea_abs(PLAYER_DASH)));
    tax8();
    op_rep(0x20);
    lda16(ea_absx(AIR_TOP_TOWARD));
    cmp16_v(rd16(ea_dp(ACTOR_SPEED_X)));
    if (!IF_C)
        sta16(ea_dp(ACTOR_SPEED_X));
    op_sep(0x20);
    return;
against:
    txa8();
    and8_v(0x03);
    pha8();
    tay8();
    lda8(ea_absy(AIR_ACCEL_AGAINST));
    sta8(ea_dp(ACTOR_ACCEL_X));
    abi_ActorDecelerateX();
    pla8();
    ACC8(asl);
    ACC8(asl);
    clc();
    adc8_v(rd8(ea_abs(PLAYER_DASH)));
    clc();
    adc8_v(rd8(ea_abs(PLAYER_DASH)));
    tax8();
    op_rep(0x20);
    lda16(ea_absx(AIR_LEAST_AGAINST));
    cmp16_v(rd16(ea_dp(ACTOR_SPEED_X)));
    if (IF_C)
        sta16(ea_dp(ACTOR_SPEED_X));
    op_sep(0x20);
    return;
settle:
    op_rep(0x10);
    lda8_v(0x00);
    xba();
    lda8(ea_abs(PLAYER_DASH));
    ACC8(asl);
    tay16();
    ldx16(ea_absy(AIR_SETTLE));
    cpx16_v(rd16(ea_dp(ACTOR_SPEED_X)));
    if (IF_Z) goto done;
    if (!IF_C) goto above;
    cpx16_v(rd16(ea_dp(ACTOR_SPEED_X)));
    if (IF_C)
        stx16(ea_dp(ACTOR_SPEED_X));
    lda8_v(0x28);
    sta8(ea_dp(ACTOR_ACCEL_X));
    abi_ActorAccelerateX();
    goto done;
above:
    lda8_v(0x08);
    sta8(ea_dp(ACTOR_ACCEL_X));
    abi_ActorDecelerateX();
    ldx16_v(0x0180);
    cpx16_v(rd16(ea_dp(ACTOR_SPEED_X)));
    if (IF_C)
        stx16(ea_dp(ACTOR_SPEED_X));
done:
    op_sep(0x10);
}

/*
 * @routine 81:D855 PlayerSpeedLeast
 * WHAT IT DOES: Keeps the player moving across at three quarters of a
 * pixel a frame or more, counting the frames it had to.
 * HOW: A speed under $00C0 becomes $00C0 and PLAYER_SLOWED steps up.
 * Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: M 8-bit; A = the speed (or $00C0); N, Z, C from the
 * compare, or from the count's step; X, Y unchanged.
 */
void abi_PlayerSpeedLeast(void)
{
    op_rep(0x20);
    lda16(ea_dp(ACTOR_SPEED_X));
    sec();
    cmp16_v(0x00C0);
    if (!IF_C) {
        MEM16(inc, ea_abs(PLAYER_SLOWED));
        lda16_v(0x00C0);
        sta16(ea_dp(ACTOR_SPEED_X));
    }
    op_sep(0x20);
}

/*
 * @routine 81:DACE PlayerPosRestore
 * WHAT IT DOES: Puts the player back at the position kept for it.
 * HOW: Its X and Y pixels from PLAYER_POS_SAVED.
 * Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: M 8-bit; A = the Y; N, Z from it; X, Y unchanged.
 */
void abi_PlayerPosRestore(void)
{
    op_rep(0x20);
    lda16(ea_abs(PLAYER_POS_SAVED));
    sta16(ea_dp(ACTOR_X + 1));
    lda16(ea_abs(PLAYER_POS_SAVED + 2));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
}

/*
 * @routine 81:DDB4 PlayerFlagsClear
 * WHAT IT DOES: Clears four of the player's flags and counts (V_1A08,
 * V_141C, V_142D, V_144D).
 * HOW: Each byte stored 0.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit:
 * everything unchanged.
 */
void abi_PlayerFlagsClear(void)
{
    stz8(ea_abs(V_1A08));
    stz8(ea_abs(V_141C));
    stz8(ea_abs(V_142D));
    stz8(ea_abs(V_144D));
}

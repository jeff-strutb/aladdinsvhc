/*
 * carpet.c: the magic carpet ride's steering: the joypad's eight
 * directions push the speeds, the carpet tilts when it climbs or dives,
 * and with no direction held it drifts back to its cruising speed and
 * levels off. Two rides use it: the one whose record has $06 = 1 and
 * another (the "Alt" routines), each with its own rates and limits.
 * Drafted by cdraft.py; named and described by hand.
 *
 * A steering direction ($14) is 0 right, then clockwise to 7 (2 down, 4
 * left, 6 up); 8 slows down across, 9 speeds up, 10 neither; negative
 * none. The rate tables (in bank $80) give per direction an across rate
 * and an up-down rate, signed by bit 7 (set: take away).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_PlayerMoveStart(void);

/* The ride's own fields of the rider's record */
#define CARPET_DIRECTION 0x14u     /* the steering direction (see above) */
#define CARPET_DIVING 0x41u        /* nonzero: tilted to dive (move 3) */
#define CARPET_CLIMBING 0x42u      /* nonzero: tilted to climb (move 1) */
#define CARPET_SPED_UP 0x44u       /* nonzero: it has sped up to cruise (direction 9) */
#define CARPET_SLOWED 0x45u        /* nonzero: it has slowed to cruise (direction 8) */

/* In bank $80: per direction, the across and the up-down rate (bytes) */
#define CARPET_RATES 0xC7B5u       /* the ride with $06 = 1 */
#define CARPET_RATES_ALT 0xC7CBu   /* the other ride */
/* In bank $80: per direction, 0 level, positive dive, negative climb */
#define CARPET_TILT 0xC7E1u

/*
 * @routine 81:E555 CarpetLevelOut
 * WHAT IT DOES: Levels the carpet off when it is tilted: out of a dive
 * (move 4) or out of a climb (move 2).
 * HOW: The dive flag set: cleared, PlayerMoveStart with move 4. Else the
 * climb flag set: cleared, PlayerMoveStart with move 2.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Also reached by
 * JMP from CarpetTilt. Exit: not tilted: A low = 0, Z set; levelled: as
 * PlayerMoveStart. M 8-bit, X 8-bit.
 */
void abi_CarpetLevelOut(void)
{
    lda8(ea_dp(CARPET_DIVING));  /* 81E555 lda */
    if (IF_Z) goto L_81E561;  /* 81E557 beq */
    stz8(ea_dp(CARPET_DIVING));  /* 81E559 stz */
    lda8_v(0x04);  /* 81E55B lda */
    abi_PlayerMoveStart();  /* 81E55D jsr */
    return;  /* 81E560 rts */
L_81E561:
    lda8(ea_dp(CARPET_CLIMBING));  /* 81E561 lda */
    if (IF_Z) goto L_81E56C;  /* 81E563 beq */
    stz8(ea_dp(CARPET_CLIMBING));  /* 81E565 stz */
    lda8_v(0x02);  /* 81E567 lda */
    abi_PlayerMoveStart();  /* 81E569 jsr */
L_81E56C:
    return;  /* 81E56C rts */
}


/*
 * @routine 81:E56D CarpetTilt
 * WHAT IT DOES: Tilts the carpet the way it is steered: diving (move 3)
 * when steered down, climbing (move 1) when steered up; level steering
 * levels it off.
 * HOW: The tilt table by the direction: 0, on into CarpetLevelOut;
 * positive, unless already diving, the dive flag set, the climb flag
 * cleared and PlayerMoveStart with move 3; negative, the same for a climb
 * with move 1.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). The direction
 * 0-7. Exit: steered up or down: A low = 1, except a new dive: as
 * PlayerMoveStart; level: as CarpetLevelOut. M 8-bit, X 8-bit.
 */
void abi_CarpetTilt(void)
{
    ldx8(ea_dp(CARPET_DIRECTION));  /* 81E56D ldx */
    lda8(ea_absx(CARPET_TILT));  /* 81E56F lda */
    if (IF_Z) goto L_81E594;  /* 81E572 beq */
    if (IF_N) goto L_81E584;  /* 81E574 bmi */
    lda8(ea_dp(CARPET_DIVING));  /* 81E576 lda */
    if (!IF_Z) goto L_81E591;  /* 81E578 bne */
    MEM8(inc, ea_dp(CARPET_DIVING));  /* 81E57A inc */
    stz8(ea_dp(CARPET_CLIMBING));  /* 81E57C stz */
    lda8_v(0x03);  /* 81E57E lda */
    abi_PlayerMoveStart();  /* 81E580 jsr */
    return;  /* 81E583 rts */
L_81E584:
    lda8(ea_dp(CARPET_CLIMBING));  /* 81E584 lda */
    if (!IF_Z) goto L_81E591;  /* 81E586 bne */
    MEM8(inc, ea_dp(CARPET_CLIMBING));  /* 81E588 inc */
    stz8(ea_dp(CARPET_DIVING));  /* 81E58A stz */
    lda8_v(0x01);  /* 81E58C lda */
    abi_PlayerMoveStart();  /* 81E58E jsr */
L_81E591:
    lda8_v(0x01);  /* 81E591 lda */
    return;  /* 81E593 rts */
L_81E594:
    abi_CarpetLevelOut(); return;  /* 81E594 jmp */
}


/*
 * @routine 81:E597 CarpetSteerXAlt
 * WHAT IT DOES: Pushes the other ride's speed across by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the across rate from the
 * other ride's table, then on in CarpetSteerX: added to the 24-bit speed,
 * or taken away when bit 7 is set.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: as
 * CarpetSteerX.
 */
void abi_CarpetSteerXAlt(void)
{
    lda8(ea_dp(CARPET_DIRECTION));  /* 81E597 lda */
    if (IF_N) goto L_81E5DA;  /* 81E599 bmi */
    ACC8(asl);  /* 81E59B asl */
    tax8();  /* 81E59C tax */
    lda8(ea_absx(CARPET_RATES_ALT));  /* 81E59D lda */
    if (IF_N) goto L_81E5C1;  /* 81E5A0 bmi */
    goto L_81E5AF;  /* 81E5A2 bra */
L_81E5AF:
    clc();  /* 81E5AF clc */
    adc8_v(rd8(ea_dp(0x19)));  /* 81E5B0 adc */
    sta8(ea_dp(0x19));  /* 81E5B2 sta */
    lda8(ea_dp(0x1A));  /* 81E5B4 lda */
    adc8_v(0x00);  /* 81E5B6 adc */
    sta8(ea_dp(0x1A));  /* 81E5B8 sta */
    lda8(ea_dp(0x1B));  /* 81E5BA lda */
    adc8_v(0x00);  /* 81E5BC adc */
    sta8(ea_dp(0x1B));  /* 81E5BE sta */
    return;  /* 81E5C0 rts */
L_81E5C1:
    and8_v(0x7F);  /* 81E5C1 and */
    sta8(ea_abs(0x002C));  /* 81E5C3 sta */
    lda8(ea_dp(0x19));  /* 81E5C6 lda */
    sec();  /* 81E5C8 sec */
    sbc8_v(rd8(ea_abs(0x002C)));  /* 81E5C9 sbc */
    sta8(ea_dp(0x19));  /* 81E5CC sta */
    lda8(ea_dp(0x1A));  /* 81E5CE lda */
    sbc8_v(0x00);  /* 81E5D0 sbc */
    sta8(ea_dp(0x1A));  /* 81E5D2 sta */
    lda8(ea_dp(0x1B));  /* 81E5D4 lda */
    sbc8_v(0x00);  /* 81E5D6 sbc */
    sta8(ea_dp(0x1B));  /* 81E5D8 sta */
L_81E5DA:
    return;  /* 81E5DA rts */
}


/*
 * @routine 81:E5A4 CarpetSteerX
 * WHAT IT DOES: Pushes the ride's speed across by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the across rate from the
 * table: bit 7 clear, added to the 24-bit speed; set, its low 7 bits (kept
 * at $002C) taken away. A byte at a time.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; X = the direction times 2; A low = the speed's top byte; flags
 * from the last add or subtract. No direction: A low = the direction, N
 * set.
 */
void abi_CarpetSteerX(void)
{
    lda8(ea_dp(CARPET_DIRECTION));  /* 81E5A4 lda */
    if (IF_N) goto L_81E5DA;  /* 81E5A6 bmi */
    ACC8(asl);  /* 81E5A8 asl */
    tax8();  /* 81E5A9 tax */
    lda8(ea_absx(CARPET_RATES));  /* 81E5AA lda */
    if (IF_N) goto L_81E5C1;  /* 81E5AD bmi */
    clc();  /* 81E5AF clc */
    adc8_v(rd8(ea_dp(0x19)));  /* 81E5B0 adc */
    sta8(ea_dp(0x19));  /* 81E5B2 sta */
    lda8(ea_dp(0x1A));  /* 81E5B4 lda */
    adc8_v(0x00);  /* 81E5B6 adc */
    sta8(ea_dp(0x1A));  /* 81E5B8 sta */
    lda8(ea_dp(0x1B));  /* 81E5BA lda */
    adc8_v(0x00);  /* 81E5BC adc */
    sta8(ea_dp(0x1B));  /* 81E5BE sta */
    return;  /* 81E5C0 rts */
L_81E5C1:
    and8_v(0x7F);  /* 81E5C1 and */
    sta8(ea_abs(0x002C));  /* 81E5C3 sta */
    lda8(ea_dp(0x19));  /* 81E5C6 lda */
    sec();  /* 81E5C8 sec */
    sbc8_v(rd8(ea_abs(0x002C)));  /* 81E5C9 sbc */
    sta8(ea_dp(0x19));  /* 81E5CC sta */
    lda8(ea_dp(0x1A));  /* 81E5CE lda */
    sbc8_v(0x00);  /* 81E5D0 sbc */
    sta8(ea_dp(0x1A));  /* 81E5D2 sta */
    lda8(ea_dp(0x1B));  /* 81E5D4 lda */
    sbc8_v(0x00);  /* 81E5D6 sbc */
    sta8(ea_dp(0x1B));  /* 81E5D8 sta */
L_81E5DA:
    return;  /* 81E5DA rts */
}


/*
 * @routine 81:E62F CarpetCruiseXAlt
 * WHAT IT DOES: With no direction held, steers the other ride back toward
 * its cruising speed across, once each way.
 * HOW: Once it has both sped up and slowed down to cruise: direction 10
 * (neither). Else the speed (16-bit, from its fraction byte) below $0181:
 * direction 9 and the sped-up flag; at or above: direction 8 and the
 * slowed flag (the two ends of CarpetCruiseX).
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A low = the direction set.
 */
void abi_CarpetCruiseXAlt(void)
{
    lda8(ea_dp(CARPET_SPED_UP));  /* 81E62F lda */
    if (IF_Z) goto L_81E637;  /* 81E631 beq */
    lda8(ea_dp(CARPET_SLOWED));  /* 81E633 lda */
    if (!IF_Z) goto L_81E65F;  /* 81E635 bne */
L_81E637:
    op_rep(0x20);  /* 81E637 rep */
    lda16(ea_dp(0x19));  /* 81E639 lda */
    sec();  /* 81E63B sec */
    cmp16_v(0x0181);  /* 81E63C cmp */
    if (IF_C) goto L_81E656;  /* 81E63F bcs */
    goto L_81E64D;  /* 81E641 bra */
L_81E64D:
    op_sep(0x20);  /* 81E64D sep */
    lda8_v(0x09);  /* 81E64F lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E651 sta */
    sta8(ea_dp(CARPET_SPED_UP));  /* 81E653 sta */
    return;  /* 81E655 rts */
L_81E656:
    op_sep(0x20);  /* 81E656 sep */
    lda8_v(0x08);  /* 81E658 lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E65A sta */
    sta8(ea_dp(CARPET_SLOWED));  /* 81E65C sta */
    return;  /* 81E65E rts */
L_81E65F:
    lda8_v(0x0A);  /* 81E65F lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E661 sta */
    return;  /* 81E663 rts */
}


/*
 * @routine 81:E643 CarpetCruiseX
 * WHAT IT DOES: With no direction held, steers the ride toward its
 * cruising speed across.
 * HOW: The speed (16-bit, from its fraction byte) below $0300: direction 9
 * (speed up) and the sped-up flag set to 9; else direction 8 (slow down)
 * and the slowed flag set to 8. After its end, direction 10 for
 * CarpetCruiseXAlt.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A low = the direction set; C from the comparison.
 */
void abi_CarpetCruiseX(void)
{
    op_rep(0x20);  /* 81E643 rep */
    lda16(ea_dp(0x19));  /* 81E645 lda */
    sec();  /* 81E647 sec */
    cmp16_v(0x0300);  /* 81E648 cmp */
    if (IF_C) goto L_81E656;  /* 81E64B bcs */
    op_sep(0x20);  /* 81E64D sep */
    lda8_v(0x09);  /* 81E64F lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E651 sta */
    sta8(ea_dp(CARPET_SPED_UP));  /* 81E653 sta */
    return;  /* 81E655 rts */
L_81E656:
    op_sep(0x20);  /* 81E656 sep */
    lda8_v(0x08);  /* 81E658 lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E65A sta */
    sta8(ea_dp(CARPET_SLOWED));  /* 81E65C sta */
    return;  /* 81E65E rts */
}


/*
 * @routine 81:E664 CarpetSteadyY
 * WHAT IT DOES: Steadies the carpet up and down: steers against its
 * vertical speed until it is small, then stops it.
 * HOW: The vertical speed (16-bit, from its fraction byte): 7 or more
 * down, direction 6 (up); below -8, direction 2 (down); between, the speed
 * zeroed (all three bytes) and no direction ($FF).
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A low = the direction set.
 */
void abi_CarpetSteadyY(void)
{
    op_rep(0x20);  /* 81E664 rep */
    lda16(ea_dp(0x1D));  /* 81E666 lda */
    if (IF_Z) goto L_81E684;  /* 81E668 beq */
    if (IF_N) goto L_81E678;  /* 81E66A bmi */
    cmp16_v(0x0007);  /* 81E66C cmp */
    if (!IF_C) goto L_81E684;  /* 81E66F bcc */
    op_sep(0x20);  /* 81E671 sep */
    lda8_v(0x06);  /* 81E673 lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E675 sta */
    return;  /* 81E677 rts */
L_81E678:
    cmp16_v(0xFFF8);  /* 81E678 cmp */
    if (IF_C) goto L_81E684;  /* 81E67B bcs */
    op_sep(0x20);  /* 81E67D sep */
    lda8_v(0x02);  /* 81E67F lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E681 sta */
    return;  /* 81E683 rts */
L_81E684:
    stz16(ea_dp(0x1D));  /* 81E684 stz */
    op_sep(0x20);  /* 81E686 sep */
    stz8(ea_dp(0x1F));  /* 81E688 stz */
    lda8_v(0xFF);  /* 81E68A lda */
    sta8(ea_dp(CARPET_DIRECTION));  /* 81E68C sta */
    return;  /* 81E68E rts */
}


/*
 * @routine 81:E68F CarpetSpeedXLimitAlt
 * WHAT IT DOES: Keeps the other ride's speed across between its least and
 * most: $0020 and $0380 (an eighth of a pixel and three and a half pixels
 * a frame).
 * HOW: The speed's fraction byte and pixels as a 16-bit word, compared
 * unsigned and set to the limit it passes.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A = the speed word as limited, or as it was; C from the last
 * comparison.
 */
void abi_CarpetSpeedXLimitAlt(void)
{
    op_rep(0x20);  /* 81E68F rep */
    lda16(ea_dp(0x19));  /* 81E691 lda */
    sec();  /* 81E693 sec */
    cmp16_v(0x0380);  /* 81E694 cmp */
    if (!IF_C) goto L_81E6A0;  /* 81E697 bcc */
    lda16_v(0x0380);  /* 81E699 lda */
    sta16(ea_dp(0x19));  /* 81E69C sta */
    goto L_81E6AB;  /* 81E69E bra */
L_81E6A0:
    sec();  /* 81E6A0 sec */
    cmp16_v(0x0020);  /* 81E6A1 cmp */
    if (IF_C) goto L_81E6AB;  /* 81E6A4 bcs */
    lda16_v(0x0020);  /* 81E6A6 lda */
    sta16(ea_dp(0x19));  /* 81E6A9 sta */
L_81E6AB:
    op_sep(0x20);  /* 81E6AB sep */
    return;  /* 81E6AD rts */
}


/*
 * @routine 81:E6AE CarpetSpeedXLimit
 * WHAT IT DOES: Keeps the ride's speed across between its least and most:
 * $0020 and $0480 (an eighth of a pixel and four and a half pixels a
 * frame).
 * HOW: The speed's fraction byte and pixels as a 16-bit word, compared
 * unsigned and set to the limit it passes.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; A = the speed word as limited, or as it was; C from the last
 * comparison.
 */
void abi_CarpetSpeedXLimit(void)
{
    op_rep(0x20);  /* 81E6AE rep */
    lda16(ea_dp(0x19));  /* 81E6B0 lda */
    sec();  /* 81E6B2 sec */
    cmp16_v(0x0480);  /* 81E6B3 cmp */
    if (!IF_C) goto L_81E6BF;  /* 81E6B6 bcc */
    lda16_v(0x0480);  /* 81E6B8 lda */
    sta16(ea_dp(0x19));  /* 81E6BB sta */
    goto L_81E6CA;  /* 81E6BD bra */
L_81E6BF:
    sec();  /* 81E6BF sec */
    cmp16_v(0x0020);  /* 81E6C0 cmp */
    if (IF_C) goto L_81E6CA;  /* 81E6C3 bcs */
    lda16_v(0x0020);  /* 81E6C5 lda */
    sta16(ea_dp(0x19));  /* 81E6C8 sta */
L_81E6CA:
    op_sep(0x20);  /* 81E6CA sep */
    return;  /* 81E6CC rts */
}

/*
 * @routine 81:E5DB CarpetSteerYAlt
 * WHAT IT DOES: Pushes the other ride's vertical speed by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the up-down rate from the
 * other ride's table; a rate of 0: CarpetSteadyY, then the same rate read
 * again; then on in CarpetSteerY: added to the 24-bit speed, or taken away
 * when bit 7 is set.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: as
 * CarpetSteerY.
 */
void abi_CarpetSteerYAlt(void)
{
    lda8(ea_dp(0x14));  /* 81E5DB lda */
    if (IF_N) goto L_81E62E;  /* 81E5DD bmi */
    ACC8(asl);  /* 81E5DF asl */
    tax8();  /* 81E5E0 tax */
    lda8(ea_absx(0xC7CC));  /* 81E5E1 lda */
    if (!IF_Z) goto L_81E5EC;  /* 81E5E4 bne */
    abi_CarpetSteadyY();  /* 81E5E6 jsr */
    lda8(ea_absx(0xC7CC));  /* 81E5E9 lda */
L_81E5EC:
    if (IF_N) goto L_81E615;  /* 81E5EC bmi */
    goto L_81E603;  /* 81E5EE bra */
L_81E603:
    clc();  /* 81E603 clc */
    adc8_v(rd8(ea_dp(0x1D)));  /* 81E604 adc */
    sta8(ea_dp(0x1D));  /* 81E606 sta */
    lda8(ea_dp(0x1E));  /* 81E608 lda */
    adc8_v(0x00);  /* 81E60A adc */
    sta8(ea_dp(0x1E));  /* 81E60C sta */
    lda8(ea_dp(0x1F));  /* 81E60E lda */
    adc8_v(0x00);  /* 81E610 adc */
    sta8(ea_dp(0x1F));  /* 81E612 sta */
    return;  /* 81E614 rts */
L_81E615:
    and8_v(0x7F);  /* 81E615 and */
    sta8(ea_abs(0x002C));  /* 81E617 sta */
    lda8(ea_dp(0x1D));  /* 81E61A lda */
    sec();  /* 81E61C sec */
    sbc8_v(rd8(ea_abs(0x002C)));  /* 81E61D sbc */
    sta8(ea_dp(0x1D));  /* 81E620 sta */
    lda8(ea_dp(0x1E));  /* 81E622 lda */
    sbc8_v(0x00);  /* 81E624 sbc */
    sta8(ea_dp(0x1E));  /* 81E626 sta */
    lda8(ea_dp(0x1F));  /* 81E628 lda */
    sbc8_v(0x00);  /* 81E62A sbc */
    sta8(ea_dp(0x1F));  /* 81E62C sta */
L_81E62E:
    return;  /* 81E62E rts */
}


/*
 * @routine 81:E5F0 CarpetSteerY
 * WHAT IT DOES: Pushes the ride's vertical speed by its rate for the
 * steering direction.
 * HOW: No direction (negative): nothing. Else the up-down rate from the
 * table; a rate of 0: CarpetSteadyY, then the same rate read again. Bit 7
 * clear, added to the 24-bit speed; set, its low 7 bits (kept at $002C)
 * taken away. A byte at a time.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
 * 8-bit; X = the direction times 2; A low = the speed's top byte; flags
 * from the last add or subtract. No direction: A low = the direction, N
 * set.
 */
void abi_CarpetSteerY(void)
{
    lda8(ea_dp(0x14));  /* 81E5F0 lda */
    if (IF_N) goto L_81E62E;  /* 81E5F2 bmi */
    ACC8(asl);  /* 81E5F4 asl */
    tax8();  /* 81E5F5 tax */
    lda8(ea_absx(0xC7B6));  /* 81E5F6 lda */
    if (!IF_Z) goto L_81E601;  /* 81E5F9 bne */
    abi_CarpetSteadyY();  /* 81E5FB jsr */
    lda8(ea_absx(0xC7B6));  /* 81E5FE lda */
L_81E601:
    if (IF_N) goto L_81E615;  /* 81E601 bmi */
    clc();  /* 81E603 clc */
    adc8_v(rd8(ea_dp(0x1D)));  /* 81E604 adc */
    sta8(ea_dp(0x1D));  /* 81E606 sta */
    lda8(ea_dp(0x1E));  /* 81E608 lda */
    adc8_v(0x00);  /* 81E60A adc */
    sta8(ea_dp(0x1E));  /* 81E60C sta */
    lda8(ea_dp(0x1F));  /* 81E60E lda */
    adc8_v(0x00);  /* 81E610 adc */
    sta8(ea_dp(0x1F));  /* 81E612 sta */
    return;  /* 81E614 rts */
L_81E615:
    and8_v(0x7F);  /* 81E615 and */
    sta8(ea_abs(0x002C));  /* 81E617 sta */
    lda8(ea_dp(0x1D));  /* 81E61A lda */
    sec();  /* 81E61C sec */
    sbc8_v(rd8(ea_abs(0x002C)));  /* 81E61D sbc */
    sta8(ea_dp(0x1D));  /* 81E620 sta */
    lda8(ea_dp(0x1E));  /* 81E622 lda */
    sbc8_v(0x00);  /* 81E624 sbc */
    sta8(ea_dp(0x1E));  /* 81E626 sta */
    lda8(ea_dp(0x1F));  /* 81E628 lda */
    sbc8_v(0x00);  /* 81E62A sbc */
    sta8(ea_dp(0x1F));  /* 81E62C sta */
L_81E62E:
    return;  /* 81E62E rts */
}

/*
 * @routine 81:E503 CarpetCruiseSet
 * WHAT IT DOES: Sets the carpet's speed across to cruise: $0300 for the
 * ride with $06 = 1, $0180 for the other.
 * HOW: The record's $06 less 1: zero, the speed's three bytes 0, 3, 0;
 * else $80, 1, 0.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). The game calls
 * it only from the other ride's branch (81:E4EC): the $0300 setting is
 * reached only by unit_carpet_cruise_check.box. Exit: M 8-bit; A low = the
 * speed's pixel byte.
 */
void abi_CarpetCruiseSet(void)
{
    lda8(ea_dp(0x06));  /* 81E503 lda */
    ACC8(dec);  /* 81E505 dec */
    if (!IF_Z) goto L_81E511;  /* 81E506 bne */
    stz8(ea_dp(0x19));  /* 81E508 stz */
    lda8_v(0x03);  /* 81E50A lda */
    sta8(ea_dp(0x1A));  /* 81E50C sta */
    stz8(ea_dp(0x1B));  /* 81E50E stz */
    return;  /* 81E510 rts */
L_81E511:
    lda8_v(0x80);  /* 81E511 lda */
    sta8(ea_dp(0x19));  /* 81E513 sta */
    lda8_v(0x01);  /* 81E515 lda */
    sta8(ea_dp(0x1A));  /* 81E517 sta */
    stz8(ea_dp(0x1B));  /* 81E519 stz */
    return;  /* 81E51B rts */
}


/*
 * @routine 81:E6CD CarpetSpeedYLimit
 * WHAT IT DOES: Keeps the carpet's vertical speed within its limits: for
 * the ride with $06 = 1 from -$01C0 up to $01E0 down; for the other from
 * -$0240 to $0280.
 * HOW: The speed (16-bit, from its fraction byte) signed: down past the
 * limit, or up past the other, set to it.
 * Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
 * DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit; A
 * as the speed left or set.
 */
void abi_CarpetSpeedYLimit(void)
{
    lda8(ea_dp(0x06));  /* 81E6CD lda */
    ACC8(dec);  /* 81E6CF dec */
    if (!IF_Z) goto L_81E6F2;  /* 81E6D0 bne */
    op_rep(0x20);  /* 81E6D2 rep */
    lda16(ea_dp(0x1D));  /* 81E6D4 lda */
    if (IF_N) goto L_81E6E5;  /* 81E6D6 bmi */
    sec();  /* 81E6D8 sec */
    cmp16_v(0x01E0);  /* 81E6D9 cmp */
    if (!IF_C) goto L_81E710;  /* 81E6DC bcc */
    lda16_v(0x01E0);  /* 81E6DE lda */
    sta16(ea_dp(0x1D));  /* 81E6E1 sta */
    goto L_81E710;  /* 81E6E3 bra */
L_81E6E5:
    sec();  /* 81E6E5 sec */
    cmp16_v(0xFE40);  /* 81E6E6 cmp */
    if (IF_C) goto L_81E710;  /* 81E6E9 bcs */
    lda16_v(0xFE40);  /* 81E6EB lda */
    sta16(ea_dp(0x1D));  /* 81E6EE sta */
    goto L_81E710;  /* 81E6F0 bra */
L_81E6F2:
    op_rep(0x20);  /* 81E6F2 rep */
    lda16(ea_dp(0x1D));  /* 81E6F4 lda */
    if (IF_N) goto L_81E705;  /* 81E6F6 bmi */
    sec();  /* 81E6F8 sec */
    cmp16_v(0x0280);  /* 81E6F9 cmp */
    if (!IF_C) goto L_81E710;  /* 81E6FC bcc */
    lda16_v(0x0280);  /* 81E6FE lda */
    sta16(ea_dp(0x1D));  /* 81E701 sta */
    goto L_81E710;  /* 81E703 bra */
L_81E705:
    sec();  /* 81E705 sec */
    cmp16_v(0xFDC0);  /* 81E706 cmp */
    if (IF_C) goto L_81E710;  /* 81E709 bcs */
    lda16_v(0xFDC0);  /* 81E70B lda */
    sta16(ea_dp(0x1D));  /* 81E70E sta */
L_81E710:
    op_sep(0x20);  /* 81E710 sep */
    return;  /* 81E712 rts */
}


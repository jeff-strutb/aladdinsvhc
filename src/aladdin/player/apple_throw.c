/*
 * apple_throw.c: Aladdin throws an apple: a shot (kind $10) from the
 * second pool, from his hand by his pose, one apple fewer. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"
#include "wram_video.h"

void abi_ActorAllocBLong(void);
void abi_ActorStopX(void);
void abi_SoundQueueAdd(void);

#define APPLES 0x0369u  /* apples left, two decimal digits */
#define V_0389 0x0389u  /* zero: no apple is thrown */
#define THROW_X 0xC718u  /* by pose (the wanted box, low two bits), word: the hand's x from the player's, along his facing */
#define THROW_Y 0xC720u  /* by pose, word: the hand's height above his y */
#define THROW_COURSE 0xC728u  /* by pose, word (low byte used): the shot's course ($07) */

/*
 * @routine 81:CDE9 PlayerAppleThrow
 * WHAT IT DOES: Throws an apple when one can be thrown: a shot (kind $10)
 * starts at Aladdin's hand for his pose, on the course the pose gives,
 * facing his way; one apple fewer, sound 3, and his horizontal motion
 * stopped unless $141C is set. C set when thrown.
 * HOW: V_0389 zero, APPLES zero, LEVEL $25, or no free record
 * (ActorAllocBLong, N set): out with C clear (the clc at $81:CDE7). Else
 * the record: state $0C, facing and flip his flip; the pose
 * (PLAYER_BOX_WANTED and 3) picks THROW_X (added to his x, or taken when
 * his flip is 0), THROW_Y (taken from his y) and THROW_COURSE (its $07);
 * kind $10. APPLES down by one in decimal (a low digit 0 takes 7 off);
 * SoundQueueAdd 3; V_141C zero: ActorStopX. C set.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; called by
 * JSR from the player's code. Exit: M 8-bit, X 8-bit; C set when thrown,
 * clear when not.
 */
void abi_PlayerAppleThrow(void)
{
    goto L_81CDE9;
L_81CDE7:
    clc();  /* 81CDE7 clc */
    return;  /* 81CDE8 rts */
L_81CDE9:
    lda8(ea_abs(V_0389));  /* 81CDE9 lda */
    if (IF_Z) goto L_81CDE7;  /* 81CDEC beq */
    lda8(ea_abs(APPLES));  /* 81CDEE lda */
    if (IF_Z) goto L_81CDE7;  /* 81CDF1 beq */
    lda8(ea_abs(LEVEL));  /* 81CDF3 lda */
    cmp8_v(0x25);  /* 81CDF6 cmp */
    if (IF_Z) goto L_81CDE7;  /* 81CDF8 beq */
    abi_ActorAllocBLong();  /* 81CDFA jsl */
    if (IF_N) goto L_81CDE7;  /* 81CDFE bmi */
    lda8_v(0x0C);  /* 81CE00 lda */
    sta8(ea_absx(ACTOR_STATE));  /* 81CE02 sta */
    lda8(ea_dp(ACTOR_FLIP));  /* 81CE05 lda */
    sta8(ea_absx(ACTOR_FACING));  /* 81CE07 sta */
    sta8(ea_absx(ACTOR_FLIP));  /* 81CE0A sta */
    op_rep(0x20);  /* 81CE0D rep */
    lda16(ea_abs(PLAYER_BOX_WANTED));  /* 81CE0F lda */
    and16_v(0x0003);  /* 81CE12 and */
    pha16();  /* 81CE15 pha */
    ACC16(asl);  /* 81CE16 asl */
    tay16();  /* 81CE17 tay */
    lda16(ea_dp(ACTOR_FLIP));  /* 81CE18 lda */
    and16_v(0x00FF);  /* 81CE1A and */
    if (IF_Z) goto L_81CE2A;  /* 81CE1D beq */
    lda16(ea_dp(ACTOR_X + 1));  /* 81CE1F lda */
    clc();  /* 81CE21 clc */
    adc16_v(rd16(ea_absy(THROW_X)));  /* 81CE22 adc */
    sta16(ea_absx(ACTOR_X + 1));  /* 81CE25 sta */
    goto L_81CE33;  /* 81CE28 bra */
L_81CE2A:
    lda16(ea_dp(ACTOR_X + 1));  /* 81CE2A lda */
    sec();  /* 81CE2C sec */
    sbc16_v(rd16(ea_absy(THROW_X)));  /* 81CE2D sbc */
    sta16(ea_absx(ACTOR_X + 1));  /* 81CE30 sta */
L_81CE33:
    lda16(ea_dp(ACTOR_Y + 1));  /* 81CE33 lda */
    sec();  /* 81CE35 sec */
    sbc16_v(rd16(ea_absy(THROW_Y)));  /* 81CE36 sbc */
    sta16(ea_absx(ACTOR_Y + 1));  /* 81CE39 sta */
    op_sep(0x20);  /* 81CE3C sep */
    ply16();  /* 81CE3E ply */
    lda8_v(0x10);  /* 81CE3F lda */
    sta8(ea_absx(ACTOR_06));  /* 81CE41 sta */
    lda8(ea_absy(THROW_COURSE));  /* 81CE44 lda */
    sta8(ea_absx(ACTOR_07));  /* 81CE47 sta */
    op_sep(0x10);  /* 81CE4A sep */
    lda8(ea_abs(APPLES));  /* 81CE4C lda */
    pha8();  /* 81CE4F pha */
    and8_v(0x0F);  /* 81CE50 and */
    if (!IF_Z) goto L_81CE5A;  /* 81CE52 bne */
    pla8();  /* 81CE54 pla */
    sec();  /* 81CE55 sec */
    sbc8_v(0x07);  /* 81CE56 sbc */
    goto L_81CE5C;  /* 81CE58 bra */
L_81CE5A:
    pla8();  /* 81CE5A pla */
    ACC8(dec);  /* 81CE5B dec */
L_81CE5C:
    sta8(ea_abs(APPLES));  /* 81CE5C sta */
    lda8_v(0x03);  /* 81CE5F lda */
    abi_SoundQueueAdd();  /* 81CE61 jsl */
    lda8(ea_abs(V_141C));  /* 81CE65 lda */
    if (!IF_Z) goto L_81CE6E;  /* 81CE68 bne */
    abi_ActorStopX();  /* 81CE6A jsl */
L_81CE6E:
    sec();  /* 81CE6E sec */
    return;  /* 81CE6F rts */
}

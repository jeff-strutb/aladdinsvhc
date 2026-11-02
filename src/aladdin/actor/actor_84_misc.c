/*
 * actor_84_misc.c: bank $84 actor helpers: a child spawned with a hit box
 * of $A2, an actor freed far out of range telling its parent, a chain of
 * followers, a clamp above the floor, the player's fall test, and a grab
 * test.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorAnimFreeFar(void);
void abi_ActorClampAboveFloor(void);
void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorGrabTest(void);
void abi_ActorHitsTestBoxIfActiveLong(void);
void abi_ActorMoveAlong(void);
void abi_ActorSpawn(void);
void abi_ActorSpawnA2(void);
void abi_ActorStompOrHurtLong(void);
void abi_ActorsFollowSteps(void);
void abi_AnimStep(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_PlayerFallCheck(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 84:8145 ActorSpawnA2
 * WHAT IT DOES: Starts a child with hit box $A2 at the spawner's spawn
 * point $82, telling it its parent and a $07, and counts it in the
 * spawner's $42.
 * HOW: A kept at $002C; ActorSpawn; none free: back. The child's $07 =
 * the spawner's $07 less 3 plus A when that $07 is 3 or more, else the
 * spawner's $44; the child's $4C word = the spawner's record (D); the
 * spawner's $42 up.
 * Entry: M 8-bit, X 8-bit; A = the byte for the child's $07 sum; D = the
 * spawner; DB = $81; called by JSR. Exit: M 8-bit, X 8-bit; spawned: X =
 * the child's low byte; else as ActorSpawn.
 */
void abi_ActorSpawnA2(void)
{
    sta8(ea_abs(0x002C));  /* 848145 sta */
    lda8_v(0x82);  /* 848148 lda */
    sta8(ea_dp(0x21));  /* 84814A sta */
    lda8_v(0xA2);  /* 84814C lda */
    abi_ActorSpawn();  /* 84814E jsl */
    if (IF_N) goto L_848172;  /* 848152 bmi */
    sec();  /* 848154 sec */
    lda8(ea_dp(0x07));  /* 848155 lda */
    sbc8_v(0x03);  /* 848157 sbc */
    if (!IF_C) goto L_848161;  /* 848159 bcc */
    clc();  /* 84815B clc */
    adc8_v(rd8(ea_abs(0x002C)));  /* 84815C adc */
    goto L_848163;  /* 84815F bra */
L_848161:
    lda8(ea_dp(0x44));  /* 848161 lda */
L_848163:
    sta8(ea_absx(0x0007));  /* 848163 sta */
    tdc();  /* 848166 tdc */
    sta8(ea_absx(0x004C));  /* 848167 sta */
    xba();  /* 84816A xba */
    sta8(ea_absx(0x004D));  /* 84816B sta */
    MEM8(inc, ea_dp(0x42));  /* 84816E inc */
    op_sep(0x10);  /* 848170 sep */
L_848172:
    return;  /* 848172 rts */
}

/*
 * @routine 84:832B ActorAnimFreeFar
 * WHAT IT DOES: Plays the actor's animation, lets it hurt or be stomped
 * by the player, and ends it when out of range by box $58, then counting
 * it off its parent's $42.
 * HOW: AnimStep, ActorStompOrHurtLong, ActorFreeOutOfRangeLong with X =
 * $58; ended (C set): the parent's record (the actor's $4C word) has its
 * $42 stepped down.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL. Exit:
 * M 8-bit, X 8-bit.
 */
void abi_ActorAnimFreeFar(void)
{
    abi_AnimStep();  /* 84832B jsl */
    abi_ActorStompOrHurtLong();  /* 84832F jsl */
    ldx8_v(0x58);  /* 848333 ldx */
    abi_ActorFreeOutOfRangeLong();  /* 848335 jsl */
    if (!IF_C) goto L_848348;  /* 848339 bcc */
    op_rep(0x10);  /* 84833B rep */
    ldx16(ea_dp(0x4C));  /* 84833D ldx */
    lda8(ea_absx(0x0042));  /* 84833F lda */
    ACC8(dec);  /* 848342 dec */
    sta8(ea_absx(0x0042));  /* 848343 sta */
    op_sep(0x10);  /* 848346 sep */
L_848348:
    return;  /* 848348 rtl */
}

/*
 * @routine 84:8D04 ActorsFollowSteps
 * WHAT IT DOES: Moves each actor of the list from the head at $1B16 to
 * where the one before it is, then on along its own direction by the step
 * $41 + $1B19 (at least 1).
 * HOW: With 16-bit registers, from the head: each next record (the word
 * at $37 of the one before, 0 ends) becomes D; its x and y (the bytes at
 * $0B-$10) copied from the record before; its direction ($42 and $1FF) in
 * Y and the step in A (0 or negative taken as 1); ActorMoveAlong. D kept
 * on the stack.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M 8-bit, X
 * 8-bit; D as at entry.
 */
void abi_ActorsFollowSteps(void)
{
    phd();  /* 848D04 phd */
    op_rep(0x30);  /* 848D05 rep */
    ldx16(ea_abs(0x1B16));  /* 848D07 ldx */
L_848D0A:
    lda16(ea_absx(0x0037));  /* 848D0A lda */
    if (IF_Z) goto L_848D3D;  /* 848D0D beq */
    tcd();  /* 848D0F tcd */
    lda16(ea_absx(0x000B));  /* 848D10 lda */
    sta16(ea_dp(0x0B));  /* 848D13 sta */
    lda16(ea_absx(0x000D));  /* 848D15 lda */
    sta16(ea_dp(0x0D));  /* 848D18 sta */
    lda16(ea_absx(0x000F));  /* 848D1A lda */
    sta16(ea_dp(0x0F));  /* 848D1D sta */
    lda16(ea_dp(0x42));  /* 848D1F lda */
    and16_v(0x01FF);  /* 848D21 and */
    tay16();  /* 848D24 tay */
    op_sep(0x20);  /* 848D25 sep */
    clc();  /* 848D27 clc */
    lda8(ea_dp(0x41));  /* 848D28 lda */
    adc8_v(rd8(ea_abs(0x1B19)));  /* 848D2A adc */
    if (IF_Z) goto L_848D31;  /* 848D2D beq */
    if (!IF_N) goto L_848D33;  /* 848D2F bpl */
L_848D31:
    lda8_v(0x01);  /* 848D31 lda */
L_848D33:
    abi_ActorMoveAlong();  /* 848D33 jsl */
    op_rep(0x30);  /* 848D37 rep */
    tdc();  /* 848D39 tdc */
    tax16();  /* 848D3A tax */
    goto L_848D0A;  /* 848D3B bra */
L_848D3D:
    op_sep(0x30);  /* 848D3D sep */
    pld();  /* 848D3F pld */
    return;  /* 848D40 rts */
}

/*
 * @routine 84:8F3C ActorClampAboveFloor
 * WHAT IT DOES: Keeps the actor's y plus offset A above the floor line at
 * its x: C set (and y moved up to the floor) when it reached it.
 * HOW: The floor: the word for the actor's x from the table at $7E:DFC0
 * ((x - $200) / 4, in words), sign-extended from 9 bits, negated, plus
 * $B0. The actor's y + A: negative, C clear; below the floor, y = the
 * floor less the actor's offset at $D7C4 (by Y), C set; else C clear.
 * Entry: M 16-bit, X 8-bit; A = the offset; Y = the offset table's index;
 * D = the actor; called by JSR. Exit: M 16-bit; C as above.
 */
void abi_ActorClampAboveFloor(void)
{
    sta16(ea_abs(SCRATCH));  /* 848F3C sta */
    ldy8_v(0x00);  /* 848F3F ldy */
    sec();  /* 848F41 sec */
    lda16(ea_dp(0x0C));  /* 848F42 lda */
    sbc16_v(0x0200);  /* 848F44 sbc */
    ACC16(lsr);  /* 848F47 lsr */
    ACC16(lsr);  /* 848F48 lsr */
    and16_v(0x003E);  /* 848F49 and */
    tax8();  /* 848F4C tax */
    clc();  /* 848F4D clc */
    lda16(ea_longx(0x7EDFC0u));  /* 848F4E lda */
    and16_v(0x01FF);  /* 848F52 and */
    cmp16_v(0x0100);  /* 848F55 cmp */
    if (!IF_C) goto L_848F5D;  /* 848F58 bcc */
    ora16_v(0xFF00);  /* 848F5A ora */
L_848F5D:
    eor16_v(0xFFFF);  /* 848F5D eor */
    ACC16(inc);  /* 848F60 inc */
    clc();  /* 848F61 clc */
    adc16_v(0x00B0);  /* 848F62 adc */
    sta16(ea_abs(0x000E));  /* 848F65 sta */
    clc();  /* 848F68 clc */
    lda16(ea_dp(0x0F));  /* 848F69 lda */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 848F6B adc */
    if (IF_N) goto L_848F7F;  /* 848F6E bmi */
    cmp16_v(rd16(ea_abs(0x000E)));  /* 848F70 cmp */
    if (!IF_C) goto L_848F80;  /* 848F73 bcc */
    lda16(ea_abs(0x000E));  /* 848F75 lda */
    sbc16_v(rd16(ea_absy(0xD7C4)));  /* 848F78 sbc */
    sta16(ea_dp(0x0F));  /* 848F7B sta */
    sec();  /* 848F7D sec */
    return;  /* 848F7E rts */
L_848F7F:
    clc();  /* 848F7F clc */
L_848F80:
    return;  /* 848F80 rts */
}

/*
 * @routine 84:8F81 PlayerFallCheck
 * WHAT IT DOES: When $1B2B is clear and the player is at or below y $B8,
 * puts him into state $8C facing as he is (the fall).
 * HOW: $1B2B nonzero: nothing; the player's y negative: nothing; at or
 * past $B8: PlayerEnter8CFacingLong.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M 8-bit; as PlayerEnter8CFacingLong when it ran.
 */
void abi_PlayerFallCheck(void)
{
    lda8(ea_abs(0x1B2B));  /* 848F81 lda */
    if (!IF_Z) goto L_848F98;  /* 848F84 bne */
    op_rep(0x20);  /* 848F86 rep */
    lda16(ea_abs(0x08EA));  /* 848F88 lda */
    if (IF_N) goto L_848F99;  /* 848F8B bmi */
    cmp16_v(0x00B8);  /* 848F8D cmp */
    op_sep(0x20);  /* 848F90 sep */
    if (!IF_C) goto L_848F98;  /* 848F92 bcc */
    abi_PlayerEnter8CFacingLong();  /* 848F94 jsl */
L_848F98:
    return;  /* 848F98 rts */
L_848F99:
    op_sep(0x20);  /* 848F99 sep */
    return;  /* 848F9B rts */
}

/*
 * @routine 82:88CF ActorGrabTest
 * WHAT IT DOES: Tells whether the actor can grab the player: C clear when
 * he may be grabbed, touches it, is below it and is within its reach on
 * the side he faces.
 * HOW: $145A negative, $141B nonzero or $141C zero: C set.
 * ActorHitsTestBoxIfActiveLong: no touch, C set. The actor's y at or
 * below the player's: C set. Left of him: he must face right and its x +
 * its hit box's width - 15 reach his x; right of him: he must face left
 * and its x - width + 15 reach it; else C set.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80, the hit boxes' bank;
 * called by JSL. Exit: M 8-bit, X 8-bit (C clear, or C set at $82:8931 or
 * by the test).
 */
void abi_ActorGrabTest(void)
{
    lda8(ea_abs(0x145A));  /* 8288CF lda */
    if (IF_N) goto L_828935;  /* 8288D2 bmi */
    lda8(ea_abs(0x141B));  /* 8288D4 lda */
    if (!IF_Z) goto L_828935;  /* 8288D7 bne */
    lda8(ea_abs(V_141C));  /* 8288D9 lda */
    if (IF_Z) goto L_828935;  /* 8288DC beq */
    abi_ActorHitsTestBoxIfActiveLong();  /* 8288DE jsl */
    if (IF_C) goto L_828934;  /* 8288E2 bcs */
    op_rep(0x30);  /* 8288E4 rep */
    lda16(ea_dp(0x0F));  /* 8288E6 lda */
    cmp16_v(rd16(ea_abs(0x08EA)));  /* 8288E8 cmp */
    if (IF_C) goto L_828931;  /* 8288EB bcs */
    lda16(ea_dp(0x06));  /* 8288ED lda */
    and16_v(0x00FF);  /* 8288EF and */
    ACC16(asl);  /* 8288F2 asl */
    ACC16(asl);  /* 8288F3 asl */
    tay16();  /* 8288F4 tay */
    lda16(ea_dp(0x0C));  /* 8288F5 lda */
    sec();  /* 8288F7 sec */
    sbc16_v(rd16(ea_abs(0x08E7)));  /* 8288F8 sbc */
    if (IF_C) goto L_828916;  /* 8288FB bcs */
    lda16(ea_abs(0x08F0));  /* 8288FD lda */
    and16_v(0x00FF);  /* 828900 and */
    if (IF_Z) goto L_828931;  /* 828903 beq */
    lda16(ea_absy(0xC29A));  /* 828905 lda */
    sec();  /* 828908 sec */
    sbc16_v(0x000F);  /* 828909 sbc */
    clc();  /* 82890C clc */
    adc16_v(rd16(ea_dp(0x0C)));  /* 82890D adc */
    cmp16_v(rd16(ea_abs(0x08E7)));  /* 82890F cmp */
    if (IF_C) goto L_828931;  /* 828912 bcs */
    goto L_82892D;  /* 828914 bra */
L_828916:
    lda16(ea_abs(0x08F0));  /* 828916 lda */
    and16_v(0x00FF);  /* 828919 and */
    if (!IF_Z) goto L_828931;  /* 82891C bne */
    lda16(ea_dp(0x0C));  /* 82891E lda */
    sec();  /* 828920 sec */
    sbc16_v(rd16(ea_absy(0xC29A)));  /* 828921 sbc */
    clc();  /* 828924 clc */
    adc16_v(0x000F);  /* 828925 adc */
    cmp16_v(rd16(ea_abs(0x08E7)));  /* 828928 cmp */
    if (!IF_C) goto L_828931;  /* 82892B bcc */
L_82892D:
    op_sep(0x30);  /* 82892D sep */
    clc();  /* 82892F clc */
    return;  /* 828930 rtl */
L_828931:
    op_sep(0x31);  /* 828931 sep */
    return;  /* 828933 rtl */
L_828934:
    return;  /* 828934 rtl */
L_828935:
    sec();  /* 828935 sec */
    return;  /* 828936 rtl */
}

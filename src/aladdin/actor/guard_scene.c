/*
 * guard_scene.c: the scene code kind $92 sends a guard or escort to ($A5:E826, set in its resume address by the start at $A5:F5BA): by
 * its $47 (the kind's $07) one of three walks, each ending in GuardStop. cov_escort_scene plays them. Written from the traced assembly
 * (guard_scene.asm).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAnimRiseFall(void);
void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorMove(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);
void abi_ActorSpeedYFromA5(void);
void abi_AnimStart(void);
void abi_GuardStop(void);
#define SCENE_COUNTER 0x146Fu         /* the scene's frame counter: $2C ends a wait */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)  /* the player's x, whole pixels (word) */

void abi_GuardSceneStart(void);
void abi_GuardSceneWalkOut(void);
void abi_GuardSceneAhead(void);
void abi_GuardSceneCharge(void);
void abi_GuardSceneEnd(void);

/*
 * @routine A5:E826 GuardSceneStart
 * @actor
 * WHAT IT DOES: Where the actor goes on after kind $92 set its resume
 * address: by ACTOR_47 (the kind's $07, 0 to 3) to GuardSceneWalkOut (0),
 * GuardSceneCharge (1) or GuardSceneEnd (2 and 3).
 * HOW: A = ACTOR_47 times 2, X = A; JMP through the word table that
 * follows (GuardSceneWalkOut, GuardSceneCharge, GuardSceneEnd,
 * GuardSceneEnd).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's RTI at the resume address. Exit: never returns.
 */
void abi_GuardSceneStart(void)
{
    lda8(ea_dp(0x47));  /* A5E826 lda */
    ACC8(asl);  /* A5E828 asl */
    tax8();  /* A5E829 tax */
    switch (cpu.x) {  /* A5E82A jmp (table,X): the word table at $A5:E82D */
    case 0x00:
        abi_GuardSceneWalkOut();
        return;
    case 0x02:
        abi_GuardSceneCharge();
        return;
    case 0x04:
    case 0x06:
        abi_GuardSceneEnd();
        return;
    default:
        jump_routine(0xA50000u | rd16(0xA50000u | (uint16_t)(0xE82D + cpu.x)));
        return;
    }
}

/*
 * @routine A5:E835 GuardSceneWalkOut
 * @actor
 * WHAT IT DOES: Scene 0: the actor stands (animation 0) until its counter
 * reaches $2C or Aladdin is $90 ahead, then drops and walks out in two
 * legs: animation $0B falling to y $BC, animation $16 by motion $85 to
 * x $0D00, then by motions $7D, $7C, $7C and $7B through four falls, and
 * again the wait, the drop to y $58, the walk to x $0F00 and past $A0 in
 * its low x byte, and a last fall; then it ends as GuardStop with
 * ACTOR_41 0 and ACTOR_4D 4.
 * HOW: ACTOR_41 up; AnimStart 0; SCENE_COUNTER 0; each frame (BRK first)
 * GuardSceneAhead until C is clear; ActorSpeedYFromA5 with Y = $12; facing
 * and flip 0; AnimStart $0B; each frame ActorMoveY until y's high byte is
 * 0 and its low byte under $BC is not (below it); AnimStart $16, motion $85,
 * each frame ActorMove while x's top byte is under $0D; motion $7D, then
 * a fall each: ActorAnimRiseFall, ActorFly, ActorOutOfGround (until it is
 * on ground with ACTOR_SPEED_Y's top byte not negative) for motions $7D,
 * $7C, $7C, then facing and flip 1 and $7B; the wait again, the drop to $58,
 * x top byte $0F and low byte $A0, motion $7B and a last fall; ACTOR_41 0,
 * ACTOR_4D 4, on at GuardStop.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the jump
 * from GuardSceneStart. Exit: never returns.
 */
void abi_GuardSceneWalkOut(void)
{
    MEM8(inc, ea_dp(0x41));  /* A5E835 inc */
    lda8_v(0x00);  /* A5E837 lda */
    abi_AnimStart();  /* A5E839 jsl */
    stz8(ea_abs(SCENE_COUNTER));  /* A5E83D stz */
L_A5E840:
    actor_yield(0xA5E840u);  /* A5E840 brk */
    call_frame(0xE844, abi_GuardSceneAhead);
    if (IF_C) goto L_A5E840;  /* A5E845 bcs */
    ldy8_v(0x12);  /* A5E847 ldy */
    abi_ActorSpeedYFromA5();  /* A5E849 jsr */
    stz8(ea_dp(0x14));  /* A5E84C stz */
    stz8(ea_dp(0x15));  /* A5E84E stz */
    lda8_v(0x0B);  /* A5E850 lda */
    abi_AnimStart();  /* A5E852 jsl */
L_A5E856:
    actor_yield(0xA5E856u);  /* A5E856 brk */
    abi_ActorMoveY();  /* A5E858 jsl */
    lda8(ea_dp(0x10));  /* A5E85C lda */
    if (!IF_Z) goto L_A5E856;  /* A5E85E bne */
    lda8(ea_dp(0x0F));  /* A5E860 lda */
    cmp8_v(0xBC);  /* A5E862 cmp */
    if (IF_C) goto L_A5E856;  /* A5E864 bcs */
    lda8_v(0x16);  /* A5E866 lda */
    abi_AnimStart();  /* A5E868 jsl */
    ldy8_v(0x85);  /* A5E86C ldy */
    abi_ActorMotionPreset();  /* A5E86E jsl */
L_A5E872:
    actor_yield(0xA5E872u);  /* A5E872 brk */
    abi_ActorMove();  /* A5E874 jsl */
    lda8(ea_dp(0x0D));  /* A5E878 lda */
    cmp8_v(0x0D);  /* A5E87A cmp */
    if (!IF_C) goto L_A5E872;  /* A5E87C bcc */
    ldy8_v(0x7D);  /* A5E87E ldy */
    abi_ActorMotionPreset();  /* A5E880 jsl */
L_A5E884:
    actor_yield(0xA5E884u);  /* A5E884 brk */
    push16(0xE888); abi_ActorAnimRiseFall(); cpu.s = (uint16_t)(cpu.s + 2);  /* A5E886 jsr */
    abi_ActorFly();  /* A5E889 jsl */
    lda8(ea_dp(0x1F));  /* A5E88D lda */
    if (IF_N) goto L_A5E884;  /* A5E88F bmi */
    abi_ActorOutOfGround();  /* A5E891 jsl */
    if (IF_Z) goto L_A5E884;  /* A5E895 beq */
    ldy8_v(0x7C);  /* A5E897 ldy */
    abi_ActorMotionPreset();  /* A5E899 jsl */
L_A5E89D:
    actor_yield(0xA5E89Du);  /* A5E89D brk */
    push16(0xE8A1); abi_ActorAnimRiseFall(); cpu.s = (uint16_t)(cpu.s + 2);  /* A5E89F jsr */
    abi_ActorFly();  /* A5E8A2 jsl */
    lda8(ea_dp(0x1F));  /* A5E8A6 lda */
    if (IF_N) goto L_A5E89D;  /* A5E8A8 bmi */
    abi_ActorOutOfGround();  /* A5E8AA jsl */
    if (IF_Z) goto L_A5E89D;  /* A5E8AE beq */
    ldy8_v(0x7C);  /* A5E8B0 ldy */
    abi_ActorMotionPreset();  /* A5E8B2 jsl */
L_A5E8B6:
    actor_yield(0xA5E8B6u);  /* A5E8B6 brk */
    push16(0xE8BA); abi_ActorAnimRiseFall(); cpu.s = (uint16_t)(cpu.s + 2);  /* A5E8B8 jsr */
    abi_ActorFly();  /* A5E8BB jsl */
    lda8(ea_dp(0x1F));  /* A5E8BF lda */
    if (IF_N) goto L_A5E8B6;  /* A5E8C1 bmi */
    abi_ActorOutOfGround();  /* A5E8C3 jsl */
    if (IF_Z) goto L_A5E8B6;  /* A5E8C7 beq */
    lda8_v(0x01);  /* A5E8C9 lda */
    sta8(ea_dp(0x14));  /* A5E8CB sta */
    sta8(ea_dp(0x15));  /* A5E8CD sta */
    ldy8_v(0x7B);  /* A5E8CF ldy */
    abi_ActorMotionPreset();  /* A5E8D1 jsl */
L_A5E8D5:
    actor_yield(0xA5E8D5u);  /* A5E8D5 brk */
    push16(0xE8D9); abi_ActorAnimRiseFall(); cpu.s = (uint16_t)(cpu.s + 2);  /* A5E8D7 jsr */
    abi_ActorFly();  /* A5E8DA jsl */
    lda8(ea_dp(0x1F));  /* A5E8DE lda */
    if (IF_N) goto L_A5E8D5;  /* A5E8E0 bmi */
    abi_ActorOutOfGround();  /* A5E8E2 jsl */
    if (IF_Z) goto L_A5E8D5;  /* A5E8E6 beq */
    lda8_v(0x00);  /* A5E8E8 lda */
    abi_AnimStart();  /* A5E8EA jsl */
    stz8(ea_abs(SCENE_COUNTER));  /* A5E8EE stz */
L_A5E8F1:
    actor_yield(0xA5E8F1u);  /* A5E8F1 brk */
    call_frame(0xE8F5, abi_GuardSceneAhead);
    if (IF_C) goto L_A5E8F1;  /* A5E8F6 bcs */
    ldy8_v(0x12);  /* A5E8F8 ldy */
    abi_ActorSpeedYFromA5();  /* A5E8FA jsr */
    stz8(ea_dp(0x14));  /* A5E8FD stz */
    stz8(ea_dp(0x15));  /* A5E8FF stz */
    lda8_v(0x0B);  /* A5E901 lda */
    abi_AnimStart();  /* A5E903 jsl */
L_A5E907:
    actor_yield(0xA5E907u);  /* A5E907 brk */
    abi_ActorMoveY();  /* A5E909 jsl */
    lda8(ea_dp(0x10));  /* A5E90D lda */
    if (!IF_Z) goto L_A5E907;  /* A5E90F bne */
    lda8(ea_dp(0x0F));  /* A5E911 lda */
    cmp8_v(0x58);  /* A5E913 cmp */
    if (IF_C) goto L_A5E907;  /* A5E915 bcs */
    lda8_v(0x16);  /* A5E917 lda */
    abi_AnimStart();  /* A5E919 jsl */
    ldy8_v(0x85);  /* A5E91D ldy */
    abi_ActorMotionPreset();  /* A5E91F jsl */
L_A5E923:
    actor_yield(0xA5E923u);  /* A5E923 brk */
    abi_ActorMove();  /* A5E925 jsl */
    lda8(ea_dp(0x0D));  /* A5E929 lda */
    cmp8_v(0x0F);  /* A5E92B cmp */
    if (!IF_C) goto L_A5E923;  /* A5E92D bcc */
    lda8(ea_dp(0x0C));  /* A5E92F lda */
    cmp8_v(0xA0);  /* A5E931 cmp */
    if (!IF_C) goto L_A5E923;  /* A5E933 bcc */
    ldy8_v(0x7B);  /* A5E935 ldy */
    abi_ActorMotionPreset();  /* A5E937 jsl */
L_A5E93B:
    actor_yield(0xA5E93Bu);  /* A5E93B brk */
    push16(0xE93F); abi_ActorAnimRiseFall(); cpu.s = (uint16_t)(cpu.s + 2);  /* A5E93D jsr */
    abi_ActorFly();  /* A5E940 jsl */
    lda8(ea_dp(0x1F));  /* A5E944 lda */
    if (IF_N) goto L_A5E93B;  /* A5E946 bmi */
    abi_ActorOutOfGround();  /* A5E948 jsl */
    if (IF_Z) goto L_A5E93B;  /* A5E94C beq */
    stz8(ea_dp(0x41));  /* A5E94E stz */
    lda8_v(0x04);  /* A5E950 lda */
    sta8(ea_dp(0x4D));  /* A5E952 sta */
    abi_GuardStop(); return;  /* A5E954 jmp */
}

/*
 * @routine A5:E957 GuardSceneAhead
 * WHAT IT DOES: The scene's wait test: C clear when SCENE_COUNTER is $2C
 * or the actor's x + $90 is below Aladdin's x.
 * HOW: SCENE_COUNTER against $2C, equal: C clear, RTS; else (16-bit) the
 * actor's x + $90 (carry as it stands) against the player's x: C clear
 * when below.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
 * C as above; M 8-bit.
 */
void abi_GuardSceneAhead(void)
{
    lda8(ea_abs(SCENE_COUNTER));  /* A5E957 lda */
    cmp8_v(0x2C);  /* A5E95A cmp */
    clc();  /* A5E95C clc */
    if (IF_Z) goto L_A5E96B;  /* A5E95D beq */
    op_rep(0x20);  /* A5E95F rep */
    lda16(ea_dp(0x0C));  /* A5E961 lda */
    adc16_v(0x0090);  /* A5E963 adc */
    cmp16_v(rd16(ea_abs(PLAYER_X_AT)));  /* A5E966 cmp */
    op_sep(0x20);  /* A5E969 sep */
L_A5E96B:
    return;  /* A5E96B rts */
}

/*
 * @routine A5:E96C GuardSceneCharge
 * @actor
 * WHAT IT DOES: Scene 1: the actor charges left by motion $88 for $C0
 * frames, flies by $87 while falling, flies against by $88 until it
 * falls again, flies against 256 frames more, and goes on into
 * GuardSceneEnd.
 * HOW: ACTOR_08 low two bits set; ACTOR_41 up; AnimStart $1B; facing and
 * flip 1; ACTOR_42 $C0; motion $88, each frame ActorMove, 192 times;
 * motion $87, each frame ActorFly while ACTOR_SPEED_Y's top byte is
 * negative; motion $88, each frame ActorFlyAgainst while it is not
 * negative; ACTOR_42 0, each frame ActorFlyAgainst 256 times; then on into
 * GuardSceneEnd.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the jump
 * from GuardSceneStart. Exit: never returns.
 */
void abi_GuardSceneCharge(void)
{
    lda8(ea_dp(0x08));  /* A5E96C lda */
    ora8_v(0x03);  /* A5E96E ora */
    sta8(ea_dp(0x08));  /* A5E970 sta */
    MEM8(inc, ea_dp(0x41));  /* A5E972 inc */
    lda8_v(0x1B);  /* A5E974 lda */
    abi_AnimStart();  /* A5E976 jsl */
    lda8_v(0x01);  /* A5E97A lda */
    sta8(ea_dp(0x14));  /* A5E97C sta */
    sta8(ea_dp(0x15));  /* A5E97E sta */
    lda8_v(0xC0);  /* A5E980 lda */
    sta8(ea_dp(0x42));  /* A5E982 sta */
    ldy8_v(0x88);  /* A5E984 ldy */
    abi_ActorMotionPreset();  /* A5E986 jsl */
L_A5E98A:
    actor_yield(0xA5E98Au);  /* A5E98A brk */
    abi_ActorMove();  /* A5E98C jsl */
    MEM8(dec, ea_dp(0x42));  /* A5E990 dec */
    if (!IF_Z) goto L_A5E98A;  /* A5E992 bne */
    ldy8_v(0x87);  /* A5E994 ldy */
    abi_ActorMotionPreset();  /* A5E996 jsl */
L_A5E99A:
    actor_yield(0xA5E99Au);  /* A5E99A brk */
    abi_ActorFly();  /* A5E99C jsl */
    lda8(ea_dp(0x1F));  /* A5E9A0 lda */
    if (IF_N) goto L_A5E99A;  /* A5E9A2 bmi */
    ldy8_v(0x88);  /* A5E9A4 ldy */
    abi_ActorMotionPreset();  /* A5E9A6 jsl */
L_A5E9AA:
    actor_yield(0xA5E9AAu);  /* A5E9AA brk */
    abi_ActorFlyAgainst();  /* A5E9AC jsl */
    lda8(ea_dp(0x1F));  /* A5E9B0 lda */
    if (!IF_N) goto L_A5E9AA;  /* A5E9B2 bpl */
    stz8(ea_dp(0x42));  /* A5E9B4 stz */
L_A5E9B6:
    actor_yield(0xA5E9B6u);  /* A5E9B6 brk */
    abi_ActorFlyAgainst();  /* A5E9B8 jsl */
    MEM8(dec, ea_dp(0x42));  /* A5E9BC dec */
    if (!IF_Z) goto L_A5E9B6;  /* A5E9BE bne */
    abi_GuardSceneEnd();
    return;
}

/*
 * @routine A5:E9C0 GuardSceneEnd
 * @actor
 * WHAT IT DOES: The end of the scene: the actor goes on as GuardStop with
 * ACTOR_08's low bits 1, ACTOR_41 and ACTOR_4D 0.
 * HOW: ACTOR_08 AND 3 OR 1; ACTOR_41 and ACTOR_4D 0; on at GuardStop.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the jump
 * from GuardSceneStart, or by GuardSceneCharge falling into it. Exit:
 * never returns.
 */
void abi_GuardSceneEnd(void)
{
    lda8(ea_dp(0x08));  /* A5E9C0 lda */
    and8_v(0x03);  /* A5E9C2 and */
    ora8_v(0x01);  /* A5E9C4 ora */
    sta8(ea_dp(0x08));  /* A5E9C6 sta */
    stz8(ea_dp(0x41));  /* A5E9C8 stz */
    stz8(ea_dp(0x4D));  /* A5E9CA stz */
    abi_GuardStop();  /* A5E9CC jmp */
    return;
}

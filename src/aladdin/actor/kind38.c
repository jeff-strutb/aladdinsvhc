/*
 * kind38.c: actor kind $38, which rolls along until Aladdin is close, then
 * turns to him and flies up at him. Its wait is Kind37Wait
 * (actor_waits_small.c), shared with kind $37.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveX(void);
void abi_ActorNearPlayerX(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSpeedXSet(void);
void abi_ActorYByFrame47(void);
void abi_AnimStart(void);
void abi_RandomFromList(void);

#define KIND38_16 0x16u        /* word: $D1D6 */
#define KIND38_47 0x47u        /* ActorYByFrame47's frame kept */
#define KIND38_4A 0x4Au
#define KIND38_4B 0x4Bu        /* 8 */
#define KIND38_UP 0x45u        /* set once it flies (Kind37Wait then ends it as other actors) */
#define KIND38_HIT 0x46u       /* Kind37Wait's: 1 bounced, 2 downed by a shot */
#define KIND38_CHECKS 0x49u    /* near checks left before it may turn */
#define KIND38_SIDE 0x48u      /* the side Aladdin is on (ActorSideOfPlayer) */
#define KIND38_FRAMES 0x40u    /* frames left of a pause */

/*
 * @routine 83:BE01 Kind38Start
 * @actor
 * WHAT IT DOES: Actor kind $38: it rolls along bobbing with its frames
 * until Aladdin is close across (or it meets a wall), then, on a random
 * pick by which side he is on (else it rolls on for 16 more checks; no
 * script has that), it turns to him, pauses, shows itself (animation 1)
 * for 32 frames and flies up at him for good.
 * HOW: ACTOR_08 = 2, ACTOR_09 = $90, facing and flip 1, AnimStart 0,
 * $16 = $D1D6; $47 cleared, $4B = 8, $4A, KIND38_UP and KIND38_HIT
 * cleared, KIND38_CHECKS = 1. ActorSpeedXSet with Y = $18; each frame
 * (BRK) ActorMoveX, ActorYByFrame47; ActorOutOfWallProbe4 C set (no
 * script): KIND38_SIDE from ActorSideOfPlayer, on at the turn. Else
 * ActorNearPlayerX with Y = 2, C clear and KIND38_CHECKS counted down to
 * 0: KIND38_SIDE from ActorSideOfPlayer; RandomFromList with X = (side +
 * 3) * 2: zero, KIND38_CHECKS = $10 and roll on. The turn: KIND38_FRAMES
 * = $10, facing and flip = bit 0 of KIND38_SIDE, $10 frames; KIND38_UP
 * up, AnimStart 1, $20 frames; ActorMotionPreset with Y = $30; each frame
 * (BRK) ActorFlyAgainst.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $38 in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_RandomFromList(void);

void abi_Kind38Start(void)
{
    lda8_v(0x02);  /* 83BE01 lda */
    sta8(ea_dp(ACTOR_08));  /* 83BE03 sta */
    lda8_v(0x90);  /* 83BE05 lda */
    sta8(ea_dp(ACTOR_09));  /* 83BE07 sta */
    lda8_v(0x01);  /* 83BE09 lda */
    sta8(ea_dp(ACTOR_FACING));  /* 83BE0B sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83BE0D sta */
    lda8_v(0x00);  /* 83BE0F lda */
    abi_AnimStart();  /* 83BE11 jsl */
    op_rep(0x20);  /* 83BE15 rep */
    lda16_v(0xD1D6);  /* 83BE17 lda */
    sta16(ea_dp(KIND38_16));  /* 83BE1A sta */
    op_sep(0x20);  /* 83BE1C sep */
    stz8(ea_dp(KIND38_47));  /* 83BE1E stz */
    lda8_v(0x08);  /* 83BE20 lda */
    sta8(ea_dp(KIND38_4B));  /* 83BE22 sta */
    stz8(ea_dp(KIND38_4A));  /* 83BE24 stz */
    stz8(ea_dp(KIND38_UP));  /* 83BE26 stz */
    stz8(ea_dp(KIND38_HIT));  /* 83BE28 stz */
    lda8_v(0x01);  /* 83BE2A lda */
    sta8(ea_dp(KIND38_CHECKS));  /* 83BE2C sta */
    /* 83BE2E bra to the next instruction */
    ldy8_v(0x18);  /* 83BE30 ldy */
    abi_ActorSpeedXSet();  /* 83BE32 jsl */
L_83BE36:
    actor_yield(0x83BE36u);  /* 83BE36 brk */
    abi_ActorMoveX();  /* 83BE38 jsl */
    abi_ActorYByFrame47();  /* 83BE3C jsr */
    abi_ActorOutOfWallProbe4();  /* 83BE3F jsl */
    if (IF_C) goto L_83BE68;  /* 83BE43 bcs */
    ldy8_v(0x02);  /* 83BE45 ldy */
    abi_ActorNearPlayerX();  /* 83BE47 jsl */
    if (IF_C) goto L_83BE36;  /* 83BE4B bcs */
    MEM8(dec, ea_dp(KIND38_CHECKS));  /* 83BE4D dec */
    if (!IF_Z) goto L_83BE36;  /* 83BE4F bne */
    abi_ActorSideOfPlayer();  /* 83BE51 jsl */
    sta8(ea_dp(KIND38_SIDE));  /* 83BE55 sta */
    clc();  /* 83BE57 clc */
    adc8_v(0x03);  /* 83BE58 adc */
    ACC8(asl);  /* 83BE5A asl */
    tax8();  /* 83BE5B tax */
    abi_RandomFromList();  /* 83BE5C jsl */
    if (!IF_Z) goto L_83BE6E;  /* 83BE60 bne */
    lda8_v(0x10);  /* 83BE62 lda */
    sta8(ea_dp(KIND38_CHECKS));  /* 83BE64 sta */
    goto L_83BE36;  /* 83BE66 bra */
L_83BE68:
    abi_ActorSideOfPlayer();  /* 83BE68 jsl */
    sta8(ea_dp(KIND38_SIDE));  /* 83BE6C sta */
L_83BE6E:
    lda8_v(0x10);  /* 83BE6E lda */
    sta8(ea_dp(KIND38_FRAMES));  /* 83BE70 sta */
    lda8(ea_dp(KIND38_SIDE));  /* 83BE72 lda */
    and8_v(0x01);  /* 83BE74 and */
    sta8(ea_dp(ACTOR_FACING));  /* 83BE76 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83BE78 sta */
L_83BE7A:
    actor_yield(0x83BE7Au);  /* 83BE7A brk */
    MEM8(dec, ea_dp(KIND38_FRAMES));  /* 83BE7C dec */
    if (!IF_Z) goto L_83BE7A;  /* 83BE7E bne */
    MEM8(inc, ea_dp(KIND38_UP));  /* 83BE80 inc */
    lda8_v(0x01);  /* 83BE82 lda */
    abi_AnimStart();  /* 83BE84 jsl */
    lda8_v(0x20);  /* 83BE88 lda */
    sta8(ea_dp(KIND38_FRAMES));  /* 83BE8A sta */
L_83BE8C:
    actor_yield(0x83BE8Cu);  /* 83BE8C brk */
    MEM8(dec, ea_dp(KIND38_FRAMES));  /* 83BE8E dec */
    if (!IF_Z) goto L_83BE8C;  /* 83BE90 bne */
    ldy8_v(0x30);  /* 83BE92 ldy */
    abi_ActorMotionPreset();  /* 83BE94 jsl */
L_83BE98:
    actor_yield(0x83BE98u);  /* 83BE98 brk */
    abi_ActorFlyAgainst();  /* 83BE9A jsl */
    goto L_83BE98;  /* 83BE9E bra */
}


/*
 * kind58.c: actor kind $58, the platforms of stage a3 s0 that move to and
 * fro along a direction, which Aladdin rides or hangs from: the pose it
 * shows each frame. Its start is in kind58_start.c, its wait
 * Kind58Wait (kind_waits_rest.c). Named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);

#define K58_28 0x28u            /* set to $FF at its start */
#define K58_16 0x16u            /* word: $D3C0 */
#define K58_FRAMES 0x40u        /* frames left of this leg or rest */
#define K58_LEG 0x41u           /* frames of its first leg (a half leg); the others are twice */
#define K58_COPIER 0x42u        /* frames to Kind58Wait's copier test */
#define K58_CARRIES 0x43u       /* nonzero: Aladdin rides it this frame (ActorTestCarries) */
#define K58_LIFT 0x44u          /* ActorLiftPlayerStep's step: 0 none, positive hung from, negative let go */
#define K58_POSE 0x45u          /* its animation: 0, 1 ridden, 3 the variant, 4/5 hung from, $FF Aladdin dead */
#define K58_LEGS 0xD3C4u        /* in bank $80, by $07: the first leg's frames */
#define K58_DIRS 0xD3CAu        /* in bank $80, by $07: its direction */
#define HEALTH 0x0367u          /* the player's health */
#define PLAYER_FLIP (PLAYER + ACTOR_FLIP)

/*
 * @routine 83:DC4E Kind58Pose
 * WHAT IT DOES: Kind $58's pose for the frame: ridden, hung from, or
 * neither, and the dead pose once Aladdin has no health; starts the
 * animation when the pose changes.
 * HOW: By K58_POSE: negative, RTS. 0 or 3: health 0, pose $FF
 * (AnimStart 2); else pose 1 when K58_CARRIES; else, K58_LIFT positive,
 * pose 4 + the player's flip; else RTS. 1: pose 0 unless K58_CARRIES; when
 * it rides, K58_COPIER = $20 and RTS. 2: pose $FF. 4 and up: pose 0 once
 * K58_LIFT is 0. A new pose: K58_POSE = it, AnimStart with it.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit.
 */
void abi_Kind58Pose(void)
{
    lda8(ea_dp(K58_POSE));  /* 83DC4E lda */
    if (IF_N) return;  /* 83DC50 bmi, 83DC8F rts */
    if (IF_Z) goto L_83DC70;  /* 83DC52 beq */
    ACC8(dec);  /* 83DC54 dec */
    if (IF_Z) goto L_83DC65;  /* 83DC55 beq */
    ACC8(dec);  /* 83DC57 dec */
    if (IF_Z) goto L_83DC90;  /* 83DC58 beq */
    ACC8(dec);  /* 83DC5A dec */
    if (IF_Z) goto L_83DC70;  /* 83DC5B beq */
    lda8(ea_dp(K58_LIFT));  /* 83DC5D lda */
    if (!IF_Z) return;  /* 83DC5F bne, 83DC8F rts */
    ldx8_v(0x00);  /* 83DC61 ldx */
    goto L_83DC88;  /* 83DC63 bra */
L_83DC65:
    ldx8_v(0x00);  /* 83DC65 ldx */
    lda8(ea_dp(K58_CARRIES));  /* 83DC67 lda */
    if (IF_Z) goto L_83DC88;  /* 83DC69 beq */
    lda8_v(0x20);  /* 83DC6B lda */
    sta8(ea_dp(K58_COPIER));  /* 83DC6D sta */
    return;  /* 83DC6F rts */
L_83DC70:
    lda8(ea_abs(HEALTH));  /* 83DC70 lda */
    if (IF_Z) goto L_83DC90;  /* 83DC73 beq */
    ldx8_v(0x01);  /* 83DC75 ldx */
    lda8(ea_dp(K58_CARRIES));  /* 83DC77 lda */
    if (!IF_Z) goto L_83DC88;  /* 83DC79 bne */
    lda8(ea_dp(K58_LIFT));  /* 83DC7B lda */
    if (IF_Z) return;  /* 83DC7D beq, 83DC8F rts */
    if (IF_N) return;  /* 83DC7F bmi, 83DC8F rts */
    clc();  /* 83DC81 clc */
    lda8(ea_abs(PLAYER_FLIP));  /* 83DC82 lda */
    adc8_v(0x04);  /* 83DC85 adc */
    tax8();  /* 83DC87 tax */
L_83DC88:
    stx8(ea_dp(K58_POSE));  /* 83DC88 stx */
    txa8();  /* 83DC8A txa */
    abi_AnimStart();  /* 83DC8B jsl */
    return;  /* 83DC8F rts */
L_83DC90:
    lda8_v(0xFF);  /* 83DC90 lda */
    sta8(ea_dp(K58_POSE));  /* 83DC92 sta */
    lda8_v(0x02);  /* 83DC94 lda */
    abi_AnimStart();  /* 83DC96 jsl */
}

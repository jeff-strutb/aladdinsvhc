/*
 * kind58_start.c: actor kind $58's start. Named and described by hand.
 * Its pose helper Kind58Pose is in kind58.c.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorCarryPlayer(void);
void abi_ActorMoveBackByVector(void);
void abi_ActorMoveByVector(void);
void abi_AnimStart(void);
void abi_Kind58Pose(void);

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
 * @routine 83:DBBB Kind58Start
 * @actor
 * WHAT IT DOES: Actor kind $58 started: a platform that moves along its
 * direction and back, resting $40 frames at each end, showing by its
 * animation whether Aladdin rides it or hangs from it.
 * HOW: Bits 1 of $08 and $90 of $09 set, $28 = $FF, $16 = $D3C0, $15, $43
 * and $44 = 0. $07 bit 4 clear: pose 0 (AnimStart 0); set: $07 &= $0F,
 * pose 3 (AnimStart 3). K58_LEG = K58_FRAMES = K58_LEGS[$07], $14 =
 * K58_DIRS[$07]. Then for good: each frame (BRK first) Kind58Pose,
 * ActorMoveByVector $14 and ActorCarryPlayer for K58_FRAMES frames; $40
 * frames of Kind58Pose; 2 * K58_LEG frames of Kind58Pose,
 * ActorMoveBackByVector $14 and ActorCarryPlayer; $40 frames of
 * Kind58Pose; the next legs forward are 2 * K58_LEG frames too.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for state $0C. Exit: never returns.
 */
void abi_Kind58Start(void)
{
    lda8(ea_dp(ACTOR_08));  /* 83DBBB lda */
    ora8_v(0x02);  /* 83DBBD ora */
    sta8(ea_dp(ACTOR_08));  /* 83DBBF sta */
    lda8(ea_dp(ACTOR_09));  /* 83DBC1 lda */
    ora8_v(0x90);  /* 83DBC3 ora */
    sta8(ea_dp(ACTOR_09));  /* 83DBC5 sta */
    lda8_v(0xFF);  /* 83DBC7 lda */
    sta8(ea_dp(K58_28));  /* 83DBC9 sta */
    op_rep(0x20);  /* 83DBCB rep */
    lda16_v(0xD3C0);  /* 83DBCD lda */
    sta16(ea_dp(K58_16));  /* 83DBD0 sta */
    op_sep(0x20);  /* 83DBD2 sep */
    stz8(ea_dp(ACTOR_FLIP));  /* 83DBD4 stz */
    stz8(ea_dp(K58_CARRIES));  /* 83DBD6 stz */
    stz8(ea_dp(K58_LIFT));  /* 83DBD8 stz */
    lda8(ea_dp(ACTOR_07));  /* 83DBDA lda */
    bit8_i(0x10);  /* 83DBDC bit */
    if (!IF_Z) goto L_83DBEA;  /* 83DBDE bne */
    lda8_v(0x00);  /* 83DBE0 lda */
    sta8(ea_dp(K58_POSE));  /* 83DBE2 sta */
    jsl_frame(0x83DBE4u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83DBE4 jsl */
    goto L_83DBF6;  /* 83DBE8 bra */
L_83DBEA:
    and8_v(0x0F);  /* 83DBEA and */
    sta8(ea_dp(ACTOR_07));  /* 83DBEC sta */
    lda8_v(0x03);  /* 83DBEE lda */
    sta8(ea_dp(K58_POSE));  /* 83DBF0 sta */
    jsl_frame(0x83DBF2u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83DBF2 jsl */
L_83DBF6:
    ldx8(ea_dp(ACTOR_07));  /* 83DBF6 ldx */
    lda8(ea_absx(K58_LEGS));  /* 83DBF8 lda */
    sta8(ea_dp(K58_LEG));  /* 83DBFB sta */
    sta8(ea_dp(K58_FRAMES));  /* 83DBFD sta */
    lda8(ea_absx(K58_DIRS));  /* 83DBFF lda */
    sta8(ea_dp(ACTOR_FACING));  /* 83DC02 sta */
L_83DC04:
    actor_yield(0x83DC04u);  /* 83DC04 brk */
    call_frame(0xDC08, abi_Kind58Pose);  /* 83DC06 jsr */
    ldx8_v(0x14);  /* 83DC09 ldx */
    jsl_frame(0x83DC0Bu); abi_ActorMoveByVector(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83DC0B jsl */
    call_frame(0xDC11, abi_ActorCarryPlayer);  /* 83DC0F jsr */
    MEM8(dec, ea_dp(K58_FRAMES));  /* 83DC12 dec */
    if (!IF_Z) goto L_83DC04;  /* 83DC14 bne */
    lda8_v(0x40);  /* 83DC16 lda */
    sta8(ea_dp(K58_FRAMES));  /* 83DC18 sta */
L_83DC1A:
    actor_yield(0x83DC1Au);  /* 83DC1A brk */
    call_frame(0xDC1E, abi_Kind58Pose);  /* 83DC1C jsr */
    MEM8(dec, ea_dp(K58_FRAMES));  /* 83DC1F dec */
    if (!IF_Z) goto L_83DC1A;  /* 83DC21 bne */
    lda8(ea_dp(K58_LEG));  /* 83DC23 lda */
    ACC8(asl);  /* 83DC25 asl */
    sta8(ea_dp(K58_FRAMES));  /* 83DC26 sta */
L_83DC28:
    actor_yield(0x83DC28u);  /* 83DC28 brk */
    call_frame(0xDC2C, abi_Kind58Pose);  /* 83DC2A jsr */
    ldx8_v(0x14);  /* 83DC2D ldx */
    jsl_frame(0x83DC2Fu); abi_ActorMoveBackByVector(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83DC2F jsl */
    call_frame(0xDC35, abi_ActorCarryPlayer);  /* 83DC33 jsr */
    MEM8(dec, ea_dp(K58_FRAMES));  /* 83DC36 dec */
    if (!IF_Z) goto L_83DC28;  /* 83DC38 bne */
    lda8_v(0x40);  /* 83DC3A lda */
    sta8(ea_dp(K58_FRAMES));  /* 83DC3C sta */
L_83DC3E:
    actor_yield(0x83DC3Eu);  /* 83DC3E brk */
    call_frame(0xDC42, abi_Kind58Pose);  /* 83DC40 jsr */
    MEM8(dec, ea_dp(K58_FRAMES));  /* 83DC43 dec */
    if (!IF_Z) goto L_83DC3E;  /* 83DC45 bne */
    lda8(ea_dp(K58_LEG));  /* 83DC47 lda */
    ACC8(asl);  /* 83DC49 asl */
    sta8(ea_dp(K58_FRAMES));  /* 83DC4A sta */
    goto L_83DC04;  /* 83DC4C bra */
}


/*
 * kind40.c: actor kind $40, a spring Aladdin bounces off. Its wait is
 * Kind40Wait (actor_waits_small.c).
 */
#include "machine.h"
#include "ops65.h"
#include "wram_state.h"
#include "wram_player.h"
#include "actor.h"

void abi_ActorHitsTestBoxLong(void);
void abi_AnimStart(void);

#define KIND40_16 0x16u            /* word: $D29B */
#define PLAYER_SPRUNG 0xD8FDu      /* where the player's code goes on when it springs him */
#define PLAYER_SPEED_Y_HI (PLAYER + ACTOR_SPEED_Y + 2)
#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1)

/*
 * @routine 83:C507 Kind40Start
 * @actor
 * WHAT IT DOES: Actor kind $40, a spring: while Aladdin falls onto it from
 * above it plays its push (animation 1), sends his code on at
 * PLAYER_SPRUNG and rests 32 frames; a touch while he has no health turns
 * it to state $8C (no script has that).
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; KIND40_16 = $D29B. Then
 * AnimStart 0 and each frame (BRK) until V_141C is set, his speed y high
 * byte not negative and ActorHitsTestBoxLong clears C. Health 0: state
 * $8C, back to AnimStart 0. Else his y plus 8 less its y: no borrow, wait
 * on; a borrow (he is above): AnimStart 1, his resume address = $D8FD,
 * COP $20, back to AnimStart 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $40 in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind40Start(void)
{
    lda8(ea_dp(ACTOR_08));  /* 83C507 lda */
    ora8_v(0x02);  /* 83C509 ora */
    sta8(ea_dp(ACTOR_08));  /* 83C50B sta */
    lda8(ea_dp(ACTOR_09));  /* 83C50D lda */
    ora8_v(0x90);  /* 83C50F ora */
    sta8(ea_dp(ACTOR_09));  /* 83C511 sta */
    op_rep(0x20);  /* 83C513 rep */
    lda16_v(0xD29B);  /* 83C515 lda */
    sta16(ea_dp(KIND40_16));  /* 83C518 sta */
    op_sep(0x20);  /* 83C51A sep */
L_83C51C:
    lda8_v(0x00);  /* 83C51C lda */
    abi_AnimStart();  /* 83C51E jsl */
L_83C522:
    actor_yield(0x83C522u);  /* 83C522 brk */
    lda8(ea_abs(V_141C));  /* 83C524 lda */
    if (IF_Z) goto L_83C522;  /* 83C527 beq */
    lda8(ea_abs(PLAYER_SPEED_Y_HI));  /* 83C529 lda */
    if (IF_N) goto L_83C522;  /* 83C52C bmi */
    abi_ActorHitsTestBoxLong();  /* 83C52E jsl */
    if (IF_C) goto L_83C522;  /* 83C532 bcs */
    lda8(ea_abs(HEALTH));  /* 83C534 lda */
    if (!IF_Z) goto L_83C53F;  /* 83C537 bne */
    lda8_v(0x8C);  /* 83C539 lda */
    sta8(ea_dp(ACTOR_STATE));  /* 83C53B sta */
    goto L_83C51C;  /* 83C53D bra */
L_83C53F:
    op_rep(0x20);  /* 83C53F rep */
    lda16(ea_abs(PLAYER_Y_AT));  /* 83C541 lda */
    clc();  /* 83C544 clc */
    adc16_v(0x0008);  /* 83C545 adc */
    sec();  /* 83C548 sec */
    sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));  /* 83C549 sbc */
    op_sep(0x20);  /* 83C54B sep */
    if (IF_C) goto L_83C522;  /* 83C54D bcs */
    lda8_v(0x01);  /* 83C54F lda */
    abi_AnimStart();  /* 83C551 jsl */
    lda8_v((uint8_t)PLAYER_SPRUNG);  /* 83C555 lda */
    sta8(ea_abs(PLAYER + ACTOR_03));  /* 83C557 sta */
    lda8_v((uint8_t)(PLAYER_SPRUNG >> 8));  /* 83C55A lda */
    sta8(ea_abs(PLAYER + ACTOR_04));  /* 83C55C sta */
    lda8_v(0x20);  /* 83C55F lda */
    actor_wait(0x83C561u);  /* 83C561 cop */
    goto L_83C51C;  /* 83C563 bra */
}

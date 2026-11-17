/*
 * kind2a.c: actor kind $2A's second state, downed: knocked away, a short
 * flight up and a fall. Its start ($83:A1E5) is not filed yet; its wait
 * is Kind2AWait (actor_waits_small.c).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAccelerateY(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMotionReset(void);
void abi_ActorStepY(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define KIND2A_DOWNS 0x47u     /* counted each time it is downed */
#define KIND2A_HITS 0x48u      /* counted when downed by a shot (Kind2AWait) */
#define KIND2A_RIGHT 0x4Cu     /* counted when downed facing right (Kind2AWait) */
#define KIND2A_FRAMES 0x43u    /* frames left of its flight */
#define PLAYER_FLIP_AT (PLAYER + ACTOR_FLIP)
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)
#define COPY_TEST 0x701331u    /* save RAM, which the retail board lacks: a write there reads back on a copier */

/*
 * @routine 83:A37B Kind2ASecond
 * @actor
 * WHAT IT DOES: Actor kind $2A in its second state (downed): it is knocked
 * away from Aladdin, flies up a little, then falls for good (its wait
 * ends it out of range). On a cartridge whose save RAM can be written
 * (a copy, not the retail board) each frame of the fall also pushes the
 * player 16 pixels right.
 * HOW: AnimStart 4; ActorMotionPreset with Y = $28; ACTOR_09 bit 7 set;
 * KIND2A_DOWNS up. Facing negative (no script has it): facing and flip =
 * its bit 0, speed x high byte up, on at the fly. Else KIND2A_RIGHT zero:
 * SoundQueueAdd $0F, y plus 9. KIND2A_HITS zero: facing away from the
 * player's flip byte, COP 9. ActorMotionPreset with Y = $28; 16 frames
 * (BRK) of ActorFly; AnimStart 3; ActorMotionReset with A = $10; then each
 * frame (BRK) AnimStep, ActorAccelerateY unless the speed y high byte is
 * 3, ActorStepY, and the byte at COPY_TEST counted up: read back equal
 * (save RAM), the player's x plus 16.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $2A in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind2ASecond(void)
{
    lda8_v(0x04);  /* 83A37B lda */
    abi_AnimStart();  /* 83A37D jsl */
    ldy8_v(0x28);  /* 83A381 ldy */
    abi_ActorMotionPreset();  /* 83A383 jsl */
    lda8_v(0x80);  /* 83A387 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83A389 tsb */
    MEM8(inc, ea_dp(KIND2A_DOWNS));  /* 83A38B inc */
    lda8(ea_dp(ACTOR_FACING));  /* 83A38D lda */
    if (!IF_N) goto L_83A39B;  /* 83A38F bpl */
    and8_v(0x01);  /* 83A391 and */
    sta8(ea_dp(ACTOR_FACING));  /* 83A393 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A395 sta */
    MEM8(inc, ea_dp(ACTOR_SPEED_X + 1));  /* 83A397 inc */
    goto L_83A3C7;  /* 83A399 bra */
L_83A39B:
    lda8(ea_dp(KIND2A_RIGHT));  /* 83A39B lda */
    if (!IF_Z) goto L_83A3B0;  /* 83A39D bne */
    lda8_v(0x0F);  /* 83A39F lda */
    abi_SoundQueueAdd();  /* 83A3A1 jsl */
    op_rep(0x21);  /* 83A3A5 rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83A3A7 lda */
    adc16_v(0x0009);  /* 83A3A9 adc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83A3AC sta */
    op_sep(0x20);  /* 83A3AE sep */
L_83A3B0:
    lda8(ea_dp(KIND2A_HITS));  /* 83A3B0 lda */
    if (!IF_Z) goto L_83A3C1;  /* 83A3B2 bne */
    lda8(ea_abs(PLAYER_FLIP_AT));  /* 83A3B4 lda */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A3B7 sta */
    eor8_v(0x01);  /* 83A3B9 eor */
    sta8(ea_dp(ACTOR_FACING));  /* 83A3BB sta */
    lda8_v(0x09);  /* 83A3BD lda */
    actor_wait(0x83A3BFu);  /* 83A3BF cop */
L_83A3C1:
    ldy8_v(0x28);  /* 83A3C1 ldy */
    abi_ActorMotionPreset();  /* 83A3C3 jsl */
L_83A3C7:
    lda8_v(0x10);  /* 83A3C7 lda */
    sta8(ea_dp(KIND2A_FRAMES));  /* 83A3C9 sta */
L_83A3CB:
    actor_yield(0x83A3CBu);  /* 83A3CB brk */
    abi_ActorFly();  /* 83A3CD jsl */
    MEM8(dec, ea_dp(KIND2A_FRAMES));  /* 83A3D1 dec */
    if (!IF_Z) goto L_83A3CB;  /* 83A3D3 bne */
    lda8_v(0x03);  /* 83A3D5 lda */
    abi_AnimStart();  /* 83A3D7 jsl */
    lda8_v(0x10);  /* 83A3DB lda */
    abi_ActorMotionReset();  /* 83A3DD jsl */
L_83A3E1:
    actor_yield(0x83A3E1u);  /* 83A3E1 brk */
    abi_AnimStep();  /* 83A3E3 jsl */
    lda8(ea_dp(ACTOR_SPEED_Y + 1));  /* 83A3E7 lda */
    cmp8_v(0x03);  /* 83A3E9 cmp */
    if (IF_Z) goto L_83A3F1;  /* 83A3EB beq */
    abi_ActorAccelerateY();  /* 83A3ED jsl */
L_83A3F1:
    abi_ActorStepY();  /* 83A3F1 jsl */
    lda8(ea_long(COPY_TEST));  /* 83A3F5 lda */
    ACC8(inc);  /* 83A3F9 inc */
    sta8(ea_long(COPY_TEST));  /* 83A3FA sta */
    cmp8_v(rd8(ea_long(COPY_TEST)));  /* 83A3FE cmp */
    if (!IF_Z) goto L_83A411;  /* 83A402 bne */
    op_rep(0x21);  /* 83A404 rep */
    lda16(ea_abs(PLAYER_X_AT));  /* 83A406 lda */
    adc16_v(0x0010);  /* 83A409 adc */
    sta16(ea_abs(PLAYER_X_AT));  /* 83A40C sta */
    op_sep(0x20);  /* 83A40F sep */
L_83A411:
    goto L_83A3E1;  /* 83A411 bra */
}
